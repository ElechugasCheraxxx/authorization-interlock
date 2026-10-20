#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>

namespace interlock {

struct Route {
    std::vector<std::string> path;
    std::string technique;
    int cost;
};

class PivotingManager {
public:
    static PivotingManager& getInstance() {
        static PivotingManager instance;
        return instance;
    }

    // --- Route Analysis (High Performance C++ Implementation) ---
    std::vector<Route> findRoutes(const std::string& target_subnet);
    std::vector<std::string> getReachableSubnets();

    // --- Tunnel Control ---
    bool createTunnel(const std::string& type, const std::string& host, int port);
    void closeTunnel(const std::string& tunnel_id);

private:
    PivotingManager() = default;
    std::mutex mtx;

    // Graph representation of the network
    std::map<std::string, std::vector<std::string>> network_graph;
};

} // namespace interlock
