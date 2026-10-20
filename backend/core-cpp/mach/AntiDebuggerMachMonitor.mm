#import <Foundation/Foundation.h>
#include <mach/mach.h>
#include <mach/task.h>
#include <mach/thread_act.h>
#include <mach/exception_types.h>
#include <pthread.h>
#include <iostream>
#include "mach/MachProtector.hpp"
#include "EventSystem.hpp"
#include "RiskManager.hpp"

namespace interlock::mach {

static mach_port_t g_exception_port = MACH_PORT_NULL;

/**
 * El thread de protección que escucha por excepciones Mach.
 * Al secuestrar el puerto de excepciones del proceso, LLDB no puede recibir
 * notificaciones de breakpoints o crashes, efectivamente bloqueándolo.
 */
void* AntiDebugThread(void* arg) {
    mach_msg_header_t* msg = (mach_msg_header_t*)malloc(1024);

    std::cout << "[AntiDebug] Mach Exception Monitor active on port " << g_exception_port << std::endl;

    while (true) {
        // Bloquear esperando un mensaje de excepción
        kern_return_t kr = mach_msg(msg, MACH_RCV_MSG, 0, 1024, g_exception_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);

        if (kr == KERN_SUCCESS) {
            std::cerr << "[AntiDebug] !!! DETECTED DEBUG EXCEPTION !!! Intercepting and neutralizing..." << std::endl;

            // Trigger the crash induction ported from referencia previa via RiskManager
            RiskManager::getInstance().reportDeviation("MachMonitor", "Debug Exception Detected", 10);
            RiskManager::getInstance().induceCrash(false);

            // Simplemente no reenviamos la excepción al debugger.
            // Esto causa que el debugger se cuelgue o pierda el rastro del proceso.
        }
    }
    return nullptr;
}

void StartAntiDebugMonitor() {
    kern_return_t kr;

    // 1. Crear un nuevo puerto de excepción
    kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &g_exception_port);
    if (kr != KERN_SUCCESS) return;

    kr = mach_port_insert_right(mach_task_self(), g_exception_port, g_exception_port, MACH_MSG_TYPE_MAKE_SEND);
    if (kr != KERN_SUCCESS) return;

    // 2. Establecer este puerto como el manejador de excepciones para el TASK actual
    // Interceptamos EXC_BREAKPOINT y EXC_BAD_ACCESS (comúnmente usado para trampas anti-debug)
    kr = task_set_exception_ports(
        mach_task_self(),
        EXC_MASK_BREAKPOINT | EXC_MASK_BAD_ACCESS | EXC_MASK_CRASH,
        g_exception_port,
        EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES,
        THREAD_STATE_NONE
    );

    if (kr == KERN_SUCCESS) {
        pthread_t thread;
        pthread_create(&thread, NULL, AntiDebugThread, NULL);
        pthread_detach(thread);
    } else {
        std::cerr << "[AntiDebug] Failed to set exception ports: " << kr << std::endl;
    }
}

} // namespace interlock::mach
