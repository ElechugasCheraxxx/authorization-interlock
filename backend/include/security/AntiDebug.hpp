#pragma once

#include <sys/types.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/task.h>
#include <mach/exception_types.h>
#include <iostream>
#include <vector>

namespace interlock::security {

/**
 * @brief Multi-level anti-debugging engine.
 * Integrated patterns from referencia previa debuggercheckplugin.cpp and hardened debug checks.
 */
class AntiDebug {
public:
    static AntiDebug& getInstance() {
        static AntiDebug instance;
        return instance;
    }

    /**
     * @brief Performs a full suite of anti-debug checks.
     * @return true if a debugger is detected.
     */
    bool checkAll() {
        return checkSysctl() || checkMachExceptionPorts() || checkHardwareBreakpoints();
    }

    /**
     * @brief Classic sysctl P_TRACED check.
     */
    bool checkSysctl() {
        int name[4];
        struct kinfo_proc info;
        size_t size;

        info.kp_proc.p_flag = 0;
        name[0] = CTL_KERN;
        name[1] = KERN_PROC;
        name[2] = KERN_PROC_PID;
        name[3] = getpid();

        size = sizeof(info);
        if (sysctl(name, 4, &info, &size, NULL, 0) == -1) {
            return false;
        }

        return ((info.kp_proc.p_flag & P_TRACED) != 0);
    }

    /**
     * @brief Detects if exception ports have been hijacked (common for LLDB/GDB).
     */
    bool checkMachExceptionPorts() {
        mach_msg_type_number_t count = 0;
        exception_mask_t masks[EXC_TYPES_COUNT];
        mach_port_t ports[EXC_TYPES_COUNT];
        exception_behavior_t behaviors[EXC_TYPES_COUNT];
        thread_state_flavor_t flavors[EXC_TYPES_COUNT];

        kern_return_t kr = task_get_exception_ports(
            mach_task_self(),
            EXC_MASK_ALL,
            masks,
            &count,
            ports,
            behaviors,
            flavors
        );

        if (kr != KERN_SUCCESS) return false;

        for (mach_msg_type_number_t i = 0; i < count; i++) {
            if (MACH_PORT_VALID(ports[i])) {
                // If any port is registered for EXC_BREAKPOINT or EXC_BAD_ACCESS, it's suspicious
                if (masks[i] & (EXC_MASK_BREAKPOINT | EXC_MASK_BAD_ACCESS)) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @brief Scans ARM64 hardware registers for breakpoints (DBGWVR).
     * Ported from Interlock's ARM64 Security Research.
     */
    bool checkHardwareBreakpoints() {
#if defined(__arm64__) || defined(__aarch64__)
        arm_debug_state64_t state;
        mach_msg_type_number_t count = ARM_DEBUG_STATE64_COUNT;
        kern_return_t kr = thread_get_state(mach_thread_self(), ARM_DEBUG_STATE64, (thread_state_t)&state, &count);

        if (kr != KERN_SUCCESS) return false;

        for (int i = 0; i < 16; i++) {
            if (state.__bvr[i] != 0 || state.__bcr[i] != 0) {
                return true;
            }
        }
#endif
        return false;
    }

private:
    AntiDebug() = default;
};

} // namespace interlock::security
