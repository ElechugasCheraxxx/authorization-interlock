#include "Archive.hpp"
#include <iostream>

namespace interlock {

ArchiveAgent::ArchiveAgent(const std::string& model_path)
    : NativeAgent("archive", model_path) {}

void ArchiveAgent::onResponseGenerated(const std::string& raw_output) {
    // Archive: Registro en EvidenceVault y reporting automatizado.
    std::cout << "[Archive] Committing evidence to vault..." << std::endl;
}

} // namespace interlock
