#include "DetectionModule.hpp"
#include "EventSystem.hpp"
#include <algorithm>

namespace interlock {

bool WAFDetector::analyze(const TrafficSignal& signal) {
    std::string content = signal.path + " " + signal.body;
    std::transform(content.begin(), content.end(), content.begin(), ::tolower);

    for (const auto& sig : signatures) {
        if (content.find(sig) != std::string::npos) {
            SignalMonitor::getInstance().reportAnomaly("WAF_DETECTION", {{"signature", sig}, {"ip", signal.source_ip}});
            return true;
        }
    }
    return false;
}

bool RateLimiter::isRateLimited(const std::string& key, int limit, int window_seconds) {
    std::lock_guard<std::mutex> lock(mtx);
    auto now = std::chrono::steady_clock::now();

    auto& timestamps = hits[key];

    // Limpiar antiguos
    timestamps.erase(std::remove_if(timestamps.begin(), timestamps.end(),
        [&](const auto& t) {
            return std::chrono::duration_cast<std::chrono::seconds>(now - t).count() > window_seconds;
        }), timestamps.end());

    if (timestamps.size() >= static_cast<size_t>(limit)) {
        return true;
    }

    timestamps.push_back(now);
    return false;
}

void SignalMonitor::reportAnomaly(const std::string& type, const json& details) {
    Event e;
    e.id = "anomaly_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    e.type = "ANOMALY";
    e.source = "DetectionModule";
    e.data = {{"anomaly_type", type}, {"details", details}};
    e.timestamp = std::chrono::system_clock::now().time_since_epoch().count();

    EventBus::getInstance().publish(e);
}

} // namespace interlock
