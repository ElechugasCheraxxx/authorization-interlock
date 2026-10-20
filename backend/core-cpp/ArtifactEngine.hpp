#ifndef INTERLOCK_DATA_SURGERY_ENGINE_HPP
#define INTERLOCK_DATA_SURGERY_ENGINE_HPP

#include <string>
#include <vector>
#include <atomic>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace interlock {

struct SectorRequest {
    uint32_t sector_start;
    uint32_t count;
    void* buffer;
    std::atomic<bool>* completion_flag;
};

/**
 * @brief Engine de ultra-alto rendimiento para el procesamiento de telemetría táctica.
 * Inspirado en el CdStream de RenderWare para análisis de bajo nivel de sectores de disco.
 */
class ArtifactEngine {
public:
    static ArtifactEngine& getInstance();

    void start();
    void stop();

    // --- High-Speed Operations Data Processing ---
    void processIncomingTacticalData(const std::string& raw_data);

    // --- CdStream-inspired Raw Sector Surgery ---
    int openRawDevice(const std::string& device_path);
    void readSectorsAsync(int device_id, uint32_t start_sector, uint32_t count, void* target_buffer, std::atomic<bool>& out_flag);
    void syncDevice(int device_id);

private:
    ArtifactEngine();
    ~ArtifactEngine();

    void surgeryWorkerLoop();

    std::atomic<bool> running_;
    std::thread worker_thread_;

    // Asynchronous Queue for Sector Surgery
    std::deque<SectorRequest> request_queue_;
    std::mutex queue_mtx_;
    std::condition_variable queue_cv_;

    int current_device_fd_ = -1;
};

} // namespace interlock

#endif // INTERLOCK_DATA_SURGERY_ENGINE_HPP
