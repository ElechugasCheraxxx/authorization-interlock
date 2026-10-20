#include "MoERouter.hpp"
#include <numeric>
#include <cmath>

namespace interlock {

MoERouter::MoERouter(int n_routed, int n_activated, int n_shared)
    : n_routed(n_routed), n_activated(n_activated), n_shared(n_shared) {
    // Inicialización de pesos aleatorios (en un entorno real se cargarían de los pesos del modelo)
    gate_weights.resize(n_routed, std::vector<float>(128, 0.01f));
}

float MoERouter::softmax(const std::vector<float>& logits, std::vector<float>& scores) {
    float max_logit = *std::max_element(logits.begin(), logits.end());
    float sum = 0.0f;
    scores.resize(logits.size());

    for (size_t i = 0; i < logits.size(); ++i) {
        scores[i] = std::exp(logits[i] - max_logit);
        sum += scores[i];
    }

    for (size_t i = 0; i < scores.size(); ++i) {
        scores[i] /= sum;
    }
    return sum;
}

MoERouter::RoutingDecision MoERouter::route(const std::vector<float>& task_embedding, const std::vector<std::string>& agent_pool) {
    std::vector<float> logits(n_routed, 0.0f);

    // Simulación de x * W^T
    for (int i = 0; i < n_routed; ++i) {
        float dot = 0.0f;
        size_t dim = std::min(task_embedding.size(), gate_weights[i].size());
        for (size_t j = 0; j < dim; ++j) {
            dot += task_embedding[j] * gate_weights[i][j];
        }
        logits[i] = dot;
    }

    std::vector<float> scores;
    softmax(logits, scores);

    // Selección Top-K (n_activated) para expertos enrutados (N_r)
    std::vector<size_t> indices(n_routed);
    std::iota(indices.begin(), indices.end(), 0);

    std::partial_sort(indices.begin(), indices.begin() + n_activated, indices.end(),
                      [&scores](size_t i1, size_t i2) { return scores[i1] > scores[i2]; });

    RoutingDecision decision;
    decision.balance_loss = 0.0f;

    // 1. Asignar Expertos Compartidos (K_s) - Siempre activos (ej: Cipher, Chronos)
    // En este simulacro, tomamos los primeros n_shared del pool como compartidos
    for (int i = 0; i < n_shared && i < agent_pool.size(); ++i) {
        decision.shared_experts.push_back(agent_pool[i]);
    }

    // 2. Asignar Expertos Enrutados (N_r) - Top-K según afinidad
    for (int k = 0; k < n_activated; ++k) {
        size_t idx = indices[k];
        if (idx < agent_pool.size()) {
            decision.selected_experts.push_back({agent_pool[idx], scores[idx]});
        }
    }

    return decision;
}

} // namespace interlock
