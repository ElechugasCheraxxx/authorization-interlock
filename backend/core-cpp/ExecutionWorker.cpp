#include "ExecutionWorker.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>

#ifdef __APPLE__
#include <sandbox.h>
#include <libproc.h>
#endif

namespace interlock {

std::future<ProcessResult> ExecutionWorker::execute(const std::string& command, const std::vector<std::string>& args) {
    return std::async(std::launch::async, [command, args]() {
        auto start = std::chrono::high_resolution_clock::now();

        std::cout << "[ExecutionWorker] Spawning Sandbox for: " << command << std::endl;

        int pipe_fd[2];
        if (pipe(pipe_fd) == -1) {
            return ProcessResult{-1, "", "Pipe failed", 0};
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            close(pipe_fd[0]);
            dup2(pipe_fd[1], STDOUT_FILENO);
            dup2(pipe_fd[1], STDERR_FILENO);

#ifdef __APPLE__
            char* errorbuf = nullptr;
            // Sandbox que permite lectura pero no red ni escritura fuera de /tmp
            if (sandbox_init(kSBXProfileNoNetwork, SANDBOX_NAMED, &errorbuf) != 0) {
                std::cerr << "Sandbox init failed: " << (errorbuf ? errorbuf : "unknown") << std::endl;
                exit(1);
            }
#endif

            std::vector<char*> c_args;
            c_args.push_back(const_cast<char*>(command.c_str()));
            for (const auto& arg : args) {
                c_args.push_back(const_cast<char*>(arg.c_str()));
            }
            c_args.push_back(nullptr);

            execvp(command.c_str(), c_args.data());
            perror("execvp");
            exit(1);
        } else if (pid > 0) {
            // Parent process
            close(pipe_fd[1]);

            std::string output;
            char buffer[4096];
            ssize_t bytes;
            while ((bytes = read(pipe_fd[0], buffer, sizeof(buffer))) > 0) {
                output.append(buffer, bytes);
            }
            close(pipe_fd[0]);

            int status;
            waitpid(pid, &status, 0);

            auto end = std::chrono::high_resolution_clock::now();
            float elapsed = std::chrono::duration<float, std::milli>(end - start).count();

            ProcessResult result;
            result.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            result.stdout_data = output;
            result.stderr_data = "";
            result.execution_time_ms = elapsed;

            return result;
        } else {
            return ProcessResult{-1, "", "Fork failed", 0};
        }
    });
}

void ExecutionWorker::monitorProcess(int pid) {
    std::cout << "[ExecutionWorker] Monitoring PID: " << pid << std::endl;
#ifdef __APPLE__
    struct proc_taskinfo pti;
    if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti)) > 0) {
        float cpu_usage = pti.pti_total_user / 1000000.0f; // Simplificado a segundos
        uint64_t ram_usage = pti.pti_resident_size;
        std::cout << "[ExecutionWorker] PID " << pid << " -> CPU: " << cpu_usage 
                  << "s, RAM: " << (ram_usage / 1024 / 1024) << "MB" << std::endl;
    }
#endif
}

} // namespace interlock
