#include "Cipher.hpp"
#include <iostream>

namespace interlock {

CipherAgent::CipherAgent(const std::string& model_path)
    : NativeAgent("cipher", model_path) {}

void CipherAgent::onResponseGenerated(const std::string& raw_output) {
    // Cipher: God Mode Agent. Estratega supremo.
    std::cout << "[Cipher] Strategic Directive Issued: " << raw_output.substr(0, 100) << "..." << std::endl;
}

} // namespace interlock
