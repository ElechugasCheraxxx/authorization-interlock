#import <Foundation/Foundation.h>
#include <mach/mach.h>
#include <mach/exc.h>
#include <pthread.h>
#include <iostream>
#include "anti_debug.h" // Generado por Mig

extern "C" boolean_t anti_debug_server(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

/*
 * Implementación de las rutinas del servidor definidas en anti_debug.defs
 * Estas funciones son llamadas por mach_msg_server cuando llega un mensaje al puerto.
 */

extern "C" kern_return_t anti_debug_report_violation(
    mach_port_t server_port,
    int violator_pid,
    int violation_type,
    int severity)
{
    std::cerr << "[Mach-Protector] !!! SECURITY VIOLATION DETECTED !!!" << std::endl;
    std::cerr << "[Mach-Protector] Type: " << violation_type << " from PID: " << violator_pid << std::endl;

    // Aquí podríamos disparar el Self-Destruct del RiskManager
    return KERN_SUCCESS;
}

extern "C" kern_return_t anti_debug_set_stealth_mode(
    mach_port_t server_port,
    boolean_t enabled)
{
    std::cout << "[Mach-Protector] Integrity mode set to: " << (enabled ? "ON" : "OFF") << std::endl;
    return KERN_SUCCESS;
}

extern "C" kern_return_t anti_debug_heartbeat(
    mach_port_t server_port,
    uint64_t integrity_token)
{
    // Verificar integridad del token (anti-tamper)
    return KERN_SUCCESS;
}

/* --- Monitor de Excepciones Mach (El núcleo del Anti-Debug) --- */

static mach_port_t exception_port = MACH_PORT_NULL;

void* MachExceptionThread(void* arg) {
    std::cout << "[Mach-Protector] Exception Monitor Thread Active." << std::endl;

    // Loop de escucha para mensajes de excepción Mach
    // En una implementación real de producción, aquí manejaríamos EXC_BREAKPOINT, EXC_BAD_ACCESS, etc.
    // Si recibimos uno inesperado, significa que alguien está intentando debuggear o forzar un crash.

    while (true) {
        // Usamos mach_msg_server para despachar los mensajes de nuestro subsistema custom
        mach_msg_server(anti_debug_server, 1024, (mach_port_t)(uintptr_t)arg, 0);
    }
    return nullptr;
}

namespace interlock::mach {

void StartAntiDebugMonitor() {
    kern_return_t kr;

    // 1. Reservar puerto para el monitor
    kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &exception_port);
    if (kr != KERN_SUCCESS) return;

    kr = mach_port_insert_right(mach_task_self(), exception_port, exception_port, MACH_MSG_TYPE_MAKE_SEND);
    if (kr != KERN_SUCCESS) return;

    // 2. Establecer este puerto como el manejador de excepciones del proceso
    // Esto intercepta intentos de debuggers (LLDB/GDB) de capturar excepciones
    kr = task_set_exception_ports(
        mach_task_self(),
        EXC_MASK_ALL,
        exception_port,
        EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES,
        THREAD_STATE_NONE
    );

    if (kr == KERN_SUCCESS) {
        std::cout << "[Mach-Protector] Native Exception Shield Engaged." << std::endl;

        // 3. Lanzar thread de monitoreo
        pthread_t thread;
        pthread_create(&thread, nullptr, MachExceptionThread, (void*)(uintptr_t)exception_port);
        pthread_detach(thread);
    } else {
        std::cerr << "[Mach-Protector] Failed to engage Exception Shield: " << kr << std::endl;
    }
}

} // namespace interlock::mach
