#import <Foundation/Foundation.h>
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#include <iostream>
#include "mach/MachPriority.hpp"

namespace interlock::mach {

void SetThreadPriority(thread_port_t thread_port, TaskPriority priority) {
    if (thread_port == 0) {
        thread_port = mach_thread_self();
    }

    kern_return_t kr;

    // 1. Configurar precedencia (Prioridad relativa)
    thread_precedence_policy_data_t prec_policy;
    switch (priority) {
        case TaskPriority::BACKGROUND: prec_policy.importance = -10; break;
        case TaskPriority::NORMAL:     prec_policy.importance = 0; break;
        case TaskPriority::HIGH:       prec_policy.importance = 10; break;
        case TaskPriority::REALTIME:   prec_policy.importance = 63; break;
    }

    kr = thread_policy_set(thread_port, THREAD_PRECEDENCE_POLICY,
                           (thread_policy_t)&prec_policy, THREAD_PRECEDENCE_POLICY_COUNT);

    if (kr != KERN_SUCCESS) {
        std::cerr << "[Mach-Priority] Failed to set precedence: " << kr << std::endl;
    }

    // 2. Configurar QoS (Quality of Service) para sugerir cores al planificador de XNU
    // En Apple Silicon, esto mapea directamente a P-Cores (Performance) o E-Cores (Efficiency)
    if (priority == TaskPriority::HIGH || priority == TaskPriority::REALTIME) {
        pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    } else if (priority == TaskPriority::BACKGROUND) {
        pthread_set_qos_class_self_np(QOS_CLASS_BACKGROUND, 0);
    } else {
        pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0);
    }

    // 3. Para Real-Time, usar política de tiempo real de Mach
    if (priority == TaskPriority::REALTIME) {
        thread_time_constraint_policy_data_t tc_policy;
        // Valores orientativos para M4 Pro (ajustar según perfiles de carga)
        tc_policy.period      = 10 * 1000 * 1000; // 10ms
        tc_policy.computation = 2 * 1000 * 1000;  // 2ms
        tc_policy.constraint  = 5 * 1000 * 1000;  // 5ms
        tc_policy.preemptible = TRUE;

        kr = thread_policy_set(thread_port, THREAD_TIME_CONSTRAINT_POLICY,
                               (thread_policy_t)&tc_policy, THREAD_TIME_CONSTRAINT_POLICY_COUNT);
        if (kr != KERN_SUCCESS) {
            std::cerr << "[Mach-Priority] Failed to set RT constraint: " << kr << std::endl;
        }
    }
}

void SetThreadAffinity(thread_port_t thread_port, int tag) {
    if (thread_port == 0) {
        thread_port = mach_thread_self();
    }

    thread_affinity_policy_data_t policy;
    policy.affinity_tag = tag;

    kern_return_t kr = thread_policy_set(thread_port, THREAD_AFFINITY_POLICY,
                                         (thread_policy_t)&policy, THREAD_AFFINITY_POLICY_COUNT);
    if (kr != KERN_SUCCESS) {
        std::cerr << "[Mach-Priority] Failed to set affinity tag " << tag << ": " << kr << std::endl;
    }
}

} // namespace interlock::mach
