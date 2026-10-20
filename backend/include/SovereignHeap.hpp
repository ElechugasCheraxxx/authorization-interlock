/**
 * @file SovereignHeap.hpp
 * @brief Hardened Memory Manager inspirado en CMemoryHeap (re3)
 *
 * Implementa un heap aislado con capacidad de desfragmentación dinámica
 * y registro de punteros para movimiento de memoria (Anti-Forensics).
 */

#pragma once

#include <cstdint>
#include <vector>
#include <mutex>
#include <map>

namespace interlock::memory {

enum class MemoryID : uint8_t {
    FREE = 0,
    RAG_ENGINE = 1,
    ABB_BUFFER = 2,
    CIPHER_KEYS = 3,
    SHELLCODE_STAGING = 4,
    FORMAL_PROOFS = 5
};

struct HeapBlock {
    uint32_t size;
    MemoryID id;
    int32_t ptr_index; // Índice en la tabla de punteros registrados
    HeapBlock *next;
    HeapBlock *prev;

    void* data() { return reinterpret_cast<void*>(this + 1); }
};

/**
 * @class SovereignHeap
 * @brief Asignador de memoria con capacidad de reubicación (Zero-Fragmentation)
 */
class SovereignHeap {
public:
    static SovereignHeap& getInstance() {
        static SovereignHeap instance;
        return instance;
    }

    void init(size_t total_size);

    /**
     * @brief Asigna memoria y registra el puntero para permitir su movimiento posterior.
     */
    void* malloc_registered(size_t size, void** back_ptr, MemoryID id);
    void free_registered(void* ptr);

    /**
     * @brief Desfragmenta el heap moviendo los bloques registrados (TidyHeap).
     * Esto invalida los punteros físicos pero actualiza los registrados automáticamente.
     */
    void tidy();

    size_t get_used_memory() const { return total_used; }

private:
    SovereignHeap() : start(nullptr), end(nullptr), total_used(0) {}

    HeapBlock* start;
    HeapBlock* end;
    size_t total_used;
    std::mutex mtx;

    // Tabla de punteros registrados (indirección para permitir movimiento)
    std::map<int32_t, void**> registered_ptrs;
    int32_t next_ptr_index = 0;

    HeapBlock* find_free_block(size_t size);
    void combine_free_blocks(HeapBlock* block);
};

} // namespace interlock::memory
