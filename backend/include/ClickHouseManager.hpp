#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace interlock {

/**
 * @brief Almacén de métricas y rendimiento para dashboards instantáneos (Performance Layer).
 */
class ClickHouseManager {
public:
    static ClickHouseManager& getInstance() {
        static ClickHouseManager instance;
        return instance;
    }

    void insertMetric(const std::string& table, const nlohmann::json& metric);
    void queryMetrics(const std::string& sql);

private:
    ClickHouseManager() = default;
};

} // namespace interlock
