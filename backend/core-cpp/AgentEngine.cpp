#include "AgentEngine.hpp"
#include "GPUAllocator.hpp"
#include "GrammarLibrary.hpp"
#include "ToolDispatcher.hpp"
#include "ContextManager.hpp"
#include "llama.h"
#include "common.h"
#include "sampling.h"
#include <iostream>
#include <chrono>
#include <cmath>

namespace interlock {

AgentEngine::AgentEngine(const std::string& agent_id, const std::string& model_path)
    : agent_id(agent_id), model_path(model_path) {
    std::cout << "[AgentEngine:" << agent_id << "] Dedicated engine initialized (Ready to load)." << std::endl;
}

AgentEngine::~AgentEngine() {
    unload();
}

bool AgentEngine::load() {
    if (isLoaded()) return true;

    std::cout << "[AgentEngine:" << agent_id << "] Loading model into dedicated VRAM slot..." << std::endl;

    // 1. Reservar VRAM (M4 Pro Unified Memory)
    if (!GPUAllocator::getInstance().reserveVRAM(agent_id, 8.0f)) {
        std::cerr << "[AgentEngine:" << agent_id << "] Error: Insufficient VRAM." << std::endl;
        return false;
    }

    // 2. Cargar Modelo
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 99;
    model_params.use_mlock = true;

    model = llama_model_load_from_file(model_path.c_str(), model_params);
    if (!model) {
        GPUAllocator::getInstance().releaseVRAM(agent_id);
        return false;
    }

    // 3. Crear Contexto Dedicado
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = context_window;
    ctx_params.n_batch = 512;
    ctx_params.n_threads = 8;
    ctx_params.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_ENABLED;

    ctx = llama_new_context_with_model(model, ctx_params);
    if (!ctx) {
        llama_model_free(model);
        model = nullptr;
        GPUAllocator::getInstance().releaseVRAM(agent_id);
        return false;
    }

    // Inicializar Motor Recurrente (OpenMythos LTI)
    int n_embd = llama_n_embd(model);
    recurrent_engine = std::make_unique<RecurrentEngine>(n_embd);
    hidden_state.assign(n_embd, 0.0f);

    return true;
}

void AgentEngine::unload() {
    if (ctx) {
        llama_free(ctx);
        ctx = nullptr;
    }
    if (model) {
        llama_model_free(model);
        model = nullptr;
    }
    GPUAllocator::getInstance().releaseVRAM(agent_id);
    std::cout << "[AgentEngine:" << agent_id << "] Unloaded from VRAM." << std::endl;
}

std::future<InferenceResponse> AgentEngine::think(const std::string& prompt, const std::string& grammar) {
    return std::async(std::launch::async, [this, prompt, grammar]() -> InferenceResponse {
        if (!isLoaded() && !load()) {
            return {"Error: Engine failed to load", 0, 0.0f, 0.0f, "error"};
        }

        auto start = std::chrono::high_resolution_clock::now();

        const struct llama_vocab* vocab = llama_model_get_vocab(model);
        std::vector<llama_token> tokens = common_tokenize(vocab, prompt, true, true);

        int n_past = ContextManager::getInstance().prepareContext(agent_id, tokens, llama_n_ctx(ctx));

        common_params_sampling sparams;
        sparams.temp = 0.7f;
        sparams.grammar.grammar = grammar.empty() ? GrammarLibrary::ASSESSMENT_ACTION_XML : grammar;
        sparams.grammar.type = COMMON_GRAMMAR_TYPE_USER;

        struct ::common_sampler* smpl = common_sampler_init(model, sparams);

        llama_batch batch = llama_batch_init(tokens.size(), 0, 1);
        batch.n_tokens = tokens.size();
        for (size_t i = 0; i < tokens.size(); i++) {
            batch.token[i] = tokens[i];
            batch.pos[i] = n_past + i;
            batch.n_seq_id[i] = 1;
            batch.seq_id[i][0] = 0;
            batch.logits[i] = false;
        }
        batch.logits[tokens.size() - 1] = true;

        llama_decode(ctx, batch);
        llama_batch_free(batch);

        std::vector<llama_token> generated_tokens;
        while (generated_tokens.size() < 1024) {
            llama_token curr_token = common_sampler_sample(smpl, ctx, -1);
            common_sampler_accept(smpl, curr_token, true);

            if (llama_vocab_is_eog(vocab, curr_token)) break;
            generated_tokens.push_back(curr_token);

            llama_batch b = llama_batch_init(1, 0, 1);
            b.n_tokens = 1;
            b.token[0] = curr_token;
            b.pos[0] = (llama_pos)(tokens.size() + generated_tokens.size() - 1);
            b.n_seq_id[0] = 1;
            b.seq_id[0][0] = 0;
            b.logits[0] = true;

            llama_decode(ctx, b);
            llama_batch_free(b);
        }

        ContextManager::getInstance().commitTokens(agent_id, tokens.size() + generated_tokens.size());

        std::string result_text;
        for (auto const& t : generated_tokens) {
            char buf[128];
            int n = llama_token_to_piece(vocab, t, buf, sizeof(buf), 0, true);
            if (n > 0) result_text += std::string(buf, n);
        }

        ToolDispatcher::getInstance().dispatchActions(agent_id, result_text);

        auto end = std::chrono::high_resolution_clock::now();
        float latency = std::chrono::duration<float, std::milli>(end - start).count();

        common_sampler_free(smpl);

        InferenceResponse res;
        res.text = result_text;
        res.tokens_generated = generated_tokens.size();
        res.latency_ms = latency;
        res.tokens_per_second = (generated_tokens.size() / latency) * 1000.0f;

        return res;
    });
}

std::future<InferenceResponse> AgentEngine::deepThink(const std::string& prompt, const std::string& grammar) {
    return std::async(std::launch::async, [this, prompt, grammar]() -> InferenceResponse {
        if (!isLoaded() && !load()) {
            return {"Error: Engine failed to load", 0, 0.0f, 0.0f, "error"};
        }

        auto start = std::chrono::high_resolution_clock::now();
        const struct llama_vocab* vocab = llama_model_get_vocab(model);
        int n_embd = llama_n_embd(model);

        std::vector<llama_token> tokens = common_tokenize(vocab, prompt, true, true);

        // Fase 1: Inyección LTI y Estabilización de Contexto
        // Simulamos el paso recurrente sobre los embeddings del prompt
        std::vector<float> input_embedding(n_embd, 0.0f); // Simplificado para el ejemplo
        // En una implementación real, extraeríamos los embeddings de llama.cpp

        int reasoning_steps = 0;
        bool converged = false;

        while (reasoning_steps < 5 && !converged) {
            std::vector<float> delta(n_embd, 0.01f); // Simulación de gradiente de pensamiento
            recurrent_engine->step(hidden_state, input_embedding, delta);

            if (recurrent_engine->shouldHalt(hidden_state, 0.95f)) {
                converged = true;
            }
            reasoning_steps++;
        }

        // Fase 2: Inferencia estándar con el estado refinado
        // (Nota: En OpenMythos, el hidden_state se inyectaría como un token virtual o vía KV cache)
        auto response = think(prompt + "\n<thought_converged>", grammar).get();
        response.finish_reason = "deep_think_completed_" + std::to_string(reasoning_steps);

        return response;
    });
}

void AgentEngine::updateRecurrentState(const std::vector<float>& input_embeddings, const std::vector<float>& delta) {
    if (recurrent_engine) {
        recurrent_engine->step(hidden_state, input_embeddings, delta);
    }
}

} // namespace interlock
