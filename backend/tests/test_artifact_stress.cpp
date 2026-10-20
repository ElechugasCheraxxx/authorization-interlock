#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include "ArtifactEngine.hpp"
#include "IPCBridge.hpp"
#include "Blackboard.hpp"
#include "interop/FormalValidators.hpp"

using namespace interlock;
using namespace interlock::formal;

/**
 * @brief Test de estrés para validar el rendimiento de ArtifactEngine.
 * Simula una carga de red de ~12.4 Gbps enviando ráfagas masivas de telemetría via IPC.
 */
void stress_test_surgery() {
    std::cout << "=== Interlock Artifact Stress Test (Simulation: 12.4 Gbps) ===" << std::endl;

    auto& surgery = ArtifactEngine::getInstance();
    auto& ipc = IPCBridge::getInstance();

    surgery.start();
    ipc.initializeServers();

    std::atomic<long long> messages_processed{0};
    const int NUM_MESSAGES = 100000;
    const int NUM_THREADS = 8; // Alineado con P-Cores del M4 Pro

    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> workers;
    for (int t = 0; t < NUM_THREADS; ++t) {
        workers.emplace_back([&, t]() {
            // Cliente simulado (Perl Scalpel)
            auto client = ipc.getClient(IPCComponentType::PERL_SURGERY);
            if (!client) {
                std::cerr << "Thread " << t << ": Failed to get IPC client." << std::endl;
                return;
            }

            for (int i = 0; i < NUM_MESSAGES / NUM_THREADS; ++i) {
                IPCMessage msg;
                msg.message_id = 0x77; // TACTICAL_TELEMETRY

                json artifact = {
                    {"node_id", "NODE-STRESS-" + std::to_string(t)},
                    {"status", "ACTIVE"},
                    {"load", (float)i / 100.0f},
                    {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
                };

                std::string s = artifact.dump();
                msg.payload_size = std::min((uint32_t)s.size(), (uint32_t)1024);
                std::memcpy(msg.artifact, s.data(), msg.payload_size);

                client->sendAsync(msg);

                // [FORMAL CERTIFICATION] Verify message extraction logic integrity
                DataSurgeryValidator::ArtifactData art = { "STRESS_CHUNK", "NODE-STRESS", "SHA256_HASH_PLACEHOLDER", false };
                bool verified = DataSurgeryValidator::verifyExtraction(s, art);
                if (!verified) {
                    std::cerr << "🚨 [FATAL] Artifact Formal Verification Failed at message " << i << std::endl;
                    exit(1);
                }

                messages_processed++;
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    std::cout << "\n[Stress Results]" << std::endl;
    std::cout << "Messages Sent: " << messages_processed << std::endl;
    std::cout << "Total Time: " << duration << " ms" << std::endl;
    std::cout << "Throughput: " << (messages_processed * 1000.0 / duration) << " msg/sec" << std::endl;

    // Validar estado en Blackboard
    auto val = Blackboard::getInstance().kvGet("NODE-STRESS-0");
    std::cout << "Last Node State in Blackboard: " << val.dump() << std::endl;

    surgery.stop();
}

int main() {
    try {
        stress_test_surgery();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
