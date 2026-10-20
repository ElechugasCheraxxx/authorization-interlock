#pragma once

#include <string>
#include <vector>
#include <memory>
#include <future>
#include <functional>
#include <map>
#include <mutex>
#include "Schema.hpp"

// Forward declarations
struct llama_model;
struct llama_context;
struct common_sampler;

namespace mlx { namespace core { class array; } } // MLX Forward decl

namespace interlock {

    struct InferenceRequest {
        std::string model_id;
        std::string prompt;
        float temperature = 0.7f;
        int max_tokens = 512;
        std::string grammar; // GBNF Grammar for structured output
        std::vector<std::string> stop_sequences;
        std::map<int, float> logit_bias; // Milestone 2.6
        json sampling_params;
        std::vector<std::string> rop_gadgets; // Fase 3.1
        std::map<uint64_t, std::string> memory_map; // Fase 3.1
        std::string lora_adapter; // Milestone 2.12
    };

struct InferenceResponse {
    std::string text;
    int tokens_generated = 0;
    float latency_ms = 0.0f;
    float tokens_per_second = 0.0f;
    std::string finish_reason;
};

class InferenceEngine {
public:
    static InferenceEngine& getInstance() {
        static InferenceEngine instance;
        return instance;
    }

    ~InferenceEngine();

    // Inicializa el entorno Metal y carga los pesos iniciales (Pools de VRAM)
    void initialize();
    void shutdown();
    void finalizeBackend();
    void unloadAllModels();

    // Inferencia asíncrona optimizada para RDT (Recurrent-Depth Transformer)
    std::future<InferenceResponse> generate(const InferenceRequest& request);

    // Gestión de VRAM (M4 Pro Optimized)
    void allocatePool(const std::string& pool_name, size_t size_gb);
    void releasePool(const std::string& pool_name);

    // LLaMA/MLX Native Methods
    bool loadModel(const std::string& model_id, const std::string& model_path);
    bool loadDraftModel(const std::string& draft_model_path);
    void unloadModel(const std::string& model_id);
    bool applyLora(const std::string& model_id, const std::string& lora_path, float scale = 1.0f);
    bool evictOldestModel(const std::string& except_id = "");
    void loadAllModels(const std::string& directory_path);
    bool selectModel(const std::string& model_id);
    std::vector<std::string> getLoadedModels() const;
    int64_t getModelLastUsed(const std::string& model_id) const;

    struct NeuralMetrics {
        float last_tps = 0.0f;
        float avg_latency = 0.0f;
        int total_slots = 4;
        int active_slots = 0;
        bool flash_attn_enabled = true;
        float temperature_c = 0.0f;
        float vram_gb = 0.0f;
        std::string active_model;
    };
    NeuralMetrics getMetrics() const;

    // Test/Mocking methods
    void injectMockModel(const std::string& model_id, float vram_gb);

private:
    InferenceEngine() = default;

    enum class BackendType { NONE, MLX, LLAMA };
    BackendType getBackendForModel(const std::string& model_id) const;

    // Backend-specific implementations
    void initializeMLX();
    void initializeLlama();
    bool loadModelMLX(const std::string& model_id, const std::string& model_path);
    bool loadModelLlama(const std::string& model_id, const std::string& model_path);
    std::future<InferenceResponse> generateMLX(const InferenceRequest& request);
    std::future<InferenceResponse> generateLlama(const InferenceRequest& request);
    void unloadModelMLX(const std::string& model_id);
    void unloadModelLlama(const std::string& model_id);

    // Llama-specific helpers
    void allocatePoolLlama(const std::string& pool_name, size_t size_gb);
    void releasePoolLlama(const std::string& pool_name);
    bool loadDraftModelLlama(const std::string& draft_model_path);
    bool applyLoraLlama(const std::string& model_id, const std::string& lora_path, float scale);
    bool evictOldestModelLlama(const std::string& except_id);

    std::recursive_mutex engine_mtx;
    NeuralMetrics last_metrics;

    // --- Unified State (Both backends can coexist on macOS) ---

    // MLX Native State
    void* mlx_plan = nullptr;
    std::map<std::string, void*> mlx_models; // Punteros a mlx::core::array o modelos

    // LLaMA Native State (Multi-Model Support)
    std::map<std::string, struct llama_model*> models;
    std::map<std::string, struct llama_context*> contexts;

    // Speculative Decoding (Draft Model)
    struct llama_model* draft_model = nullptr;
    struct llama_context* draft_ctx = nullptr;
    std::string draft_model_id;

    struct common_sampler* sampler = nullptr;

    // Backend tracking
    std::map<std::string, BackendType> model_backends;
    std::map<std::string, int64_t> model_last_used;
    std::string active_model_id;

    // Descriptores de Metal (Opaque types for C++ header)
    void* mtl_device = nullptr;
    void* mtl_command_queue = nullptr;
    std::map<std::string, void*> vram_pools;
};

} // namespace interlock
