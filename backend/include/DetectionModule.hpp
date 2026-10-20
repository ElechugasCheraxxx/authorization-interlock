#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <chrono>
#include "Schema.hpp"

namespace interlock {

struct TrafficSignal {
    std::string source_ip;
    std::string path;
    std::string method;
    json headers;
    std::string body;
};

class WAFDetector {
public:
    static WAFDetector& getInstance() {
        static WAFDetector instance;
        return instance;
    }

    bool analyze(const TrafficSignal& signal);

private:
    WAFDetector() = default;
    std::vector<std::string> signatures = {
        "union select", "<script>", "../", "etc/passwd"
    };
};

class RateLimiter {
public:
    static RateLimiter& getInstance() {
        static RateLimiter instance;
        return instance;
    }

    bool isRateLimited(const std::string& key, int limit, int window_seconds);

private:
    RateLimiter() = default;
    std::map<std::string, std::vector<std::chrono::steady_clock::time_point>> hits;
    std::mutex mtx;
};

class SignalMonitor {
public:
    static SignalMonitor& getInstance() {
        static SignalMonitor instance;
        return instance;
    }

    void reportAnomaly(const std::string& type, const json& details);

private:
    SignalMonitor() = default;
};

} // namespace interlock
