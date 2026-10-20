#include "NativeAgent.hpp"
#include "Blackboard.hpp"
#include <iostream>

namespace interlock {

/**
 * Common Native Actions used by all agents to update the Assessment context
 */
void updateBlackboardWithFinding(const std::string& agent_alias, const std::string& type, const std::string& data) {
    Finding f;
    f.id = "find_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    f.type = type;
    f.description = data;
    f.severity = "high";
    f.confidence = 0.95;
    f.source_agent = agent_alias;

    Blackboard::getInstance().addFinding(f);
}

}
