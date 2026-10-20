#include <mach/mach.h>
#include <mach/task.h>
#include <mach/thread_act.h>
#include <iostream>
#include "RiskManager.hpp"

namespace interlock::mach {

/**
 * Esta función escanea todos los threads del proceso buscando Hardware Breakpoints
 * colocados por un debugger (registros DR0-DR7 en x86, o DBGWVR/DBGWCR en ARM64).
 */
bool ScanForHardwareBreakpoints() {
    thread_act_array_t thread_list;
    mach_msg_type_number_t thread_count;

    kern_return_t kr = task_threads(mach_task_self(), &thread_list, &thread_count);
    if (kr != KERN_SUCCESS) return false;

    bool detected = false;
    for (mach_msg_type_number_t i = 0; i < thread_count; i++) {
        arm_debug_state64_t state;
        mach_msg_type_number_t count = ARM_DEBUG_STATE64_COUNT;

        kr = thread_get_state(thread_list[i], ARM_DEBUG_STATE64, (thread_state_t)&state, &count);
        if (kr == KERN_SUCCESS) {
            // En ARM64, si algún bit en los registros de control de breakpoint está activo, hay un BP.
            for (int b = 0; b < 16; b++) {
                if (state.__bcr[b] & 0x1) {
                    detected = true;
                    break;
                }
            }
        }
        mach_port_deallocate(mach_task_self(), thread_list[i]);
    }

    vm_deallocate(mach_task_self(), (vm_address_t)thread_list, thread_count * sizeof(mach_port_t));
    return detected;
}

} // namespace interlock::mach
