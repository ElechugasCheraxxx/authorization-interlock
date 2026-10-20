#pragma once

#include <string>
#include <functional>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

namespace interlock {

enum class HeadlessState {
    CIPHER_ACTIVE,
    RESTARTING,
    HEADLESS,
    AWAITING_OPERATOR,
    PAUSED
};

class AutomationModule {
public:
    static AutomationModule& getInstance() {
        static AutomationModule instance;
        return instance;
    }

    // Gestiona la transición a modo headless si el orquestador principal falla
    void onOrchestratorFailure(const std::string& reason);

    // Intenta recuperar el control normal
    void onOrchestratorRecovery();

    HeadlessState getState() const { return state.load(); }

    void setRestartCallback(std::function<bool()> cb) { restart_cb = cb; }

private:
    AutomationModule();

    void attemptRestart();
    void enterHeadlessMode(const std::string& reason);
    void watchdogLoop();

    std::atomic<HeadlessState> state;
    std::function<bool()> restart_cb;
    int restart_attempts;
    std::chrono::system_clock::time_point headless_since;

    mutable std::mutex mtx;
    std::thread watchdog_thread;
    std::atomic<bool> running;
};

} // namespace interlock
