#pragma once
#include <mach/mach_types.h>

namespace interlock::mach {

enum class TaskPriority {
    BACKGROUND,    // E-Cores (Efficiency)
    NORMAL,        // Balanced
    HIGH,          // P-Cores (Performance)
    REALTIME       // Maximum priority, fixed quantum
};

/**
 * @brief Configura la afinidad y prioridad de un thread para optimizar el uso de P-Cores en M4 Pro.
 * @param thread_port El puerto del thread (mach_thread_self() por defecto si es 0).
 * @param priority Nivel de prioridad deseado.
 */
void SetThreadPriority(thread_port_t thread_port, TaskPriority priority);

/**
 * @brief Asigna un thread a un set de afinidad específico para mejorar la localidad de caché L2.
 * @param thread_port El puerto del thread.
 * @param tag Identificador de afinidad (threads con el mismo tag intentan compartir core/cluster).
 */
void SetThreadAffinity(thread_port_t thread_port, int tag);

} // namespace interlock::mach
