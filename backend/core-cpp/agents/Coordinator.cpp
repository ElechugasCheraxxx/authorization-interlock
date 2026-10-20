#include "Coordinator.hpp"
#include "ToolDispatcher.hpp"
#include "Blackboard.hpp"
#include "interop/AtlasSwarmClient.hpp"
#include <iostream>

namespace interlock {

RexAgent::RexAgent(const std::string& model_path)
    : NativeAgent("rex", model_path) {

    std::cout << "[Coordinator] Connecting to Atlas Swarm..." << std::endl;
    try {
        network::AtlasSwarmClient client("/tmp/interlock_atlas.sock");
        client.register_agent("rex_01", "macOS", "arm64");
    } catch (...) {}
}

void RexAgent::onResponseGenerated(const std::string& raw_output) {
    // Coordinator: Active Leader. Coordina la ejecución agresiva.
    json data = ToolDispatcher::getInstance().parseGBNFResponse(raw_output);

    // Report reasoning to Atlas
    try {
        network::AtlasSwarmClient client("/tmp/interlock_atlas.sock");
        client.route_message("rex", "atlas", "REASONING_OUTPUT", raw_output);
    } catch (...) {}

    if (data.count("command")) {
        std::string cmd = data["command"];
        std::cout << "[Coordinator] Leading active action: " << cmd << std::endl;

        // Notify mission update
        try {
            network::AtlasSwarmClient client("/tmp/interlock_atlas.sock");
            client.route_message("rex", "atlas", "MISSION_UPDATE", "OFFENSIVE_LEAD_COMMAND:" + cmd);
        } catch (...) {}
    }
}

} // namespace interlock
