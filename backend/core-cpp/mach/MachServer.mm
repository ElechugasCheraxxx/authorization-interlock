#import <Foundation/Foundation.h>
#include <mach/mach.h>
#include <iostream>
#include <vector>
#include <pthread.h>
#include "mach/MachServer.hpp"
#include "mach/MachPriority.hpp"
#include "interlock_ipc.h" // Generado por Mig

extern "C" boolean_t interlock_ipc_server(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

/*
 * MachServer.mm - Implementación del servidor Mach IPC para la orquesta Interlock.
 * Sustituye gradualmente a los Sockets Unix con comunicación nativa de Kernel.
 */

extern "C" kern_return_t interlock_dispatch_tactical_node(
    mach_port_t server_port,
    int sender_id,
    int msg_type,
    int priority,
    char* artifact,
    mach_msg_type_number_t payloadCnt)
{
    // std::cout << "[Mach-IPC] Operations Dispatch from " << sender_id << " type " << msg_type << std::endl;
    return KERN_SUCCESS;
}

extern "C" kern_return_t interlock_fetch_blackboard_state(
    mach_port_t server_port,
    char* state_data,
    mach_msg_type_number_t *state_dataCnt)
{
    // Simulación de retorno de estado del Blackboard
    return KERN_SUCCESS;
}

extern "C" kern_return_t interlock_signal_escalation(
    mach_port_t server_port,
    int escalation_lvl,
    int reason_code)
{
    std::cout << "[Mach-IPC] !!! ESCALATION SIGNAL RECEIVED: Level " << escalation_lvl << std::endl;
    return KERN_SUCCESS;
}

static mach_port_t g_main_server_port = MACH_PORT_NULL;

void* MachServerMainLoop(void* arg) {
    mach_port_t port = (mach_port_t)(uintptr_t)arg;

    // Optimizar para P-Cores (M4 Pro)
    interlock::mach::SetThreadPriority(mach_thread_self(), interlock::mach::TaskPriority::HIGH);
    interlock::mach::SetThreadAffinity(mach_thread_self(), 1); // Cluster 1 (P-Cores)

    std::cout << "[Mach-IPC] Main Server Loop active on port " << port << " [P-Core Affinity]" << std::endl;

    // Despachador generado por Mig
    while (true) {
        kern_return_t kr = mach_msg_server(interlock_ipc_server, 4096, port, 0);
        if (kr != KERN_SUCCESS) {
            std::cerr << "[Mach-IPC] Server error: " << mach_error_string(kr) << std::endl;
            break;
        }
    }
    return nullptr;
}

namespace interlock::mach {

bool StartMachServer() {
    kern_return_t kr;

    // 1. Reservar puerto con derechos de recepción
    kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &g_main_server_port);
    if (kr != KERN_SUCCESS) return false;

    // 2. Insertar derecho de envío (para que otros puedan hablarnos)
    kr = mach_port_insert_right(mach_task_self(), g_main_server_port, g_main_server_port, MACH_MSG_TYPE_MAKE_SEND);
    if (kr != KERN_SUCCESS) return false;

    // 3. Registrar puerto en el bootstrap server (opcional para visibilidad externa,
    // pero preferimos intercambio directo de derechos para sigilo)

    // 4. Lanzar thread de servicio
    pthread_t thread;
    pthread_create(&thread, nullptr, MachServerMainLoop, (void*)(uintptr_t)g_main_server_port);
    pthread_detach(thread);

    return true;
}

mach_port_t GetServerPort() {
    return g_main_server_port;
}

} // namespace interlock::mach
