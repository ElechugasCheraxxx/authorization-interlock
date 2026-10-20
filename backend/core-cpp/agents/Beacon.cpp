#include "Beacon.hpp"
#include "ToolDispatcher.hpp"
#include <iostream>

namespace interlock {

BeaconAgent::BeaconAgent(const std::string& model_path)
    : NativeAgent("beacon", model_path) {}

void BeaconAgent::onResponseGenerated(const std::string& raw_output) {
    // Beacon: Operations Head. Monitoreo de estado y SITREPs.
    json data = ToolDispatcher::getInstance().parseGBNFResponse(raw_output);

    if (data.count("status_report")) {
        std::cout << "[Beacon] SITREP Received: " << data["status_report"] << std::endl;
    }
}

} // namespace interlock
