#pragma once

#include <string>
#include <map>
#include <mutex>
#include "Schema.hpp"

namespace interlock {

struct TokenQuota {
    int allocated;
    int used;
    int reserved;
};

class TokenDispatcher {
public:
    static TokenDispatcher& getInstance() {
        static TokenDispatcher instance;
        return instance;
    }

    // Solicita permiso para gastar tokens (M4 Pro Resource Arbitration)
    bool requestTokens(const std::string& agent_alias, int amount);

    // Actualiza el consumo real tras una inferencia
    void reportUsage(const std::string& agent_alias, int amount);

    // Ajusta cuotas dinámicamente según la fase de la operación
    void rebalanceQuotas(const std::map<std::string, float>& priorities);

private:
    TokenDispatcher() = default;

    std::map<std::string, TokenQuota> quotas;
    std::mutex mtx;
    int global_reserve = 5000; // Tokens reservados para emergencias/OPSEC
};

} // namespace interlock
