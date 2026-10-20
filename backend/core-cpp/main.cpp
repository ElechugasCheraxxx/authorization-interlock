#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include "MessageRouter.hpp"
#include "Blackboard.hpp"
#include "InferenceEngine.hpp"
#include "SessionManager.hpp"
#include "AutomationModule.hpp"
#include "RiskManager.hpp"
#include "ProcessManager.hpp"
#include "PythonBridge.hpp"
#include "GPUAllocator.hpp"
#include "GhidraBridge.hpp"
#include "MissionControl.hpp"
#include "interop/TrustedCoreFFI.h"
#include <nlohmann/json.hpp>
#include <csignal>
#include <atomic>
#include <fstream>

using namespace interlock;

std::atomic<bool> keep_running(true);
void signal_handler(int signal) {
    keep_running = false;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    bool init_only = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--init-only") {
            init_only = true;
        }
    }

    std::cout << "--- Interlock Assessment IDE : C2 Master Core ---" << std::endl;
    std::cout << "Hardware Target: Apple M4 Pro (Unified Memory Optimized)" << std::endl;

    // 0. Initialize Trusted Core (OCaml Runtime)
    interlock::init_ocaml_runtime(argv);

    // 1. Start Services (Postgres, etc.)
    // Esto ejecutará initdb y createdb si es la primera vez (instalación limpia)
    ProcessManager::getInstance().startServices();

    if (init_only) {
        std::cout << "[Core] Database and services initialized successfully." << std::endl;
        // En modo init-only no apagamos servicios para dejar el DB listo para el IDE
        return 0;
    }

    // 2. Initialize Hardware and Python Bridge
    auto& gpu = GPUAllocator::getInstance();
    auto& python = PythonBridge::getInstance();

    gpu.updateHardwareStats();
    auto metrics = gpu.getMetrics();
    std::cout << "[Core] M4 Pro Telemetry: " << metrics.vram_total_gb << "GB RAM | Temp: "
              << metrics.temperature_c << "°C" << std::endl;

    python.initialize();

    // 3. Initialize Engines
    auto& router = MessageRouter::getInstance();
    auto& inference = InferenceEngine::getInstance();

    inference.initialize();
    GhidraBridge::getInstance().initialize("/Applications/ghidra_11.0.3_PUBLIC", "/Users/Cheraxxx/InterlockProjects");
    router.start();

    // 4. Priming VRAM Pools
    if (!gpu.reserveVRAM("Strategic_Pool", 18.2f) || !gpu.reserveVRAM("Validation_Pool", 8.9f)) {
        std::cerr << "[Core] WARNING: High VRAM pressure during startup." << std::endl;
    }

    std::cout << "[Core] All engines online. Entering operational state." << std::endl;

    // --- SOVEREIGN SWARM MISSION TEST ---
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--test-swarm") {
            std::ifstream file("/Users/Cheraxxx/Interlock IDE/tests/sovereign_swarm_test.chrxxx");
            std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            interlock::MissionControl::getInstance().executeSovereignSwarm(content);
            return 0;
        }
    }

    // 5. Start Embedded Python Logic (Non-blocking)
    std::thread python_thread([]() {
        try {
            std::cout << "[Core] Python thread started. Launching core_main..." << std::endl;
            PythonBridge::getInstance().executeString("import interlock.core_main as main; main.run_core()");
            std::cout << "[Core] Python core_main execution finished." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[PythonBridge] Failed to launch core_main: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[PythonBridge] Failed to launch core_main with unknown error." << std::endl;
        }
    });
    python_thread.detach();

    // 6. Main Event Loop
    while (keep_running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        gpu.updateHardwareStats();

        // Verificamos Risk Status (sin llamar a initialize() inexistente)
        auto risk = RiskManager::getInstance().getStatus();
        if (risk.level == RiskLevel::CRITICAL) {
            std::cerr << "[Core] CRITICAL OPSEC DEVIATION. Shutting down." << std::endl;
            break;
        }
    }

    std::cout << "[Core] Graceful shutdown initiated..." << std::endl;
    router.stop();
    ProcessManager::getInstance().stopServices();
    return 0;
}
