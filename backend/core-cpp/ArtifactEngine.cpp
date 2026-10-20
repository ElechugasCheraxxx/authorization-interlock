#include "ArtifactEngine.hpp"
#include "mach/MachServer.hpp"
#include "Blackboard.hpp"
#include "EventSystem.hpp"
#include "IPCBridge.hpp"
#include "interop/FormalValidators.hpp"
#include <iostream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifdef __APPLE__
#include <sys/disk.h>
#endif

namespace interlock {

ArtifactEngine::ArtifactEngine() : running_(false), current_device_fd_(-1) {}

ArtifactEngine::~ArtifactEngine() {
    stop();
}

ArtifactEngine& ArtifactEngine::getInstance() {
    static ArtifactEngine instance;
    return instance;
}

void ArtifactEngine::start() {
    if (running_) return;
    running_ = true;

    // Start Asynchronous Surgery Worker (Inspired by CdStreamThread)
    worker_thread_ = std::thread(&ArtifactEngine::surgeryWorkerLoop, this);

    // Registrar handler en el IPCBridge para mensajes de Perl/Go
    IPCBridge::getInstance().registerGlobalHandler(
        IPCComponentType::PERL_SURGERY,
        0x77, // TACTICAL_TELEMETRY
        [this](const IPCMessage& msg) {
            std::string raw_data(reinterpret_cast<const char*>(msg.artifact), msg.payload_size);
            this->processIncomingTacticalData(raw_data);
            return IPCMessage{}; // Empty response
        }
    );

    std::cout << "[Artifact] Engine Online. High-speed log parsing & Raw Sector Surgery active." << std::endl;
}

void ArtifactEngine::stop() {
    if (!running_) return;
    running_ = false;

    queue_cv_.notify_all();
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }

    if (current_device_fd_ != -1) {
        close(current_device_fd_);
        current_device_fd_ = -1;
    }
}

int ArtifactEngine::openRawDevice(const std::string& device_path) {
    std::lock_guard<std::mutex> lock(queue_mtx_);
    if (current_device_fd_ != -1) close(current_device_fd_);

    // O_RDONLY for safety during forensics, O_NONBLOCK to avoid hanging on hardware failure
    current_device_fd_ = open(device_path.c_str(), O_RDONLY | O_NONBLOCK);
    if (current_device_fd_ < 0) {
        perror("[Artifact] Failed to open raw device");
        return -1;
    }

    std::cout << "[Artifact] Raw device opened: " << device_path << " (fd: " << current_device_fd_ << ")" << std::endl;
    return current_device_fd_;
}

void ArtifactEngine::readSectorsAsync(int device_id, uint32_t start_sector, uint32_t count, void* target_buffer, std::atomic<bool>& out_flag) {
    out_flag = false;
    SectorRequest req;
    req.sector_start = start_sector;
    req.count = count;
    req.buffer = target_buffer;
    req.completion_flag = &out_flag;

    {
        std::lock_guard<std::mutex> lock(queue_mtx_);
        request_queue_.push_back(req);
    }
    queue_cv_.notify_one();
}

void ArtifactEngine::syncDevice(int device_id) {
    // Wait for queue to drain (CdStreamSync equivalent)
    while (true) {
        {
            std::lock_guard<std::mutex> lock(queue_mtx_);
            if (request_queue_.empty()) break;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
}

void ArtifactEngine::surgeryWorkerLoop() {
    const size_t SECTOR_SIZE = 512; // Standard sector size

    while (running_) {
        SectorRequest req;
        {
            std::unique_lock<std::mutex> lock(queue_mtx_);
            queue_cv_.wait(lock, [this] { return !request_queue_.empty() || !running_; });
            if (!running_) break;
            req = request_queue_.front();
            request_queue_.pop_front();
        }

        if (current_device_fd_ != -1) {
            off_t offset = (off_t)req.sector_start * SECTOR_SIZE;
            if (lseek(current_device_fd_, offset, SEEK_SET) != -1) {
                ssize_t bytes_to_read = req.count * SECTOR_SIZE;
                read(current_device_fd_, req.buffer, bytes_to_read);
            }
        }

        if (req.completion_flag) {
            req.completion_flag->store(true);
        }
    }
}

void ArtifactEngine::processIncomingTacticalData(const std::string& raw_data) {
    // 1. Parsing veloz (Surgery) de logs de Perl/Go
    // En producción, aquí se implementa el regex engine optimizado o el parser de binario

    try {
        auto j = json::parse(raw_data);

        // 2. Inyectar directamente en el Blackboard para que Qt lo vea inmediatamente
        if (j.count("node_id") && j.count("status")) {
            // --- FORMAL VERIFICATION LAYER (Coq Extract) ---
            // Verify artifact integrity before committing to Blackboard/EvidenceVault
            formal::DataSurgeryValidator::ArtifactData art = { j["node_id"], "tactical_source", "", false };
            if (!formal::DataSurgeryValidator::verifyExtraction(raw_data, art)) {
                std::cerr << "[Artifact] [FORMAL_ALARM] Artifact validation failed! Dropping corrupted node." << std::endl;
                return;
            }

            MapNode node;
            node.id = j["node_id"];
            node.type = "tactical_node";
            node.metadata = j;
            Blackboard::getInstance().addMapNode(node);
        }

        // 3. Emitir evento de telemetría para la UI
        Event ev;
        ev.type = "TELEMETRY_UPDATED";
        ev.data = j;
        EventBus::getInstance().publish(ev);

    } catch (...) {
        // Ignorar basura en el canal IPC para mantener estabilidad
    }
}

} // namespace interlock
