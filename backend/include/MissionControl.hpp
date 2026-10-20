#pragma once

#include "Schema.hpp"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace interlock {

struct Mission {
    std::string id;
    std::string objective;
    int priority = 1;
    std::vector<std::string> constraints;
    std::chrono::system_clock::time_point deadline;
    bool autonomous = true;
};

struct MissionReport {
    std::string mission_id;
    bool success;
    std::string summary;
    std::vector<std::string> artifacts;
    float risk_score;
};

/**
 * Overnight Mission Control
 * Handles long-running autonomous operations while the operator is away.
 * Orchestrates agents, manages resource duty cycles, and ensures integrity (OpSec).
 */
class MissionControl {
public:
    static MissionControl& getInstance() {
        static MissionControl instance;
        return instance;
    }

    void start();
    void stop();

    // Mission Management
    std::string submitMission(const Mission& mission);
    void cancelMission(const std::string& mission_id);
    MissionReport getReport(const std::string& mission_id);

    // Overnight Mode
    void enableOvernightMode(bool enable);
    bool isOvernightModeActive() const { return overnight_mode; }

    /**
     * @brief Ported from referencia previa VS2010ProjectGenerator.
     * Automates the generation of build environments for Windows-based agents.
     */
    void generateBuildEnvironment(const std::string& project_name, const std::string& target_os);

    // Sovereign Swarm Mission (only .chrxxx blocks)
    void executeSovereignSwarm(const std::string& chrxSource);

private:
    MissionControl();
    ~MissionControl();

    void missionProcessorLoop();
    void processMission(const Mission& mission);

    std::atomic<bool> running{false};
    std::atomic<bool> overnight_mode{false};

    std::thread processor_thread;
    std::queue<Mission> mission_queue;
    std::mutex queue_mtx;
    std::condition_variable cv;

    std::map<std::string, MissionReport> reports;
    std::mutex reports_mtx;
};

} // namespace interlock
