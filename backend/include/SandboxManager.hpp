#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace interlock {

/**
 * @brief Manages isolated execution environments (Docker/Custom Sandboxes).
 * Instrumentated with ASan/UBSan for crash detection and assess validation.
 */
class SandboxManager {
public:
    struct ExecutionResult {
        int exit_code;
        std::string stdout_log;
        std::string stderr_log;
        bool detected_crash;
        std::string crash_report; // ASan/UBSan output
        double execution_time_ms;
    };

    static SandboxManager& getInstance() {
        static SandboxManager instance;
        return instance;
    }

    /**
     * @brief Prepares an isolated environment for a specific assess target.
     */
    bool prepareEnvironment(const std::string& env_id, const std::string& docker_image);

    /**
     * @brief Executes code within the sandbox and monitors for crashes.
     */
    ExecutionResult run(const std::string& env_id, const std::string& command, const std::string& input_data = "");

    /**
     * @brief Cleans up the environment.
     */
    void teardown(const std::string& env_id);

private:
    SandboxManager() = default;

    std::string parseSanitizerOutput(const std::string& raw_stderr);
};

} // namespace interlock
