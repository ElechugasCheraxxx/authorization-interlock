#include "Probe.hpp"
#include "ToolDispatcher.hpp"
#include "Blackboard.hpp"
#include "interop/AtlasSwarmClient.hpp"
#include <iostream>

namespace interlock {

GhostAgent::GhostAgent(const std::string& model_path)
    : NativeAgent("ghost", model_path) {

    std::cout << "[Probe] Connecting to Atlas Swarm..." << std::endl;
    try {
        network::AtlasSwarmClient client("/tmp/interlock_atlas.sock");
        client.register_agent("ghost_01", "macOS", "arm64");
    } catch (...) {}
}

void GhostAgent::onResponseGenerated(const std::string& raw_output) {
    // Probe se encarga de la explotación y payloads usando extracción GBNF.
    json data = ToolDispatcher::getInstance().parseGBNFResponse(raw_output);

    // Report reasoning to Atlas
    try {
        network::AtlasSwarmClient client("/tmp/interlock_atlas.sock");
        client.route_message("ghost", "atlas", "REASONING_OUTPUT", raw_output);
    } catch (...) {}

    if (data.count("artifact")) {
        std::string artifact = data["artifact"];
        std::string method = data.count("execution") ? data["execution"] : "direct_exec";

        std::cout << "[Probe] Action: Deploying " << method << " artifact via Probe Remap..." << std::endl;

        // Notify mission update
        try {
            network::AtlasSwarmClient client("/tmp/interlock_atlas.sock");
            client.route_message("ghost", "atlas", "MISSION_UPDATE", "GHOST_REMAP_STARTED:" + method);
        } catch (...) {}

        if (method == "pipe") {
            std::string output = executeInMemory("bash", artifact);
            std::cout << "[Probe] Pipe Execution Output: " << output << std::endl;

            try {
                network::AtlasSwarmClient client("/tmp/interlock_atlas.sock");
                client.route_message("ghost", "atlas", "MISSION_RESULT", output);
            } catch (...) {}
        }
    }
}

} // namespace interlock
