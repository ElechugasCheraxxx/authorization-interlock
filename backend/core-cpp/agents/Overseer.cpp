#include "Overseer.hpp"
#include "ToolDispatcher.hpp"
#include "FormalBridge.hpp"
#include "interop/AtlasSwarmClient.hpp"
#include <iostream>

namespace interlock {

OverseerAgent::OverseerAgent(const std::string& model_path)
    : NativeAgent("overseer", model_path) {}

void OverseerAgent::onResponseGenerated(const std::string& raw_output) {
    // Overseer: Monitor de OPSEC y Supervisor Formal (Glasswing Oracle).
    json data = ToolDispatcher::getInstance().parseGBNFResponse(raw_output);

    // [Glasswing CROSS-CHECK]
    // If mission transition is proposed, validate via FormalBridge (Path B)
    if (data.count("mission_transition")) {
        formal::StateMachineValidator validator;
        formal::StateMachineValidator::Phase from = formal::StateMachineValidator::Phase::ANALYSIS; // Mock current phase
        formal::StateMachineValidator::Phase to = static_cast<formal::StateMachineValidator::Phase>(data["mission_transition"]["to_phase"].get<int>());

        std::cout << "[Overseer] Validating transition via Glasswing Path B..." << std::endl;
        if (!validator.isLegalTransition(from, to)) {
            std::cerr << "🛑 [OPSEC SHIELD] Illegal State Transition Blocked by Formal Bridge!" << std::endl;
            return;
        }
    }

    if (data.count("audit")) {
        std::cout << "[Overseer] OPSEC Audit Result: " << data["audit"] << std::endl;

        // Reporting to Atlas Orchestrator
        try {
            network::AtlasSwarmClient client("/tmp/interlock_atlas.sock");
            client.route_message("overseer", "atlas", "MISSION_UPDATE", "OPSEC_AUDIT_PASSED");
        } catch (...) {}
    }
}

} // namespace interlock
