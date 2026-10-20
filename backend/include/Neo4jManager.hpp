#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace interlock {

/**
 * @brief Gestiona el grafo de infraestructura y relaciones de ataque (Global Map).
 */
class Neo4jManager {
public:
    static Neo4jManager& getInstance() {
        static Neo4jManager instance;
        return instance;
    }

    void addNode(const std::string& label, const std::string& id, const nlohmann::json& properties);
    void addRelation(const std::string& source_id, const std::string& target_id, const std::string& type, const nlohmann::json& properties);

private:
    Neo4jManager() = default;
};

} // namespace interlock
