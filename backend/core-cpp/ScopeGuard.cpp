#include "ScopeGuard.hpp"
#include "RiskManager.hpp"
#include <iostream>
#include <regex>

namespace interlock {

ScopeGuard::ScopeGuard() {
    // Default rules: Allow local communication for orchestration, block external sensitive ranges.
    // In production, these are reinforced by the 'agents.toml' ROE (Rules of Engagement) section.
    rules.push_back({".*\\.local", true, "Allow internal .local domains for node discovery"});
    rules.push_back({"127\\.0\\.0\\.1", true, "Allow localhost for C2-Orchestrator bridge"});
    rules.push_back({"10\\..*", false, "Block private Class A unless explicitly whitelisted"});
    rules.push_back({"192\\.168\\..*", false, "Block private Class C unless explicitly whitelisted"});
}

bool ScopeGuard::isAuthorized(const std::string& action, const std::string& target) {
    std::lock_guard<std::mutex> lock(mtx);

    // Check against rules
    for (const auto& rule : rules) {
        if (std::regex_search(target, std::regex(rule.pattern))) {
            if (!rule.allow) {
                logViolation(action, target);
                return false;
            }
        }
    }

    return true;
}

void ScopeGuard::loadRules(const std::vector<ROERule>& new_rules) {
    std::lock_guard<std::mutex> lock(mtx);
    rules = new_rules;
}

void ScopeGuard::logViolation(const std::string& action, const std::string& target) {
    // In a real scenario, this would notify the RiskManager and potentially trigger a lockdown
    std::cerr << "[ScopeGuard] SECURITY VIOLATION: Action '" << action
              << "' on target '" << target << "' is NOT authorized by ROE." << std::endl;

    // Notify RiskManager of a major deviation
    RiskManager::getInstance().reportDeviation("ScopeGuard", "ROE Violation: " + action + " -> " + target, 8);
}

} // namespace interlock
