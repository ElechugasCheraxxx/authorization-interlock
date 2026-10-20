#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <mutex>

namespace interlock {

/**
 * @brief Gestiona el estado efímero y coordinación en tiempo real (Blackboard Live).
 */
class RedisManager {
public:
    static RedisManager& getInstance() {
        static RedisManager instance;
        return instance;
    }

    void set(const std::string& key, const nlohmann::json& value);
    nlohmann::json get(const std::string& key);
    void publish(const std::string& channel, const std::string& message);

private:
    RedisManager() = default;
    // Aquí iría la integración con hiredis o sw/redis-plus-plus
};

} // namespace interlock
