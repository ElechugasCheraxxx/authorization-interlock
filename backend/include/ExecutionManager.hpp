#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <functional>

namespace interlock {

struct ProcessInfo {
    int pid;
    std::string name;
    std::string status;
    float cpu_usage;
    float mem_usage_mb;
};

class ExecutionManager {
public:
    static ExecutionManager& getInstance() {
        static ExecutionManager instance;
        return instance;
    }

    // --- Process Supervision ---
    int spawnProcess(const std::string& command, const std::vector<std::string>& args, bool isolated = true);
    bool terminateProcess(int pid);
    std::vector<ProcessInfo> listProcesses();

    // --- JIT Task Execution ---
    // Ejecuta un comando y captura su salida estándar
    std::string executeAndCapture(const std::string& command);

    void executeTask(const std::string& agent_alias, const std::string& task_json);

    // --- Hardware-Aware Scheduling ---
    void setAffinity(int pid, int core_id);

private:
    ExecutionManager() = default;
    std::mutex mtx;
    std::map<int, std::string> active_tasks;
};

} // namespace interlock
