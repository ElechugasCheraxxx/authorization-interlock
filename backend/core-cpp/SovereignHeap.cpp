#include "../include/SovereignHeap.hpp"
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <cassert>
#include <algorithm>

namespace interlock::memory {

void SovereignHeap::init(size_t total_size) {
    std::lock_guard<std::mutex> lock(mtx);

    // Usamos MAP_ANONYMOUS | MAP_PRIVATE para que no sea visible en el sistema de archivos
    // pero sea persistente en el espacio de direccionamiento del proceso.
    void* mem = mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (mem == MAP_FAILED) {
        return;
    }

    start = reinterpret_cast<HeapBlock*>(mem);
    start->size = total_size - sizeof(HeapBlock);
    start->id = MemoryID::FREE;
    start->ptr_index = -1;
    start->next = nullptr;
    start->prev = nullptr;

    end = start; // En este modelo simplificado, el final es el último bloque
    total_used = 0;
}

void* SovereignHeap::malloc_registered(size_t size, void** back_ptr, MemoryID id) {
    std::lock_guard<std::mutex> lock(mtx);

    // Alineación a 16 bytes (inspirado en re3)
    if (size % 16 != 0) {
        size = (size & ~15) + 16;
    }

    HeapBlock* block = find_free_block(size);
    if (!block) return nullptr;

    // Si sobra espacio suficiente, dividimos el bloque
    if (block->size > size + sizeof(HeapBlock) + 16) {
        HeapBlock* next_block = reinterpret_cast<HeapBlock*>(reinterpret_cast<uint8_t*>(block->data()) + size);
        next_block->size = block->size - size - sizeof(HeapBlock);
        next_block->id = MemoryID::FREE;
        next_block->ptr_index = -1;
        next_block->next = block->next;
        next_block->prev = block;

        if (block->next) block->next->prev = next_block;
        block->next = next_block;
        block->size = size;
    }

    block->id = id;
    block->ptr_index = next_ptr_index++;
    registered_ptrs[block->ptr_index] = back_ptr;
    *back_ptr = block->data();

    total_used += block->size + sizeof(HeapBlock);
    return block->data();
}

void SovereignHeap::free_registered(void* ptr) {
    if (!ptr) return;
    std::lock_guard<std::mutex> lock(mtx);

    HeapBlock* block = reinterpret_cast<HeapBlock*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(HeapBlock));

    if (block->ptr_index != -1) {
        registered_ptrs.erase(block->ptr_index);
    }

    block->id = MemoryID::FREE;
    block->ptr_index = -1;
    total_used -= (block->size + sizeof(HeapBlock));

    combine_free_blocks(block);
}

HeapBlock* SovereignHeap::find_free_block(size_t size) {
    HeapBlock* curr = start;
    while (curr) {
        if (curr->id == MemoryID::FREE && curr->size >= size) {
            return curr;
        }
        curr = curr->next;
    }
    return nullptr;
}

void SovereignHeap::combine_free_blocks(HeapBlock* block) {
    // Combinar con el siguiente
    if (block->next && block->next->id == MemoryID::FREE) {
        block->size += block->next->size + sizeof(HeapBlock);
        block->next = block->next->next;
        if (block->next) block->next->prev = block;
    }
    // Combinar con el anterior
    if (block->prev && block->prev->id == MemoryID::FREE) {
        block->prev->size += block->size + sizeof(HeapBlock);
        block->prev->next = block->next;
        if (block->next) block->next->prev = block->prev;
    }
}

void SovereignHeap::tidy() {
    std::lock_guard<std::mutex> lock(mtx);
    // Lógica de desfragmentación: Mueve bloques ocupados hacia el inicio
    // Este es un proceso costoso pero vital para Anti-Forensics y estabilidad.
    HeapBlock* curr = start;
    while (curr) {
        if (curr->id != MemoryID::FREE && curr->prev && curr->prev->id == MemoryID::FREE) {
            HeapBlock* free_block = curr->prev;
            void** registered_ptr = (curr->ptr_index != -1) ? registered_ptrs[curr->ptr_index] : nullptr;

            size_t move_size = curr->size;
            MemoryID move_id = curr->id;
            int32_t move_idx = curr->ptr_index;

            // Intercambiamos los datos (simplificado: asume que el bloque libre es >= que el ocupado)
            // En una implementación real de re3, se maneja el desplazamiento parcial.
            uint8_t* old_data = reinterpret_cast<uint8_t*>(curr->data());

            // Re-estructuramos el bloque ocupado en la posición del libre
            free_block->id = move_id;
            free_block->ptr_index = move_idx;
            // Ajustamos tamaños si el bloque libre era más grande
            if (free_block->size > move_size + sizeof(HeapBlock)) {
                // Crear un nuevo bloque libre después del movido
                HeapBlock* new_free = reinterpret_cast<HeapBlock*>(reinterpret_cast<uint8_t*>(free_block->data()) + move_size);
                new_free->size = free_block->size - move_size - sizeof(HeapBlock);
                new_free->id = MemoryID::FREE;
                new_free->ptr_index = -1;
                new_free->next = free_block->next;
                new_free->prev = free_block;
                if (free_block->next) free_block->next->prev = new_free;
                free_block->next = new_free;
                free_block->size = move_size;
            }

            memmove(free_block->data(), old_data, move_size);

            if (registered_ptr) {
                *registered_ptr = free_block->data();
            }

            // El bloque viejo ahora es libre (será combinado en la siguiente iteración o por combine)
            curr = free_block;
        }
        curr = curr->next;
    }
}

} // namespace interlock::memory
