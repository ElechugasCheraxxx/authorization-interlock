#include "Atlas.hpp"
#include "ToolDispatcher.hpp"
#include <iostream>

namespace interlock {

AtlasAgent::AtlasAgent(const std::string& model_path)
    : NativeAgent("atlas", model_path) {}

void AtlasAgent::onResponseGenerated(const std::string& raw_output) {
    // Atlas: Movimiento lateral y ataques a AD.
    json data = ToolDispatcher::getInstance().parseGBNFResponse(raw_output);

    if (data.count("lateral_movement")) {
        std::cout << "[Atlas] Planning lateral jump to: " << data["lateral_movement"]["target"] << std::endl;
    }
}

} // namespace interlock
