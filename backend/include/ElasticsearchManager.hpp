#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace interlock {

/**
 * @brief Motor de búsqueda y análisis de operaciones (Findings & Logs).
 */
class ElasticsearchManager {
public:
    static ElasticsearchManager& getInstance() {
        static ElasticsearchManager instance;
        return instance;
    }

    void indexFinding(const std::string& index, const std::string& id, const nlohmann::json& document);
    void search(const std::string& query);

private:
    ElasticsearchManager() = default;
};

} // namespace interlock
