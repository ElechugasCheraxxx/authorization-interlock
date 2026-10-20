#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <regex>

namespace interlock {

struct ROERule {
    std::string pattern;
    bool allow;
    std::string description;
};

class ScopeGuard {
public:
    static ScopeGuard& getInstance() {
        static ScopeGuard instance;
        return instance;
    }

    // Valida si una acción sobre un target está permitida por las ROE
    bool isAuthorized(const std::string& action, const std::string& target);

    // Carga reglas desde una configuración estática o memoria
    void loadRules(const std::vector<ROERule>& new_rules);

    // Logs de violaciones de scope (sin tocar disco si es posible, o vía RiskManager)
    void logViolation(const std::string& action, const std::string& target);

private:
    ScopeGuard();
    std::vector<ROERule> rules;
    std::mutex mtx;
};

} // namespace interlock
