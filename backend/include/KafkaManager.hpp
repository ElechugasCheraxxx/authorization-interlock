#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace interlock {

/**
 * @brief Backbone de eventos para Replay Mode y desacoplamiento de servicios.
 */
class KafkaManager {
public:
    static KafkaManager& getInstance() {
        static KafkaManager instance;
        return instance;
    }

    void produce(const std::string& topic, const std::string& key, const std::string& value);
    // Consumo para Replay Mode
    void consume(const std::string& topic, long long offset);

private:
    KafkaManager() = default;
};

} // namespace interlock
