#ifndef KERNEL_BRIDGE_HPP
#define KERNEL_BRIDGE_HPP

#include <IOKit/IOKitLib.h>
#include "../shield-c/interlock_shared_types.h"
#include <iostream>
#include <sys/syscall.h>

extern "C" {
    // Stubs definidos en stubs-asm/darwin_arm64/syscalls.s
    long direct_syscall(long number, ...);
    void clear_registers_secure();
    uint64_t get_nanoseconds_arm();
}

namespace interlock {

class KernelBridge {
public:
    KernelBridge() : connection(IO_OBJECT_NULL) {}
    ~KernelBridge() { disconnect(); }

    /**
     * @brief Realiza una syscall directa para evadir hooks de EDR en libsystem.
     */
    template<typename... Args>
    long invoke_stealth_syscall(long number, Args... args) {
        return direct_syscall(number, args...);
    }

    /**
     * @brief Anti-Debugging: Detecta si el tiempo entre dos puntos es excesivo (stepping).
     */
    bool detect_stepping(uint64_t threshold_ns = 1000) {
        uint64_t t1 = get_nanoseconds_arm();
        // Operación ficticia para medir
        for(int i=0; i<100; ++i) __asm__ volatile("nop");
        uint64_t t2 = get_nanoseconds_arm();
        return (t2 - t1) > threshold_ns;
    }

    bool connect() {
        // ... (resto del código igual o llamar vía direct_syscall si es posible)
        io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("com_interlock_Shield"));
        if (!service) {
            std::cerr << "[!] KernelBridge: Shield service not found. Is the KEXT loaded?" << std::endl;
            return false;
        }

        kern_return_t kr = IOServiceOpen(service, mach_task_self(), 0, &connection);
        IOObjectRelease(service);

        if (kr != KERN_SUCCESS) {
            std::cerr << "[!] KernelBridge: Failed to open connection (Error: " << kr << ")" << std::endl;
            return false;
        }

        std::cout << "[+] KernelBridge: Connected to Interlock Shield." << std::endl;
        return true;
    }

    void disconnect() {
        if (connection != IO_OBJECT_NULL) {
            IOServiceClose(connection);
            connection = IO_OBJECT_NULL;
        }
    }

    bool getShieldStatus(InterlockStatusResponse& status) {
        if (connection == IO_OBJECT_NULL) return false;

        size_t outputSize = sizeof(InterlockStatusResponse);
        kern_return_t kr = IOConnectCallStructMethod(connection, kInterlockGetStatus, nullptr, 0, &status, &outputSize);

        return (kr == KERN_SUCCESS);
    }

private:
    io_connect_t connection;
};

} // namespace interlock

#endif
