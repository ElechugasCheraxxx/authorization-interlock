#include "Minimizer.hpp"
#include "ToolDispatcher.hpp"
#include "interop/AtlasSwarmClient.hpp"
#include <iostream>

namespace interlock {

WraithAgent::WraithAgent(const std::string& model_path)
    : NativeAgent("wraith", model_path) {}

void WraithAgent::onResponseGenerated(const std::string& raw_output) {
    // Minimizer: Evasión, mimetismo administrativo y Cognitive OPSEC.
    json data = ToolDispatcher::getInstance().parseGBNFResponse(raw_output);

    if (data.count("evasion_tactic")) {
        std::string tactic = data["evasion_tactic"];
        std::cout << "[Minimizer] Executing Evasion Tactic: " << tactic << std::endl;

        try {
            network::AtlasSwarmClient client("/tmp/interlock_atlas.sock");
            client.route_message("wraith", "atlas", "MISSION_UPDATE", "EVASION_ACTIVE:" + tactic);
        } catch (...) {}
    }
}

} // namespace interlock
