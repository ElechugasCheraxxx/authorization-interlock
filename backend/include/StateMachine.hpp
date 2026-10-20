#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>

namespace interlock {

enum class OperationPhase {
    INIT,
    BRIEFING,
    RECON,
    ANALYSIS,
    VULN_RESEARCH,
    EXPLOITATION,
    POST_EXPLOITATION,
    POST_OP,
    REPORTING,
    CLEANUP,
    PAUSED,
    CLOSED
};

using PhaseChangeListener = std::function<void(OperationPhase prev, OperationPhase current)>;

class StateMachine {
public:
    StateMachine(OperationPhase initial_phase = OperationPhase::INIT);

    OperationPhase getCurrentPhase() const;
    void transitionTo(OperationPhase next_phase);
    void registerListener(PhaseChangeListener listener);

    static std::string phaseToString(OperationPhase phase);
    static OperationPhase stringToPhase(const std::string& phase_str);

private:
    OperationPhase current_phase;
    std::vector<PhaseChangeListener> listeners;
    mutable std::mutex mtx;
};

} // namespace interlock
