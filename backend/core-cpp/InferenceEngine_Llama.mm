#include "InferenceEngine.hpp"
#include "GPUAllocator.hpp"
#include "ContextManager.hpp"
#include "GrammarLibrary.hpp"
#include "ToolDispatcher.hpp"
#include "mach/MachPriority.hpp"
#undef Point
#undef Size
#undef Rect

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <filesystem>
#include "llama.h"
#include "common.h"
#include "sampling.h"

namespace fs = std::filesystem;

namespace interlock {

void InferenceEngine::initializeLlama() {
    @autoreleasepool {
        std::cout << "[InferenceEngine] Initializing LLaMA Backend on M4 Pro (Native Multi-Model)..." << std::endl;

        llama_backend_init();

        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (device) {
            std::cout << "[InferenceEngine] Metal Acceleration Ready: " << [device.name UTF8String] << std::endl;
            mtl_device = (__bridge_retained void*)device;
        }
    }
}

bool InferenceEngine::loadModelLlama(const std::string& model_id, const std::string& model_path) {
    if (models.find(model_id) != models.end()) {
        std::cout << "[InferenceEngine] Model [" << model_id << "] already loaded." << std::endl;
        return true;
    }

    std::cout << "[InferenceEngine] Loading model [" << model_id << "] from: " << model_path << std::endl;

    // Unified Memory Management: Estimate VRAM usage based on file size
    std::error_code ec;
    uintmax_t fileSize = fs::file_size(model_path, ec);
    float estimated_gb = 0.0f;
    if (!ec) {
        estimated_gb = static_cast<float>(fileSize) / (1024.0f * 1024.0f * 1024.0f);
    }

    if (!GPUAllocator::getInstance().reserveVRAM(model_id, estimated_gb)) {
        std::cout << "[InferenceEngine] VRAM pressure detected. Attempting LRU eviction to make room..." << std::endl;
        while (evictOldestModel()) {
            if (GPUAllocator::getInstance().reserveVRAM(model_id, estimated_gb)) {
                goto reserved;
            }
        }
        std::cerr << "[InferenceEngine] VRAM Reservation Failed for [" << model_id << "] after eviction attempts." << std::endl;
        return false;
    }

reserved:

    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 99; // Offload everything to GPU (M4 Pro Unified Memory)
    model_params.use_mlock = true;
    model_params.use_mmap = true;   // Direct mapping to avoid redundant copies
    model_params.vocab_only = false;

    struct llama_model* m = llama_model_load_from_file(model_path.c_str(), model_params);
    if (!m) {
        std::cerr << "[InferenceEngine] Failed to load model: " << model_id << std::endl;
        GPUAllocator::getInstance().releaseVRAM(model_id);
        return false;
    }

    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 16384; // Ampliamos contexto para el M4 Pro
    ctx_params.n_batch = 1024;
    ctx_params.n_ubatch = 512; // Optimized micro-batching for Metal throughput
    ctx_params.n_threads = 12; // M4 Pro Performance Cores
    ctx_params.embeddings = true; // Habilitamos embeddings para RAG nativo
    ctx_params.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_ENABLED; // Flash Attention nativo en Metal
    struct llama_context* c = llama_new_context_with_model(m, ctx_params);
    if (!c) {
        std::cerr << "[InferenceEngine] Failed to create context for: " << model_id << std::endl;
        llama_model_free(m);
        return false;
    }

    models[model_id] = m;
    contexts[model_id] = c;
    model_last_used[model_id] = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    model_backends[model_id] = BackendType::LLAMA;

    if (active_model_id.empty()) {
        active_model_id = model_id;
    }

    std::cout << "[InferenceEngine] Model [" << model_id << "] loaded and ready." << std::endl;
    return true;
}

void InferenceEngine::unloadModelLlama(const std::string& model_id) {
    if (models.find(model_id) != models.end()) {
        if (contexts[model_id]) llama_free(contexts[model_id]);
        if (models[model_id]) llama_model_free(models[model_id]);
        contexts.erase(model_id);
        models.erase(model_id);
        model_last_used.erase(model_id);
        model_backends.erase(model_id);
        GPUAllocator::getInstance().releaseVRAM(model_id);
        std::cout << "[InferenceEngine] Model [" << model_id << "] unloaded." << std::endl;
    }
}

std::future<InferenceResponse> InferenceEngine::generateLlama(const InferenceRequest& request) {
    return std::async(std::launch::async, [this, request]() -> InferenceResponse {
        // Optimizar para P-Cores del M4 Pro durante la generación
        mach::SetThreadPriority(mach_thread_self(), mach::TaskPriority::HIGH);
        mach::SetThreadAffinity(mach_thread_self(), 2); // Cluster 2 (P-Cores secundarios para cómputo)

        auto start = std::chrono::high_resolution_clock::now();

        std::string target_id = request.model_id.empty() ? active_model_id : request.model_id;

        if (contexts.find(target_id) == contexts.end()) {
            InferenceResponse resp;
            resp.text = "Error: Model [" + target_id + "] not loaded in Llama.";
            return resp;
        }

        struct llama_context* ctx = contexts[target_id];
        struct llama_model* model = models[target_id];

        const struct llama_vocab* vocab = llama_model_get_vocab(model);
        std::vector<llama_token> tokens = common_tokenize(vocab, request.prompt, true, true);

        int n_past = ContextManager::getInstance().prepareContext(target_id, tokens, llama_n_ctx(ctx));

        {
            std::lock_guard<std::recursive_mutex> lock(engine_mtx);
            model_last_used[target_id] = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        }

        common_params_sampling sparams;
        sparams.temp = request.temperature;
        if (request.grammar.empty()) {
            sparams.grammar.grammar = GrammarLibrary::ASSESSMENT_ACTION_XML;
        } else {
            sparams.grammar.grammar = request.grammar;
        }
        sparams.grammar.type = COMMON_GRAMMAR_TYPE_USER;

        struct common_sampler* smpl = common_sampler_init(model, sparams);

        llama_batch batch = llama_batch_init((int32_t)tokens.size(), 0, 1);
        batch.n_tokens = (int32_t)tokens.size();
        for (size_t i = 0; i < tokens.size(); i++) {
            batch.token[i] = tokens[i];
            batch.pos[i] = (llama_pos)(n_past + i);
            batch.n_seq_id[i] = 1;
            batch.seq_id[i][0] = 0;
            batch.logits[i] = false;
        }
        batch.logits[tokens.size() - 1] = true;

        llama_decode(ctx, batch);
        llama_batch_free(batch);

        std::vector<llama_token> generated_tokens;
        while (generated_tokens.size() < request.max_tokens) {
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

        std::string result_text;
        for (auto const& token : generated_tokens) {
            char buf[128];
            int n = llama_token_to_piece(vocab, token, buf, sizeof(buf), 0, true);
            if (n > 0) result_text += std::string(buf, n);
        }

        auto end = std::chrono::high_resolution_clock::now();
        float latency = std::chrono::duration<float, std::milli>(end - start).count();

        InferenceResponse response;
        response.text = result_text;
        response.tokens_generated = (int)generated_tokens.size();
        response.latency_ms = latency;
        response.tokens_per_second = (response.tokens_generated / (latency / 1000.0f));
        response.finish_reason = "stop";

        ToolDispatcher::getInstance().dispatchActions(target_id, result_text);
        common_sampler_free(smpl);
        return response;
    });
}

void InferenceEngine::allocatePoolLlama(const std::string& pool_name, size_t size_gb) {
    @autoreleasepool {
        if (!mtl_device) return;
        id<MTLDevice> device = (__bridge id<MTLDevice>)mtl_device;
        size_t bytes = size_gb * 1024LL * 1024LL * 1024LL;
        id<MTLBuffer> buffer = [device newBufferWithLength:bytes options:MTLResourceStorageModeShared];
        if (buffer) {
            vram_pools[pool_name] = (__bridge_retained void*)buffer;
        }
    }
}

void InferenceEngine::releasePoolLlama(const std::string& pool_name) {
    auto it = vram_pools.find(pool_name);
    if (it != vram_pools.end()) {
        CFRelease(it->second);
        vram_pools.erase(it);
    }
}

bool InferenceEngine::loadDraftModelLlama(const std::string& draft_model_path) {
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 99;
    draft_model = llama_model_load_from_file(draft_model_path.c_str(), model_params);
    if (!draft_model) return false;
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = 4096;
    draft_ctx = llama_new_context_with_model(draft_model, ctx_params);
    return true;
}

bool InferenceEngine::applyLoraLlama(const std::string& model_id, const std::string& lora_path, float scale) {
    struct llama_adapter_lora* adapter = llama_adapter_lora_init(models[model_id], lora_path.c_str());
    if (!adapter) return false;
    return llama_set_adapters_lora(contexts[model_id], &adapter, 1, &scale) == 0;
}

bool InferenceEngine::evictOldestModelLlama(const std::string& except_id) {
    std::string oldest_model = "";
    int64_t oldest_time = INT64_MAX;
    for (const auto& [id, backend] : model_backends) {
        if (id == except_id || backend != BackendType::LLAMA) continue;
        auto it = model_last_used.find(id);
        if (it != model_last_used.end() && it->second < oldest_time) {
            oldest_time = it->second;
            oldest_model = id;
        }
    }
    if (!oldest_model.empty()) {
        unloadModelLlama(oldest_model);
        return true;
    }
    return false;
}

} // namespace interlock
