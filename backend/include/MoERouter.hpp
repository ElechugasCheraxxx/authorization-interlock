#pragma once
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <algorithm>

namespace interlock {

/**
 * MoERouter: Implementación nativa del gating de DeepSeek-V3.
 * Se utiliza para el "Swarm Routing" (Enrutamiento de Enjambre).
 */
class MoERouter {
public:
    struct RoutingDecision {
        std::vector<std::pair<std::string, float>> selected_experts; // N_r (Routed Experts)
        std::vector<std::string> shared_experts;                    // K_s (Shared Experts)
        float balance_loss;
    };

    MoERouter(int n_routed, int n_activated, int n_shared);

    // Enruta una tarea basándose en un vector de afinidad
    RoutingDecision route(const std::vector<float>& task_embedding, const std::vector<std::string>& agent_pool);

private:
    int n_routed;
    int n_activated;
    int n_shared;

    // Pesos del enrutador (simulando W_gate de moda.py)
    // En producción estos se cargarían de un archivo .bin o .gguf
    std::vector<std::vector<float>> gate_weights;

    float softmax(const std::vector<float>& logits, std::vector<float>& scores);
};

} // namespace interlock
