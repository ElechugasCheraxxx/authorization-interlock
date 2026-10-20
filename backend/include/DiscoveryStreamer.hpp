/**
 * @file DiscoveryStreamer.hpp
 * @brief Motor de streaming asíncrono para el RAGEngine (M4 Pro Optimized)
 *
 * Basado en la lógica de prioridad de activos de re3/reVC.
 */

#pragma once

#include "SovereignHeap.hpp"
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace interlock::rag {

struct DiscoveryAsset {
    uint64_t asset_id;
    uint32_t priority;
    void* data_ptr;
    size_t size;
    bool is_loaded;
};

class DiscoveryStreamer {
public:
    static DiscoveryStreamer& getInstance() {
        static DiscoveryStreamer instance;
        return instance;
    }

    void start();
    void stop();

    /**
     * @brief Solicita la carga de un fragmento de inteligencia.
     * La prioridad determina el orden de carga (basado en el PDG).
     */
    void requestLoad(uint64_t asset_id, uint32_t priority);

    /**
     * @brief Libera activos menos utilizados si la VRAM/Heap está bajo presión.
     */
    void enforceMemoryLimit(size_t limit_bytes);

private:
    DiscoveryStreamer();

    std::priority_queue<std::pair<uint32_t, uint64_t>> load_queue;
    std::vector<DiscoveryAsset> loaded_assets;

    std::thread worker_thread;
    std::atomic<bool> running;
    std::mutex mtx;
    std::condition_variable cv;

    void workerLoop();
    void* loadFromDisk(uint64_t asset_id, size_t& size_out);
};

} // namespace interlock::rag
