#include "HealthMonitor.hpp"
#include <chrono>

namespace interlock {

void HealthMonitor::reportHeartbeat(const std::string& component_name, const json& metrics) {
    std::lock_guard<std::mutex> lock(mtx);
    ComponentStatus& s = statuses[component_name];
    s.name = component_name;
    s.status = metrics.value("status", "OK");
    s.cpu_usage = metrics.value("cpu", 0.0f);
    s.mem_usage_mb = metrics.value("mem", 0.0f);
    s.last_heartbeat = std::chrono::system_clock::now().time_since_epoch().count();
}

std::map<std::string, ComponentStatus> HealthMonitor::getFullStatus() const {
    std::lock_guard<std::mutex> lock(mtx);
    return statuses;
}

} // namespace interlock
