#pragma once
#include <vector>
#include <string>
#include <map>
#include <mutex>

namespace interlock {

/**
 * DepthCacheManager: Inspirado en Mixture-of-Depths (MoDA).
 * Almacena estados de "pensamiento profundo" por capa/agente para
 * recuperación vertical de contexto sin re-tokenización.
 */
class DepthCacheManager {
public:
    struct DepthLayer {
        std::vector<float> latent_state; // Representación vectorial del insight
        std::string summary;             // Resumen textual para inyección rápida
        long long timestamp;
    };

    static DepthCacheManager& getInstance() {
        static DepthCacheManager instance;
        return instance;
    }

    // Registra un "depth insight" después de una inferencia
    void pushInsight(const std::string& alias, const std::vector<float>& state, const std::string& summary);

    // Recupera los insights más relevantes verticalmente (MoDA style)
    std::vector<DepthLayer> getVerticalContext(const std::string& alias, int max_layers = 5);

    void clear(const std::string& alias);

private:
    DepthCacheManager() = default;
    std::map<std::string, std::vector<DepthLayer>> cache;
    std::mutex mtx;
};

} // namespace interlock
