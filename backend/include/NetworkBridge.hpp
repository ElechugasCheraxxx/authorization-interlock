#ifndef NETWORK_BRIDGE_HPP
#define NETWORK_BRIDGE_HPP

#include <string>
#include <vector>
#include <functional>

namespace interlock {

struct NetworkStats {
    uint64_t bytesSent;
    uint64_t bytesReceived;
    int activeConnections;
};

class NetworkBridge {
public:
    static NetworkBridge& getInstance();

    void initialize();
    void shutdown();

    // Stealthy transmission using Network.framework
    bool transmitStealthy(const std::string& destination, const std::vector<uint8_t>& data);

    NetworkStats getStats() const;

private:
    NetworkBridge() = default;
    ~NetworkBridge() = default;
    NetworkBridge(const NetworkBridge&) = delete;
    NetworkBridge& operator=(const NetworkBridge&) = delete;

    NetworkStats stats = {0, 0, 0};
};

} // namespace interlock

#endif // NETWORK_BRIDGE_HPP
