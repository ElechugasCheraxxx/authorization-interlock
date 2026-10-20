#include "Chronos.hpp"
#include <iostream>

namespace interlock {

ChronosAgent::ChronosAgent(const std::string& model_path)
    : NativeAgent("chronos", model_path) {}

void ChronosAgent::onResponseGenerated(const std::string& raw_output) {
    // Chronos: Temporal Analyst. Gestión de TTL.
    std::cout << "[Chronos] Time Coherence Sync: " << raw_output.substr(0, 50) << std::endl;
}

} // namespace interlock
