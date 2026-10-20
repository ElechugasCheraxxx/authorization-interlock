/**
 * @file AutonomousBreadcrumbBuffer.hpp
 * @brief Autonomous Breadcrumb Buffer (ABB) - Lock-free circular buffer para debugging inter-lenguajes
 * 
 * Propósito: Registrar transiciones entre fronteras de lenguajes (C++ -> Swift -> ASM)
 * para auditoría y depuración en entornos de producción.
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <atomic>

namespace interlock {

/**
 * @struct BreadcrumbEntry
 * @brief Entrada en el buffer circular ABB
 */
struct BreadcrumbEntry {
    uint64_t timestamp;        // Timestamp UNIX (nanosegundos)
    uint64_t func_pointer;     // Dirección de la función/syscall
    uint32_t source_id;        // ID del componente origen
    uint32_t target_id;        // ID del componente destino
    uint32_t stack_depth;      // Profundidad del stack
    uint32_t thread_id;        // ID del thread
    uint32_t integrity_check;  // Jenkins Hash integrity (referencia previa seed)
    uint8_t  execution_context; // Contexto (0=userspace, 1=kernel)
    uint8_t  reserved[27];     // Padding to reach 64 bytes
} __attribute__((aligned(64)));

/**
 * @class AutonomousBreadcrumbBuffer
 * @brief Buffer circular lock-free para trazabilidad inter-lenguajes
 * 
 * Características:
 * - Acceso sin locks (lock-free) mediante operaciones atómicas
 * - Mapeado en memoria compartida para persistencia post-crash
 * - Circular, sobrescribe entradas antiguas
 * - Tamaño: 256 MB (ajustable en compile-time)
 */
class AutonomousBreadcrumbBuffer {
public:
    static constexpr size_t ABB_SIZE = 256 * 1024 * 1024;  // 256 MB
    static constexpr size_t MAX_ENTRIES = ABB_SIZE / sizeof(BreadcrumbEntry);
    static constexpr const char* ABB_MMAP_PATH = "/dev/shm/interlock_abb_prod";

    /**
     * @brief Inicializa el ABB
     * @param production_mode Si es true, usa mmap para persistencia
     */
    static AutonomousBreadcrumbBuffer& getInstance(bool production_mode = true);

    /**
     * @brief Registra una transición entre componentes
     * @param source_id ID del componente origen
     * @param target_id ID del componente destino
     * @param func_ptr Dirección de la función/syscall
     * @param stack_depth Profundidad del stack
     */
    void recordBreadcrumb(uint32_t source_id, uint32_t target_id,
                         uint64_t func_ptr, uint32_t stack_depth);

    /**
     * @brief Lee el contenido del buffer para análisis post-mortem
     * @param output Buffer de salida
     * @param count Número máximo de entradas a leer
     * @return Número de entradas leídas
     */
    size_t dumpBuffer(BreadcrumbEntry* output, size_t count) const;

    /**
     * @brief Limpia el buffer (reset)
     */
    void reset();

private:
    AutonomousBreadcrumbBuffer();
    ~AutonomousBreadcrumbBuffer();

    // Prohibir copia/movimiento
    AutonomousBreadcrumbBuffer(const AutonomousBreadcrumbBuffer&) = delete;
    AutonomousBreadcrumbBuffer& operator=(const AutonomousBreadcrumbBuffer&) = delete;

    BreadcrumbEntry* buffer_;
    std::atomic<uint64_t> write_index_;
    int mmap_fd_;
    bool production_mode_;
};

/**
 * @class BreadcrumbScope
 * @brief RAII wrapper para registrar automáticamente transiciones
 * 
 * Uso:
 * {
 *     BreadcrumbScope scope(COMPONENT_SWIFT, COMPONENT_CPP, (uint64_t)&my_function);
 *     // Función se ejecuta...
 * } // Breadcrumb se registra al salir del scope
 */
class BreadcrumbScope {
public:
    BreadcrumbScope(uint32_t source, uint32_t target, uint64_t func_ptr);
    ~BreadcrumbScope();

private:
    uint32_t source_;
    uint32_t target_;
    uint64_t func_ptr_;
};

// ============================================================================
// IDs DE COMPONENTES PARA TRAZABILIDAD
// ============================================================================
enum ComponentID : uint32_t {
    COMPONENT_UNKNOWN      = 0,
    COMPONENT_CPP_CORE     = 1,
    COMPONENT_SWIFT        = 2,
    COMPONENT_RUST_CIPHER  = 3,
    COMPONENT_GO_ATLAS     = 4,
    COMPONENT_ZIG_PAYLOAD  = 5,
    COMPONENT_C_SHIELD     = 6,
    COMPONENT_ASM_STUB     = 7,
    COMPONENT_TS_INTEL     = 8,
    COMPONENT_PERL_SURGERY = 9,
};

} // namespace interlock
