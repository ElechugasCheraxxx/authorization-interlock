#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Genera un stub polimórfico usando el motor Zig
 * @param buffer Buffer de salida para el shellcode
 * @param buffer_len Tamaño del buffer
 * @param seed Semilla para el polimorfismo
 * @return Tamaño del artifact generado
 */
uint32_t zig_generate_polymorphic_stub(uint8_t* buffer, uint32_t buffer_len, uint32_t seed);

/**
 * @brief Alocador de memoria sigiloso (Zig Native)
 * Evita patrones de alocación estándar detectables por EDR.
 */
void* zig_stealth_alloc(size_t size);

/**
 * @brief Libera memoria alocada con zig_stealth_alloc
 */
void zig_stealth_free(void* ptr, size_t size);

#ifdef __cplusplus
}
#endif
