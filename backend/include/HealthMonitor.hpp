#pragma once

#include <string>
#include <map>
#include <mutex>
#include "Schema.hpp"

namespace interlock {

struct ComponentStatus {
    std::string name;
    std::string status; // OK, WARNING, ERROR
    float cpu_usage;
    float mem_usage_mb;
    long long last_heartbeat;
};

class HealthMonitor {
public:
    static HealthMonitor& getInstance() {
        static HealthMonitor instance;
        return instance;
    }

    void reportHeartbeat(const std::string& component_name, const json& metrics);
    std::map<std::string, ComponentStatus> getFullStatus() const;

private:
    HealthMonitor() = default;
    std::map<std::string, ComponentStatus> statuses;
    mutable std::mutex mtx;
};

} // namespace interlock
