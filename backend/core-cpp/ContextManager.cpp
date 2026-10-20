#include "ContextManager.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>

namespace interlock {

int ContextManager::prepareContext(const std::string& alias, const std::vector<llama_token>& current_prompt_tokens, int max_ctx) {
    std::lock_guard<std::mutex> lock(mtx);
    auto& state = states[alias];
    state.alias = alias;

    long long now = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    state.last_interaction = now;

    // Si es la primera vez, el n_past es 0
    // Si ya hay tokens, intentamos hacer un "Smart Reuse" de la cache (KV Cache reuse)
    // El n_past indica hasta qué punto la cache de llama.cpp es válida para este agente

    int n_tokens = current_prompt_tokens.size();

    // Si el prompt actual es el mismo que el anterior en su inicio, podemos reusar
    // (Lógica simplificada para el M4 Pro: si n_past > 0, asumimos que estamos en un diálogo)
    if (state.n_past + n_tokens > max_ctx) {
        enforceLimits(alias, max_ctx);
    }

    return state.n_past;
}

void ContextManager::commitTokens(const std::string& alias, int count) {
    std::lock_guard<std::mutex> lock(mtx);
    states[alias].n_past += count;
}

void ContextManager::enforceLimits(const std::string& alias, int max_ctx) {
    auto& state = states[alias];

    // MLA (Multi-Latent Attention) Compression Emulation:
    // Instead of simple sliding window, we prioritize latent relevance.
    // For now, we implement KV Cache Shifting for massive context (>100k).

    int reserved = std::min(state.n_past, KEEP_SYSTEM_TOKENS);
    int remaining = state.n_past - reserved;

    if (state.n_past > max_ctx * 0.9) {
        int window_size = max_ctx / 4; // Keep 25% of the window for recent history
        int to_erase = remaining - window_size;

        if (to_erase > 0) {
            std::cout << "[ContextManager] Sliding Window Eviction for " << alias
                      << ": Removing " << to_erase << " stale tokens." << std::endl;

            // In a real llama.cpp integration, we would call llama_kv_cache_seq_rm
            // Here we update the logic: n_past = reserved + window_size
            state.n_past = reserved + window_size;

            // Logically shift the history if we were storing it
            if (!state.history.empty() && (int)state.history.size() > state.n_past) {
                state.history.erase(state.history.begin() + reserved,
                                    state.history.begin() + reserved + to_erase);
            }
        }
    }
}

void ContextManager::resetContext(const std::string& alias) {
    std::lock_guard<std::mutex> lock(mtx);
    states[alias].n_past = 0;
    states[alias].history.clear();
    std::cout << "[ContextManager] Context cleared for agent: " << alias << std::endl;
}

} // namespace interlock
