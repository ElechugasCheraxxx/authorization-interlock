#pragma once

#include "InferenceEngine.hpp"
#include "RecurrentEngine.hpp"
#include <string>
#include <memory>
#include <future>
#include <vector>

namespace interlock {

class AgentEngine {
public:
    AgentEngine(const std::string& agent_id, const std::string& model_path);
    ~AgentEngine();

    // Ciclo de vida independiente
    bool load();
    void unload();
    bool isLoaded() const { return ctx != nullptr; }

    // Inferencia dedicada
    std::future<InferenceResponse> think(const std::string& prompt, const std::string& grammar = "");

    /**
     * Deep Think: Razonamiento recurrente usando LTI Injection.
     * Permite al agente iterar internamente antes de dar una respuesta final.
     */
    std::future<InferenceResponse> deepThink(const std::string& prompt, const std::string& grammar = "");

private:
    std::string agent_id;
    std::string model_path;
    struct llama_model* model = nullptr;
    struct llama_context* ctx = nullptr;

    int context_window = 16384;

    // OpenMythos LTI Logic
    std::unique_ptr<RecurrentEngine> recurrent_engine;
    std::vector<float> hidden_state;

    void updateRecurrentState(const std::vector<float>& input_embeddings, const std::vector<float>& delta);
};

} // namespace interlock
