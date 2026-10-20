#include "InferenceEngine.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace interlock {

InferenceEngine::~InferenceEngine() {
    shutdown();
}

void InferenceEngine::initialize() {
    std::lock_guard<std::recursive_mutex> lock(engine_mtx);
#ifdef USE_LLAMA
    initializeLlama();
#endif
#ifdef USE_MLX
    initializeMLX();
#endif
}

void InferenceEngine::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(engine_mtx);
    unloadAllModels();
}

void InferenceEngine::finalizeBackend() {
    std::lock_guard<std::recursive_mutex> lock(engine_mtx);
    unloadAllModels();
#ifdef USE_LLAMA
    llama_backend_free();
#endif
}

void InferenceEngine::unloadAllModels() {
    std::lock_guard<std::recursive_mutex> lock(engine_mtx);

    std::vector<std::string> ids;
    for (auto const& [id, _] : model_backends) {
        ids.push_back(id);
    }

    for (const auto& id : ids) {
        unloadModel(id);
    }

    active_model_id = "";
}

bool InferenceEngine::loadModel(const std::string& model_id, const std::string& model_path) {
    std::lock_guard<std::recursive_mutex> lock(engine_mtx);

    fs::path p(model_path);
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".safetensors" || ext == ".bin") {
#ifdef USE_MLX
        return loadModelMLX(model_id, model_path);
#else
        std::cerr << "[InferenceEngine] MLX backend not enabled for " << model_path << std::endl;
        return false;
#endif
    } else if (ext == ".gguf") {
#ifdef USE_LLAMA
        return loadModelLlama(model_id, model_path);
#else
        std::cerr << "[InferenceEngine] Llama backend not enabled for " << model_path << std::endl;
        return false;
#endif
    } else {
        std::cerr << "[InferenceEngine] Unknown model extension: " << ext << " for " << model_path << std::endl;
        return false;
    }
}

void InferenceEngine::unloadModel(const std::string& model_id) {
    std::lock_guard<std::recursive_mutex> lock(engine_mtx);
    auto it = model_backends.find(model_id);
    if (it == model_backends.end()) return;

    if (it->second == BackendType::MLX) {
#ifdef USE_MLX
        unloadModelMLX(model_id);
#endif
    } else if (it->second == BackendType::LLAMA) {
#ifdef USE_LLAMA
        unloadModelLlama(model_id);
#endif
    }
}

std::future<InferenceResponse> InferenceEngine::generate(const InferenceRequest& request) {
    std::lock_guard<std::recursive_mutex> lock(engine_mtx);
    std::string target_id = request.model_id.empty() ? active_model_id : request.model_id;

    auto it = model_backends.find(target_id);
    if (it == model_backends.end()) {
        std::promise<InferenceResponse> p;
        p.set_value(InferenceResponse{"Model not loaded", 0, 0.0f, 0.0f, "error"});
        return p.get_future();
    }

    if (it->second == BackendType::MLX) {
#ifdef USE_MLX
        return generateMLX(request);
#endif
    } else if (it->second == BackendType::LLAMA) {
#ifdef USE_LLAMA
        return generateLlama(request);
#endif
    }

    std::promise<InferenceResponse> p;
    p.set_value(InferenceResponse{"Backend mismatch", 0, 0.0f, 0.0f, "error"});
    return p.get_future();
}

InferenceEngine::BackendType InferenceEngine::getBackendForModel(const std::string& model_id) const {
    auto it = model_backends.find(model_id);
    return (it != model_backends.end()) ? it->second : BackendType::NONE;
}

void InferenceEngine::loadAllModels(const std::string& directory_path) {
    std::cout << "[InferenceEngine] Scanning directory for models: " << directory_path << std::endl;
    try {
        if (!fs::exists(directory_path)) return;
        for (const auto& entry : fs::directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".gguf" || ext == ".safetensors" || ext == ".bin") {
                    loadModel(entry.path().stem().string(), entry.path().string());
                }
            }
        }
    } catch (...) {}
}

bool InferenceEngine::selectModel(const std::string& model_id) {
    std::lock_guard<std::recursive_mutex> lock(engine_mtx);
    if (model_backends.count(model_id)) {
        active_model_id = model_id;
        return true;
    }
    return false;
}

std::vector<std::string> InferenceEngine::getLoadedModels() const {
    std::vector<std::string> loaded;
    for (auto const& [id, _] : model_backends) loaded.push_back(id);
    return loaded;
}

int64_t InferenceEngine::getModelLastUsed(const std::string& model_id) const {
    auto it = model_last_used.find(model_id);
    return (it != model_last_used.end()) ? it->second : 0;
}

void InferenceEngine::allocatePool(const std::string& pool_name, size_t size_gb) {
#ifdef USE_LLAMA
    allocatePoolLlama(pool_name, size_gb);
#endif
}

void InferenceEngine::releasePool(const std::string& pool_name) {
#ifdef USE_LLAMA
    releasePoolLlama(pool_name);
#endif
}

bool InferenceEngine::applyLora(const std::string& model_id, const std::string& lora_path, float scale) {
#ifdef USE_LLAMA
    if (getBackendForModel(model_id) == BackendType::LLAMA) {
        return applyLoraLlama(model_id, lora_path, scale);
    }
#endif
    return false;
}

bool InferenceEngine::evictOldestModel(const std::string& except_id) {
#ifdef USE_LLAMA
    return evictOldestModelLlama(except_id);
#endif
    return false;
}

bool InferenceEngine::loadDraftModel(const std::string& draft_model_path) {
#ifdef USE_LLAMA
    return loadDraftModelLlama(draft_model_path);
#endif
    return false;
}

void InferenceEngine::injectMockModel(const std::string& model_id, float vram_gb) {
    std::lock_guard<std::recursive_mutex> lock(engine_mtx);
    model_backends[model_id] = BackendType::NONE;
}

InferenceEngine::NeuralMetrics InferenceEngine::getMetrics() const {
    return last_metrics;
}

} // namespace interlock
