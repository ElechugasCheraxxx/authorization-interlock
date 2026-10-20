#include "InferenceEngine.hpp"
#include "GPUAllocator.hpp"
#include "ContextManager.hpp"
#include "GrammarLibrary.hpp"
#include "ToolDispatcher.hpp"
#include "mach/MachPriority.hpp"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

// MLX Headers
#include <mlx/mlx.h>
#include <mlx/backend/metal/metal.h>

#include <iostream>
#include <chrono>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;
namespace mx = mlx::core;

namespace interlock {

void InferenceEngine::initializeMLX() {
    @autoreleasepool {
        std::cout << "[InferenceEngine] Initializing MLX Backend on M4 Pro (Unified Memory Optimized)..." << std::endl;

        try {
            // Configuración agresiva de cache para los 48GB del M4 Pro
            // Dejamos 6GB para el sistema/UI.
            mx::set_cache_limit(42 * 1024 * 1024 * 1024LL);

            auto device = mx::default_device();
            std::cout << "[InferenceEngine] MLX Default Device: " << device << " Ready." << std::endl;
            std::cout << "[InferenceEngine] MLX VRAM Cache limit set to 42GB." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[InferenceEngine] Failed to initialize MLX: " << e.what() << std::endl;
        }
    }
}

bool InferenceEngine::loadModelMLX(const std::string& model_id, const std::string& model_path) {
    if (mlx_models.find(model_id) != mlx_models.end()) {
        active_model_id = model_id;
        return true;
    }

    std::cout << "[InferenceEngine] MLX Loading (Safetensors/Weights): " << model_path << std::endl;

    try {
        // En MLX, mx::load_safetensors devuelve un SafetensorsLoad (par de mapas: pesos y metadatos)
        auto load_result = mx::load_safetensors(model_path);
        auto& weights = load_result.first;

        // Evaluamos para asegurar que los tensores se cargan físicamente en la memoria unificada del M4 Pro
        for (auto& [name, arr] : weights) {
            mx::eval(arr);
        }

        // Guardamos los pesos en nuestro mapa de modelos
        auto* weights_ptr = new std::map<std::string, mx::array>(weights);
        mlx_models[model_id] = (void*)weights_ptr;

        model_last_used[model_id] = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        model_backends[model_id] = BackendType::MLX;

        if (active_model_id.empty()) active_model_id = model_id;

        std::cout << "[InferenceEngine] MLX Model [" << model_id << "] initialized and cached in UMA." << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[InferenceEngine] MLX Load Failed for [" << model_id << "]: " << e.what() << std::endl;
        return false;
    }
}

void InferenceEngine::unloadModelMLX(const std::string& model_id) {
    auto it = mlx_models.find(model_id);
    if (it != mlx_models.end()) {
        delete (std::map<std::string, mx::array>*)it->second;
        mlx_models.erase(it);
        model_backends.erase(model_id);
        std::cout << "[InferenceEngine] MLX Model [" << model_id << "] unloaded." << std::endl;
    }
}

std::future<InferenceResponse> InferenceEngine::generateMLX(const InferenceRequest& request) {
    return std::async(std::launch::async, [this, request]() -> InferenceResponse {
        // Prioridad máxima para los P-Cores del M4 Pro
        mach::SetThreadPriority(mach_thread_self(), mach::TaskPriority::HIGH);

        auto start = std::chrono::high_resolution_clock::now();
        std::string target_id = request.model_id.empty() ? active_model_id : request.model_id;

        if (mlx_models.find(target_id) == mlx_models.end()) {
             return InferenceResponse{"Model not loaded in MLX", 0, 0.0f, 0.0f, "error"};
        }

        // --- MLX Inference Bridge ---
        // MLX utiliza grafos dinámicos. Aquí se dispararían los kernels de Metal.
        // Simulamos la generación para validación de arquitectura.

        std::string result_text = "[MLX_UMA_ENGINE] Ejecutando inferencia nativa en M4 Pro...";

        // Simulación de delay de cómputo GPU (MLX es ~2x-3x más rápido en batching que llama.cpp en Apple)
        std::this_thread::sleep_for(std::chrono::milliseconds(15));

        auto end = std::chrono::high_resolution_clock::now();
        float latency = std::chrono::duration<float, std::milli>(end - start).count();

        InferenceResponse response;
        response.text = result_text;
        response.tokens_generated = 32; // Mock
        response.latency_ms = latency;
        response.tokens_per_second = (response.tokens_generated / (latency / 1000.0f));
        response.finish_reason = "stop";

        // Update metrics for telemetry
        {
            std::lock_guard<std::recursive_mutex> lock(engine_mtx);
            last_metrics.last_tps = response.tokens_per_second;
            last_metrics.avg_latency = response.latency_ms / 32.0f;
            last_metrics.active_model = target_id;
        }

        // Dispatch de herramientas (Sync con el resto del sistema de agentes)
        ToolDispatcher::getInstance().dispatchActions(target_id, response.text);

        return response;
    });
}

} // namespace interlock
