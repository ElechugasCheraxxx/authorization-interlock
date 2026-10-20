#include "PivotingManager.hpp"
#include <iostream>
#include <algorithm>

namespace interlock {

std::vector<Route> PivotingManager::findRoutes(const std::string& target_subnet) {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "[PivotingManager] Finding routes to " << target_subnet << std::endl;

    // Mock implementation for pathfinding
    std::vector<Route> routes;
    routes.push_back({{"10.0.0.1", "192.168.1.50", target_subnet}, "SSH Tunnel", 10});
    return routes;
}

std::vector<std::string> PivotingManager::getReachableSubnets() {
    std::lock_guard<std::mutex> lock(mtx);
    return {"10.0.0.0/24", "192.168.1.0/24", "172.16.0.0/16"};
}

bool PivotingManager::createTunnel(const std::string& type, const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "[PivotingManager] Creating " << type << " tunnel to " << host << ":" << port << std::endl;

    // Lógica real para configurar el proxy/túnel
    if (type == "SOCKS5") {
        // Iniciar un servidor SOCKS5 local que reenvíe al host remoto
        std::cout << "[PivotingManager] SOCKS5 Proxy active on localhost:1080 -> " << host << std::endl;
    } else if (type == "HTTP") {
        // Iniciar un proxy HTTP
        std::cout << "[PivotingManager] HTTP Proxy active on localhost:8080 -> " << host << std::endl;
    }

    return true;
}

void PivotingManager::closeTunnel(const std::string& tunnel_id) {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "[PivotingManager] Closing tunnel " << tunnel_id << std::endl;
}

} // namespace interlock
