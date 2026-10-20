#include "ExecutionManager.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <vector>

namespace interlock {

int ExecutionManager::spawnProcess(const std::string& command, const std::vector<std::string>& args, bool isolated) {
    std::lock_guard<std::mutex> lock(mtx);

    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        std::vector<char*> c_args;
        c_args.push_back(const_cast<char*>(command.c_str()));
        for (const auto& arg : args) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        execvp(command.c_str(), c_args.data());
        // If execvp returns, it failed
        std::cerr << "[ExecutionManager] Failed to exec: " << command << std::endl;
        exit(1);
    } else if (pid > 0) {
        // Parent process
        active_tasks[pid] = command;
        if (!isolated) {
            int status;
            waitpid(pid, &status, 0);
            active_tasks.erase(pid);
        }
        return pid;
    } else {
        std::cerr << "[ExecutionManager] Failed to fork" << std::endl;
        return -1;
    }
}

bool ExecutionManager::terminateProcess(int pid) {
    std::lock_guard<std::mutex> lock(mtx);
    if (kill(pid, SIGTERM) == 0) {
        active_tasks.erase(pid);
        return true;
    }
    return false;
}

std::vector<ProcessInfo> ExecutionManager::listProcesses() {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<ProcessInfo> list;
    for (auto const& [pid, name] : active_tasks) {
        list.push_back({pid, name, "RUNNING", 0.0f, 0.0f});
    }
    return list;
}

std::string ExecutionManager::executeAndCapture(const std::string& command) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

void ExecutionManager::executeTask(const std::string& agent_alias, const std::string& task_json) {
    // Implementation for Hito 4.x
}

void ExecutionManager::setAffinity(int pid, int core_id) {
    // Platform specific affinity setting
}

} // namespace interlock
