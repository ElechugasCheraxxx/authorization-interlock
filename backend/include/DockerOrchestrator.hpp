#pragma once

#include <string>
#include <vector>
#include <memory>

namespace interlock {

/**
 * @brief Manages ephemeral Docker containers for assess execution and validation.
 * Milestone 1.1 of GLASSWING.
 */
class DockerOrchestrator {
public:
    struct ContainerConfig {
        std::string image;
        std::string name;
        std::vector<std::string> env_vars;
        bool use_asan = true;
    };

    static DockerOrchestrator& getInstance() {
        static DockerOrchestrator instance;
        return instance;
    }

    /**
     * @brief Spawns a new container for a specific task.
     */
    bool createContainer(const ContainerConfig& config);

    /**
     * @brief Executes a command inside an active container.
     */
    std::string executeCommand(const std::string& container_name, const std::string& command);

    /**
     * @brief Removes a container and its associated resources.
     */
    void removeContainer(const std::string& container_name);

private:
    DockerOrchestrator() = default;

    // Internal helper to run shell commands (via popen or similar for Docker CLI)
    std::string runDockerCLI(const std::string& args);
};

} // namespace interlock
