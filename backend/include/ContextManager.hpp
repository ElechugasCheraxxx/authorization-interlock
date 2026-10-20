#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include "llama.h"

namespace interlock {

struct AgentContextState {
    std::string alias;
    int n_past = 0;           // Cuántos tokens ya están en la KV Cache
    int system_tokens = 0;    // Tokens del prompt del sistema (que nunca se borran)
    std::vector<llama_token> history;
    long long last_interaction = 0;
};

/**
 * ContextManager (M4 Pro Native)
 * Manages KV Cache persistence and sliding window compression.
 */
class ContextManager {
public:
    static ContextManager& getInstance() {
        static ContextManager instance;
        return instance;
    }

    // Sincroniza la posición de la cache para un agente
    int prepareContext(const std::string& alias, const std::vector<llama_token>& current_prompt_tokens, int max_ctx);

    // Actualiza el contador después de una generación
    void commitTokens(const std::string& alias, int count);

    // Libera espacio si nos acercamos al límite (Smart Eviction)
    void enforceLimits(const std::string& alias, int max_ctx);

    // Limpia la memoria de un agente específico
    void resetContext(const std::string& alias);

private:
    ContextManager() = default;
    std::map<std::string, AgentContextState> states;
    std::mutex mtx;

    // El M4 Pro nos permite mantener el "System Prompt" siempre caliente
    const int KEEP_SYSTEM_TOKENS = 1024;
};

} // namespace interlock
