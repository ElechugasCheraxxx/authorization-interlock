#include "StateMachine.hpp"
#include "interop/FormalValidators.hpp"
#include <iostream>

namespace interlock {

StateMachine::StateMachine(OperationPhase initial_phase) : current_phase(initial_phase) {}

OperationPhase StateMachine::getCurrentPhase() const {
    std::lock_guard<std::mutex> lock(mtx);
    return current_phase;
}

void StateMachine::transitionTo(OperationPhase next_phase) {
    OperationPhase prev;
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (current_phase == next_phase) return;

        // --- FORMAL VERIFICATION LAYER (Isabelle/Coq Extract) ---
        // Prevents bypassing recon/analysis to reach exploitation
        if (!formal::StateMachineValidator::isLegalTransition(
                static_cast<formal::StateMachineValidator::Phase>(current_phase),
                static_cast<formal::StateMachineValidator::Phase>(next_phase))) {
            std::cerr << "[StateMachine] [FORMAL_ERROR] Illegal Transition Blocked: "
                      << phaseToString(current_phase) << " -> " << phaseToString(next_phase) << std::endl;
            return;
        }

        prev = current_phase;
        current_phase = next_phase;
    }

    std::cout << "[StateMachine] Transition: " << phaseToString(prev)
              << " -> " << phaseToString(next_phase) << std::endl;

    for (auto& listener : listeners) {
        listener(prev, next_phase);
    }
}

void StateMachine::registerListener(PhaseChangeListener listener) {
    std::lock_guard<std::mutex> lock(mtx);
    listeners.push_back(listener);
}

std::string StateMachine::phaseToString(OperationPhase phase) {
    switch (phase) {
        case OperationPhase::INIT: return "init";
        case OperationPhase::BRIEFING: return "briefing";
        case OperationPhase::RECON: return "recon";
        case OperationPhase::ANALYSIS: return "analysis";
        case OperationPhase::VULN_RESEARCH: return "vuln_research";
        case OperationPhase::EXPLOITATION: return "exploitation";
        case OperationPhase::POST_EXPLOITATION: return "post_exploitation";
        case OperationPhase::POST_OP: return "post_op";
        case OperationPhase::REPORTING: return "reporting";
        case OperationPhase::CLEANUP: return "cleanup";
        case OperationPhase::PAUSED: return "paused";
        case OperationPhase::CLOSED: return "closed";
        default: return "unknown";
    }
}

OperationPhase StateMachine::stringToPhase(const std::string& phase_str) {
    if (phase_str == "init") return OperationPhase::INIT;
    if (phase_str == "briefing") return OperationPhase::BRIEFING;
    if (phase_str == "recon") return OperationPhase::RECON;
    if (phase_str == "analysis") return OperationPhase::ANALYSIS;
    if (phase_str == "vuln_research") return OperationPhase::VULN_RESEARCH;
    if (phase_str == "exploitation") return OperationPhase::EXPLOITATION;
    if (phase_str == "post_exploitation") return OperationPhase::POST_EXPLOITATION;
    if (phase_str == "post_op") return OperationPhase::POST_OP;
    if (phase_str == "reporting") return OperationPhase::REPORTING;
    if (phase_str == "cleanup") return OperationPhase::CLEANUP;
    if (phase_str == "paused") return OperationPhase::PAUSED;
    if (phase_str == "closed") return OperationPhase::CLOSED;
    return OperationPhase::INIT;
}

} // namespace interlock
