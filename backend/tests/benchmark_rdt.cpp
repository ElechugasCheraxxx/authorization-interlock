#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include "InferenceEngine.hpp"
#include "GPUAllocator.hpp"

using namespace interlock;

struct BenchmarkResult {
    std::string model_id;
    int rdt_factor;
    float tokens_per_sec;
    float latency_ms;
    float vram_used_gb;
    float temperature_c;
};

void run_rdt_benchmark(const std::string& model_id, int rdt_factor, std::vector<BenchmarkResult>& results) {
    auto& engine = InferenceEngine::getInstance();
    auto& allocator = GPUAllocator::getInstance();

    std::cout << "[Benchmark] Testing Model: " << model_id << " | RDT Factor: " << rdt_factor << "x" << std::endl;

    InferenceRequest req;
    req.model_id = model_id;
    req.prompt = "Write a high-performance C++ kernel for neural recursion on Apple Silicon.";
    req.max_tokens = 128;
    req.temperature = 0.7f;
    req.sampling_params = {{"rdt_factor", rdt_factor}};

    auto start = std::chrono::high_resolution_clock::now();
    auto future = engine.generate(req);
    auto resp = future.get();
    auto end = std::chrono::high_resolution_clock::now();

    float duration_sec = std::chrono::duration<float>(end - start).count();

    // Use actual generated tokens and reported TPS from engine
    float tps = resp.tokens_per_second;

    auto metrics = allocator.getMetrics();

    results.push_back({
        model_id,
        rdt_factor,
        tps,
        resp.latency_ms,
        metrics.vram_reserved_gb,
        metrics.temperature_c
    });

    std::cout << "  >> Model: " << model_id << " | TPS: " << std::fixed << std::setprecision(2) << tps
              << ", Tokens: " << resp.tokens_generated << ", Latency: " << resp.latency_ms << "ms" << std::endl;

    if (resp.tokens_generated == 0) {
        std::cout << "  !! WARNING: Zero tokens generated. Check InferenceEngine loop." << std::endl;
    }
}

int main() {
    std::cout << "=== Interlock RDT Performance Benchmark (M4 Pro - NATIVE GGUF) ===" << std::endl;

    auto& engine = InferenceEngine::getInstance();
    engine.initialize();

    std::string model_dir = "/Users/Cheraxxx/Interlock IDE/data/models";
    std::cout << "[Benchmark] Loading all real GGUF models from: " << model_dir << std::endl;
    engine.loadAllModels(model_dir);

    auto loaded_models = engine.getLoadedModels();
    if (loaded_models.empty()) {
        std::cerr << "[Benchmark] CRITICAL: No real models found in " << model_dir << ". Aborting benchmark." << std::endl;
        return 1;
    }

    std::cout << "[Benchmark] Found " << loaded_models.size() << " models. Starting test suite..." << std::endl;

    std::vector<int> rdt_factors = {1, 4}; // Testing base and 4x recursion
    std::vector<BenchmarkResult> results;

    for (const auto& model_id : loaded_models) {
        for (int factor : rdt_factors) {
            run_rdt_benchmark(model_id, factor, results);
        }
    }

    std::cout << "\n" << std::string(85, '=') << std::endl;
    std::cout << std::left << std::setw(35) << "Model ID"
              << std::setw(12) << "RDT"
              << std::setw(10) << "TPS"
              << std::setw(12) << "Latency"
              << std::setw(10) << "VRAM"
              << "Temp" << std::endl;
    std::cout << std::string(85, '-') << std::endl;

    for (const auto& res : results) {
        std::cout << std::left << std::setw(35) << (res.model_id.length() > 32 ? res.model_id.substr(0, 30) + ".." : res.model_id)
                  << std::setw(12) << res.rdt_factor
                  << std::setw(10) << std::fixed << std::setprecision(2) << res.tokens_per_sec
                  << std::setw(12) << (std::to_string((int)res.latency_ms) + "ms")
                  << std::setw(10) << (std::to_string((int)res.vram_used_gb) + "GB")
                  << (std::to_string((int)res.temperature_c) + "C") << std::endl;
    }
    std::cout << std::string(85, '=') << std::endl;

    return 0;
}
