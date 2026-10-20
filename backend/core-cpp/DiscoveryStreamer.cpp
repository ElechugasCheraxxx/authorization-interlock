#include "../include/DiscoveryStreamer.hpp"
#include <chrono>
#include <iostream>

namespace interlock::rag {

DiscoveryStreamer::DiscoveryStreamer() : running(false) {}

void DiscoveryStreamer::start() {
    if (running) return;
    running = true;
    worker_thread = std::thread(&DiscoveryStreamer::workerLoop, this);
}

void DiscoveryStreamer::stop() {
    running = false;
    cv.notify_all();
    if (worker_thread.joinable()) {
        worker_thread.join();
    }
}

void DiscoveryStreamer::requestLoad(uint64_t asset_id, uint32_t priority) {
    std::lock_guard<std::mutex> lock(mtx);
    load_queue.push({priority, asset_id});
    cv.notify_one();
}

void DiscoveryStreamer::workerLoop() {
    while (running) {
        uint64_t asset_to_load = 0;
        uint32_t priority = 0;

        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return !load_queue.empty() || !running; });

            if (!running) break;

            auto item = load_queue.top();
            priority = item.first;
            asset_to_load = item.second;
            load_queue.pop();
        }

        // Simulación de carga (inspirada en CStreaming::RequestModel)
        size_t asset_size = 0;
        void* data = loadFromDisk(asset_to_load, asset_size);

        if (data) {
            std::lock_guard<std::mutex> lock(mtx);
            DiscoveryAsset asset;
            asset.asset_id = asset_to_load;
            asset.priority = priority;
            asset.data_ptr = data;
            asset.size = asset_size;
            asset.is_loaded = true;
            loaded_assets.push_back(asset);

            std::cout << "[+] DiscoveryStreamer: Asset " << asset_to_load << " loaded (" << asset_size << " bytes)" << std::endl;
        }
    }
}

void* DiscoveryStreamer::loadFromDisk(uint64_t asset_id, size_t& size_out) {
    // Aquí se integraría con el Scalpel (Data Surgery) para extraer fragmentos.
    // Por ahora, simulamos una asignación en el SovereignHeap.
    size_out = 1024 * 64; // 64KB fragment
    void* ptr = nullptr;

    // Usamos el SovereignHeap para asegurar que estos fragmentos son reubicables
    memory::SovereignHeap::getInstance().malloc_registered(size_out, &ptr, memory::MemoryID::RAG_ENGINE);

    return ptr;
}

void DiscoveryStreamer::enforceMemoryLimit(size_t limit_bytes) {
    std::lock_guard<std::mutex> lock(mtx);
    // Lógica de "Least Important First" para liberar memoria (inspirada en re3)
    std::sort(loaded_assets.begin(), loaded_assets.end(), [](const DiscoveryAsset& a, const DiscoveryAsset& b) {
        return a.priority < b.priority;
    });

    size_t current_usage = 0;
    for (const auto& asset : loaded_assets) current_usage += asset.size;

    auto it = loaded_assets.begin();
    while (current_usage > limit_bytes && it != loaded_assets.end()) {
        current_usage -= it->size;
        memory::SovereignHeap::getInstance().free_registered(it->data_ptr);
        it = loaded_assets.erase(it);
    }
}

} // namespace interlock::rag
