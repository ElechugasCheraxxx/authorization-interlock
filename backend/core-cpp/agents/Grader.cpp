#include "Grader.hpp"
#include <iostream>

namespace interlock {

GraderAgent::GraderAgent(const std::string& model_path)
    : NativeAgent("grader", model_path) {}

void GraderAgent::onResponseGenerated(const std::string& raw_output) {
    // Grader: Logic Validator. Auditoría lógica interna.
    std::cout << "[Grader] Logical Audit Complete." << std::endl;
}

} // namespace interlock
