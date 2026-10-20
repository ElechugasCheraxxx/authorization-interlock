#include "DockerOrchestrator.hpp"
#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <sstream>
#include <algorithm>

namespace interlock {

static bool isValidDockerName(const std::string& name) {
    if (name.empty()) return false;
    for (char c : name) {
        if (!std::isalnum(c) && c != '_' && c != '.' && c != '-') return false;
    }
    return true;
}

static bool isValidDockerImage(const std::string& image) {
    if (image.empty()) return false;
    // Basic validation for image names: allow alphanumeric, dots, dashes, underscores, and colons/slashes for tags/repos
    for (char c : image) {
        if (!std::isalnum(c) && c != '_' && c != '.' && c != '-' && c != ':' && c != '/') return false;
    }
    return true;
}

static bool isValidEnvVar(const std::string& env) {
    // Validates KEY=VALUE where KEY is alphanumeric/underscore
    auto pos = env.find('=');
    if (pos == std::string::npos || pos == 0) return false;
    std::string key = env.substr(0, pos);
    for (char c : key) {
        if (!std::isalnum(c) && c != '_') return false;
    }
    // For value, we should be careful. A simple way is to reject shell metacharacters
    std::string value = env.substr(pos + 1);
    const std::string forbidden = ";|&><$`\\\"'";
    for (char c : value) {
        if (forbidden.find(c) != std::string::npos) return false;
    }
    return true;
}

bool DockerOrchestrator::createContainer(const ContainerConfig& config) {
    if (!isValidDockerName(config.name)) {
        std::cerr << "[DockerOrchestrator] Invalid container name: " << config.name << std::endl;
        return false;
    }
    if (!isValidDockerImage(config.image)) {
        std::cerr << "[DockerOrchestrator] Invalid image name: " << config.image << std::endl;
        return false;
    }

    std::stringstream cmd;
    cmd << "docker run -d --name " << config.name;

    for (const auto& env : config.env_vars) {
        if (!isValidEnvVar(env)) {
            std::cerr << "[DockerOrchestrator] Invalid environment variable: " << env << std::endl;
            continue;
        }
        cmd << " -e " << env;
    }

    if (config.use_asan) {
        cmd << " -e ASAN_OPTIONS=detect_leaks=1:symbolize=1";
    }

    cmd << " " << config.image << " tail -f /dev/null";

    std::cout << "[DockerOrchestrator] Spawning container: " << config.name << std::endl;
    std::string result = runDockerCLI(cmd.str());

    return !result.empty() && result.find("Error") == std::string::npos;
}

std::string DockerOrchestrator::executeCommand(const std::string& container_name, const std::string& command) {
    if (!isValidDockerName(container_name)) {
        return "Error: Invalid container name";
    }

    // To prevent host-side injection, we wrap the command and use a single argument for the container shell
    // However, popen still uses a host shell. The most robust way without a library is very strict validation
    // or escaping. For now, let's reject common host-side metacharacters in the command.
    const std::string host_forbidden = ";|&><$`\\\"'";
    for (char c : command) {
         if (host_forbidden.find(c) != std::string::npos) {
             return "Error: Command contains forbidden characters";
         }
    }

    std::string full_cmd = "docker exec " + container_name + " " + command;
    return runDockerCLI(full_cmd);
}

void DockerOrchestrator::removeContainer(const std::string& container_name) {
    if (!isValidDockerName(container_name)) {
        std::cerr << "[DockerOrchestrator] Aborting removal: Invalid container name" << std::endl;
        return;
    }
    std::cout << "[DockerOrchestrator] Cleaning up: " << container_name << std::endl;
    runDockerCLI("docker rm -f " + container_name);
}

std::string DockerOrchestrator::runDockerCLI(const std::string& command) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen((command + " 2>&1").c_str(), "r"), pclose);
    if (!pipe) {
        return "Error: popen failed";
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

} // namespace interlock
