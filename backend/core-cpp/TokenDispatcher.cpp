#include "TokenDispatcher.hpp"
#include <algorithm>

namespace interlock {

bool TokenDispatcher::requestTokens(const std::string& agent_alias, int amount) {
    std::lock_guard<std::mutex> lock(mtx);

    // Si el agente no existe, inicializamos una cuota base
    if (quotas.find(agent_alias) == quotas.end()) {
        quotas[agent_alias] = {1000, 0, 0};
    }

    auto& q = quotas[agent_alias];

    // Verificamos disponibilidad en la cuota local + reserva global
    if ((q.allocated - q.used) >= amount) {
        q.used += amount;
        return true;
    } else if (global_reserve >= amount) {
        global_reserve -= amount;
        q.used += amount;
        return true;
    }

    return false;
}

void TokenDispatcher::reportUsage(const std::string& agent_alias, int amount) {
    std::lock_guard<std::mutex> lock(mtx);
    if (quotas.find(agent_alias) != quotas.end()) {
        auto& q = quotas[agent_alias];
        // Adjust the usage based on real consumption
        // If amount < 0, it's a refund; if > 0, it's additional consumption
        q.used = std::max(0, q.used + amount);

        // Log precise consumption for billing/telemetry
        // std::cout << "[TokenDispatcher] Agent " << agent_alias << " adjusted usage by " << amount << std::endl;
    }
}

void TokenDispatcher::rebalanceQuotas(const std::map<std::string, float>& priorities) {
    std::lock_guard<std::mutex> lock(mtx);

    int total_available = global_reserve;
    for (auto& [alias, q] : quotas) {
        total_available += (q.allocated - q.used);
        q.used = 0; // Reset para el nuevo ciclo de balanceo
    }

    float total_priority = 0;
    for (auto const& [alias, priority] : priorities) {
        total_priority += priority;
    }

    if (total_priority > 0) {
        for (auto const& [alias, priority] : priorities) {
            quotas[alias].allocated = static_cast<int>((priority / total_priority) * total_available);
        }
    }
}

} // namespace interlock
