#include "interop/FormalValidators.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <cassert>

using namespace interlock::formal;

// --- Property-Based Testing (PBT) Infrastructure ---
// In a full environment, we would use RapidCheck. Here we implement a custom generator.

class PDGGenerator {
public:
    static std::vector<PrimitiveData> generateRandomChain(size_t length) {
        std::vector<PrimitiveData> chain;
        for (size_t i = 0; i < length; ++i) {
            chain.push_back({"prim_" + std::to_string(i), rand() % 100, false});
        }
        return chain;
    }

    static std::vector<PrimitiveData> generateCyclicChain(size_t length) {
        auto chain = generateRandomChain(length);
        if (length > 2) {
            // Introduce a cycle: last node points back to the first
            chain.push_back(chain[0]);
        }
        return chain;
    }
};

// --- Test Properties ---

void property_pdg_acyclicity() {
    std::cout << "[PBT] Property: PDG Zero-Error chains must be acyclic" << std::endl;

    for (int i = 0; i < 100; ++i) {
        // Test with valid random chains
        auto chain = PDGGenerator::generateRandomChain(5 + rand() % 10);
        assert(PDGValidator::isAcyclic(chain) == true);

        // Test with intentionally cyclic chains
        auto cyclic_chain = PDGGenerator::generateCyclicChain(5);
        if (!PDGValidator::isAcyclic(cyclic_chain)) {
            // Success: the validator (certified by Coq logic) caught the cycle
        } else {
            throw std::runtime_error("PBT Failure: Cycle not detected!");
        }
    }
}

void property_state_machine_integrity() {
    std::cout << "[PBT] Property: Mission cannot bypass Recon to reach Exploitation" << std::endl;

    for (int i = 0; i < 100; ++i) {
        std::vector<StateMachineValidator::Phase> illegal_path = {
            StateMachineValidator::Phase::INIT,
            StateMachineValidator::Phase::BRIEFING,
            // MISSING RECON
            StateMachineValidator::Phase::EXPLOITATION
        };

        bool valid = StateMachineValidator::isValidPath(StateMachineValidator::Phase::INIT, illegal_path);
        assert(valid == false); // Should be caught by the formal transition rules
    }
}

int main() {
    std::cout << "=== Interlock Formal Property-Based Tests ===" << std::endl;
    std::srand(time(0));

    try {
        property_pdg_acyclicity();
        property_state_machine_integrity();
        std::cout << "\n[SUCCESS] All formal properties verified via PBT simulation." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "\n[FAILED] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
