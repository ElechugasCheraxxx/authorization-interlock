#include "DepthCacheManager.hpp"
#include <chrono>

namespace interlock {

void DepthCacheManager::pushInsight(const std::string& alias, const std::vector<float>& state, const std::string& summary) {
    std::lock_guard<std::mutex> lock(mtx);

    DepthLayer layer;
    layer.latent_state = state;
    layer.summary = summary;
    layer.timestamp = std::chrono::system_clock::now().time_since_epoch().count();

    cache[alias].push_back(layer);

    // Mantener solo los últimos N insights para no saturar
    if (cache[alias].size() > 20) {
        cache[alias].erase(cache[alias].begin());
    }
}

std::vector<DepthCacheManager::DepthLayer> DepthCacheManager::getVerticalContext(const std::string& alias, int max_layers) {
    std::lock_guard<std::mutex> lock(mtx);

    if (cache.find(alias) == cache.end()) {
        return {};
    }

    const auto& layers = cache[alias];
    int count = std::min((int)layers.size(), max_layers);

    return std::vector<DepthLayer>(layers.end() - count, layers.end());
}

void DepthCacheManager::clear(const std::string& alias) {
    std::lock_guard<std::mutex> lock(mtx);
    cache.erase(alias);
}

} // namespace interlock
