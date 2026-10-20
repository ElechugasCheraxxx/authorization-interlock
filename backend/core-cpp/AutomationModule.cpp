#include "AutomationModule.hpp"
#include "Blackboard.hpp"
#include "RiskManager.hpp"
#include <iostream>

namespace interlock {

AutomationModule::AutomationModule()
    : state(HeadlessState::CIPHER_ACTIVE), restart_attempts(0), running(true) {
    watchdog_thread = std::thread(&AutomationModule::watchdogLoop, this);
}

void AutomationModule::onOrchestratorFailure(const std::string& reason) {
    std::cout << "[AutomationModule] Orchestrator failure detected: " << reason << std::endl;
    HeadlessState current = state.load();
    if (current == HeadlessState::CIPHER_ACTIVE || current == HeadlessState::RESTARTING) {
        state.store(HeadlessState::RESTARTING);
        attemptRestart();
    }
}

void AutomationModule::onOrchestratorRecovery() {
    std::cout << "[AutomationModule] Orchestrator recovered. Resuming normal operations." << std::endl;
    state.store(HeadlessState::CIPHER_ACTIVE);
    restart_attempts = 0;
}

void AutomationModule::attemptRestart() {
    if (restart_attempts < 3) {
        restart_attempts++;
        std::cout << "[AutomationModule] Attempting restart " << restart_attempts << "/3..." << std::endl;

        if (restart_cb && restart_cb()) {
            onOrchestratorRecovery();
            return;
        }

        // Wait and retry is handled by logic or caller, here we simulate a failure leads to headless
        if (restart_attempts >= 3) {
            enterHeadlessMode("Max restart attempts reached");
        }
    }
}

void AutomationModule::enterHeadlessMode(const std::string& reason) {
    std::cout << "[AutomationModule] ENTERING HEADLESS MODE: " << reason << std::endl;
    state.store(HeadlessState::HEADLESS);
    headless_since = std::chrono::system_clock::now();

    // In headless mode, agents like Coordinator or Atlas operate under a pre-defined "Constitution"
    // Load constitution rules from Blackboard/Config
    auto& bb = Blackboard::getInstance();
    Finding f;
    f.id = "sys_automation_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    f.agent = "sys_automation";
    f.title = "Headless Mode Activated";
    f.severity = "HIGH";
    f.raw_stream = "Reason: " + reason;
    f.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    bb.addFinding(f);

    // Notify RiskManager to increase monitoring
    RiskManager::getInstance().reportDeviation("AutomationModule", "Headless mode transition due to: " + reason, 2);
}

void AutomationModule::watchdogLoop() {
    while (running) {
        if (state.load() == HeadlessState::HEADLESS) {
            auto now = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - headless_since).count();

            if (duration > 300) { // 5 minutes
                std::cout << "[AutomationModule] WATCHDOG: Headless mode active for > 5 mins. Escalating..." << std::endl;
                // Trigger escalation logic
                RiskManager::getInstance().reportDeviation("Watchdog", "Headless mode timeout (5m). Possible stalled orchestrator.", 5);

                // Emergency restart attempt if Risk allows
                if (RiskManager::getInstance().getStatus().level != RiskLevel::CRITICAL) {
                    attemptRestart();
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

} // namespace interlock
