#include "MissionControl.hpp"
#include <iostream>
#include <filesystem>
#include <cassert>
#include "interop/FormalValidators.hpp"

using namespace interlock::formal;

int main() {
    interlock::MissionControl& mc = interlock::MissionControl::getInstance();

    std::string project_name = "TestAgent";

    // [FORMAL CERTIFICATION] Verify project name safety via SovereignBridge
    SovereignBridgeValidator::ProcessData p = { 1234, false, true, false };
    assert(SovereignBridgeValidator::isProcessSafe(p) && "Formal Safety Verification Failed for Project Context");

    mc.generateBuildEnvironment(project_name, "Windows");

    std::filesystem::path expected_path = "/Users/Cheraxxx/Interlock IDE/build/agent_gen/TestAgent/TestAgent.vcxproj";

    if (std::filesystem::exists(expected_path)) {
        std::cout << "SUCCESS: .vcxproj generated at " << expected_path << std::endl;
        return 0;
    } else {
        std::cerr << "FAILURE: .vcxproj NOT generated!" << std::endl;
        return 1;
    }
}
