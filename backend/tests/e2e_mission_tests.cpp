#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include "Blackboard.hpp"
#include "GPUAllocator.hpp"
#include "InferenceEngine.hpp"
#include "CognitiveEngines.hpp"
#include "InterlockCore.h"
#include "EventSystem.hpp"
#include "interop/FormalValidators.hpp"

using namespace interlock;
using namespace interlock::formal;

#define TEST(name) void name()
#define RUN_TEST(name) \
    std::cout << "[RUNNING] " << #name << "..." << std::endl; \
    name(); \
    std::cout << "[PASSED] " << #name << std::endl;

TEST(test_e2e_multi_model_loading) {
    std::cout << "--- Starting Native E2E Multi-Model Loading Test ---" << std::endl;

    auto& inference = InferenceEngine::getInstance();
    inference.initialize();

    // Carga masiva de todos los GGUF detectados
    std::string model_dir = "/Users/Cheraxxx/Interlock IDE/data/models";
    inference.loadAllModels(model_dir);

    // En un entorno real, verificaríamos cuántos se cargaron.
    // Por ahora, validamos que el motor sigue estable.

    // Intentar una inferencia con el primer modelo cargado (o el activo por defecto)
    InferenceRequest req;
    req.prompt = "User: Hello Interlock. System: Hi. How can I assist in your Assessment mission today?";
    req.max_tokens = 32;
    req.sampling_params = {{"rdt_factor", 1}};

    std::cout << "[Inference] Testing generation with active model..." << std::endl;

    // [FORMAL CERTIFICATION] Verify model route reliability
    bool route_ok = MessageRouterValidator::isValidRoute("User", "InferenceEngine", "default");
    assert(route_ok && "Formal Routing Verification Failed");

    auto future = inference.generate(req);
    auto resp = future.get();

    std::cout << "[Response] " << resp.text << std::endl;
    assert(!resp.text.empty());

    // New Interop Performance Metric
    double overhead = InterlockCore::benchmarkInterop(1000000);
    std::cout << "[Performance] Swift-C++ Interop Latency (approx): " << overhead << " microseconds/call" << std::endl;

    std::cout << "--- Multi-Model Loading Test Completed ---" << std::endl;
}

TEST(test_stress_maintenance_loop) {
    std::cout << "--- Starting Stress Test: Maintenance Loop & Concurrency ---" << std::endl;

    // Use boot() to start the core and maintenance thread
    InterlockCore::boot();

    // Simulate heavy load by injecting many events and checking VRAM metrics
    for (int i = 0; i < 50; ++i) {
        std::string msg = "Stress Message " + std::to_string(i);
        InterlockCore::injectMessage("system", msg.c_str());

        auto metrics = GPUAllocator::getInstance().getMetrics();
        if (i % 10 == 0) {
            std::cout << "[Stress] Iteration " << i << " - Temp: " << metrics.temperature_c << "C, VRAM Free: " << metrics.vram_free_gb << "GB" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "--- Stress Test Completed ---" << std::endl;
}

int main() {
    std::cout << "=== Interlock Multi-Model E2E Test (M4 Pro Unified Memory) ===" << std::endl;

    try {
        RUN_TEST(test_e2e_multi_model_loading);
        RUN_TEST(test_stress_maintenance_loop);
        std::cout << "\nALL TESTS PASSED!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "\nFATAL ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
