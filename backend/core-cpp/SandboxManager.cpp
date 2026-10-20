#include "SandboxManager.hpp"
#include <iostream>
#include <chrono>
#include <regex>

namespace interlock {

bool SandboxManager::prepareEnvironment(const std::string& env_id, const std::string& docker_image) {
    std::cout << "[SandboxManager] Spawning isolated environment: " << env_id << " using " << docker_image << std::endl;
    // In production, this would invoke 'docker run' or a custom hypervisor API.
    return true;
}

SandboxManager::ExecutionResult SandboxManager::run(const std::string& env_id, const std::string& command, const std::string& input_data) {
    auto start = std::chrono::high_resolution_clock::now();

    ExecutionResult result;
    result.exit_code = 0;
    result.stdout_log = "Execution successful.";
    result.stderr_log = ""; // No errors by default

    // Simulating ASan detection logic
    if (command.find("overflow") != std::string::npos) {
        result.detected_crash = true;
        result.stderr_log = "==1234==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x602000000010";
        result.crash_report = parseSanitizerOutput(result.stderr_log);
    } else {
        result.detected_crash = false;
    }

    auto end = std::chrono::high_resolution_clock::now();
    result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

    return result;
}

void SandboxManager::teardown(const std::string& env_id) {
    std::cout << "[SandboxManager] Tearing down environment: " << env_id << std::endl;
}

std::string SandboxManager::parseSanitizerOutput(const std::string& raw_stderr) {
    if (raw_stderr.find("AddressSanitizer") != std::string::npos) {
        return "ASan detected Memory Corruption";
    }
    if (raw_stderr.find("UndefinedBehaviorSanitizer") != std::string::npos) {
        return "UBSan detected Undefined Behavior";
    }
    return "Unknown Crash";
}

} // namespace interlock
