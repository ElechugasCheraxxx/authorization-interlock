#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <future>
#include "Schema.hpp"

namespace interlock {

struct ProcessResult {
    int exit_code;
    std::string stdout_data;
    std::string stderr_data;
    float execution_time_ms;
};

class ExecutionWorker {
public:
    static ExecutionWorker& getInstance() {
        static ExecutionWorker instance;
        return instance;
    }

    // Ejecuta una herramienta en un entorno controlado (Sandboxing)
    std::future<ProcessResult> execute(const std::string& command, const std::vector<std::string>& args);

    // Monitoreo de salud del proceso (Exit Code tracking)
    void monitorProcess(int pid);

private:
    ExecutionWorker() = default;

    // Gestión de límites de recursos (M4 Pro cores)
    std::mutex worker_mtx;
    int max_parallel_jobs = 8;
};

} // namespace interlock
