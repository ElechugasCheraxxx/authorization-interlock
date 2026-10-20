#include "Apex.hpp"
#include "ToolDispatcher.hpp"
#include "interop/AtlasSwarmClient.hpp"
#include <iostream>

namespace interlock {

ApexAgent::ApexAgent(const std::string& model_path)
    : NativeAgent("apex", model_path) {}

void ApexAgent::onResponseGenerated(const std::string& raw_output) {
    // Apex: Technical Head. Validación técnica y síntesis de exploits.
    json data = ToolDispatcher::getInstance().parseGBNFResponse(raw_output);

    if (data.count("exploit_synthesis")) {
        std::cout << "[Apex] Synthesizing active primitive..." << std::endl;

        try {
            network::AtlasSwarmClient client("/tmp/interlock_atlas.sock");
            client.route_message("apex", "atlas", "MISSION_UPDATE", "EXPLOIT_SYNTHESIS_COMPLETED");
        } catch (...) {}
    }
}

} // namespace interlock
