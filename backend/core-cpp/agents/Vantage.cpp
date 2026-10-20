#include "Vantage.hpp"
#include "ToolDispatcher.hpp"
#include "Blackboard.hpp"
#include <iostream>

namespace interlock {

VantageAgent::VantageAgent(const std::string& model_path)
    : NativeAgent("vantage", model_path) {}

void VantageAgent::onResponseGenerated(const std::string& raw_output) {
    // Vantage: Intel Head. OSINT y correlación semántica.
    json data = ToolDispatcher::getInstance().parseGBNFResponse(raw_output);

    if (data.count("recon") || data.count("intel")) {
        std::cout << "[Vantage] Intel Data Acquired. Correlating..." << std::endl;

        Event e;
        e.type = "INTEL_DATA_ACQUIRED";
        e.source = "vantage";
        e.data = data;
        EventBus::getInstance().publish(e);
    }
}

} // namespace interlock
