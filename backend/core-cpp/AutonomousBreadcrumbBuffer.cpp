/**
 * @file AutonomousBreadcrumbBuffer.cpp
 * @brief Implementación del Autonomous Breadcrumb Buffer (ABB)
 */

#include "AutonomousBreadcrumbBuffer.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <unistd.h>

namespace interlock {

// ============================================================================
// SINGLETON INSTANCE
// ============================================================================
static AutonomousBreadcrumbBuffer* g_abb_instance = nullptr;

AutonomousBreadcrumbBuffer& AutonomousBreadcrumbBuffer::getInstance(bool production_mode) {
    if (!g_abb_instance) {
        g_abb_instance = new AutonomousBreadcrumbBuffer();
        g_abb_instance->production_mode_ = production_mode;
    }
    return *g_abb_instance;
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================
AutonomousBreadcrumbBuffer::AutonomousBreadcrumbBuffer()
    : buffer_(nullptr), write_index_(0), mmap_fd_(-1), production_mode_(true) {

    if (production_mode_) {
        // Crear o abrir el archivo de memoria compartida
        mmap_fd_ = open(ABB_MMAP_PATH, O_CREAT | O_RDWR, 0644);
        if (mmap_fd_ == -1) {
            std::cerr << "[ABB] Error abriendo " << ABB_MMAP_PATH << std::endl;
            return;
        }

        // Pre-alocar espacio
        if (ftruncate(mmap_fd_, ABB_SIZE) == -1) {
            std::cerr << "[ABB] Error en ftruncate: " << ABB_SIZE << " bytes" << std::endl;
            close(mmap_fd_);
            return;
        }

        // Mapear en memoria
        buffer_ = reinterpret_cast<BreadcrumbEntry*>(
            mmap(nullptr, ABB_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mmap_fd_, 0)
        );

        if (buffer_ == MAP_FAILED) {
            std::cerr << "[ABB] mmap falló" << std::endl;
            buffer_ = nullptr;
            close(mmap_fd_);
            return;
        }

        std::cout << "[ABB] Buffer inicializado en " << ABB_MMAP_PATH 
                  << " (" << (ABB_SIZE / 1024 / 1024) << " MB, "
                  << MAX_ENTRIES << " entradas)" << std::endl;
    } else {
        // Modo en-memoria (para testing)
        buffer_ = new BreadcrumbEntry[MAX_ENTRIES];
        std::memset(buffer_, 0, ABB_SIZE);
    }
}

AutonomousBreadcrumbBuffer::~AutonomousBreadcrumbBuffer() {
    if (buffer_ != nullptr) {
        if (production_mode_) {
            munmap(buffer_, ABB_SIZE);
            if (mmap_fd_ != -1) {
                close(mmap_fd_);
            }
        } else {
            delete[] buffer_;
        }
    }
}

// ============================================================================
// REGISTRAR TRANSICIÓN
// ============================================================================
void AutonomousBreadcrumbBuffer::recordBreadcrumb(
    uint32_t source_id, uint32_t target_id,
    uint64_t func_ptr, uint32_t stack_depth) {

    if (buffer_ == nullptr) {
        return;
    }

    // Obtener índice actual (lock-free)
    uint64_t current_idx = write_index_.fetch_add(1, std::memory_order_relaxed);
    uint64_t entry_idx = current_idx % MAX_ENTRIES;

    // Rellenar entrada
    auto& entry = buffer_[entry_idx];
    entry.timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    entry.source_id = source_id;
    entry.target_id = target_id;
    entry.func_pointer = func_ptr;
    entry.stack_depth = stack_depth;
    entry.thread_id = (uint32_t)std::hash<std::thread::id>{}(std::this_thread::get_id());
    entry.execution_context = 0;  // userspace por defecto

    // Jenkins Hash integrity check (referencia previa seed)
    // We hash the fields excluding the integrity_check itself
    uint32_t hash = 0x04c11db7;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(&entry);
    size_t hash_len = sizeof(BreadcrumbEntry) - sizeof(entry.integrity_check) - sizeof(entry.reserved);

    for (size_t i = 0; i < hash_len; ++i) {
        hash += data[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    entry.integrity_check = hash;

    // Memory fence to ensure data is visible before next atomic operation if any
    std::atomic_thread_fence(std::memory_order_release);

#ifdef DEBUG_ABB
    std::cout << "[ABB] Breadcrumb: " << source_id << " -> " << target_id 
              << " @ 0x" << std::hex << func_ptr << std::dec << " [Hash: " << std::hex << hash << "]" << std::endl;
#endif
}

// ============================================================================
// DUMP DEL BUFFER
// ============================================================================
size_t AutonomousBreadcrumbBuffer::dumpBuffer(BreadcrumbEntry* output, size_t count) const {
    if (buffer_ == nullptr || output == nullptr) {
        return 0;
    }

    uint64_t current_idx = write_index_.load(std::memory_order_acquire);
    size_t entries_to_read = std::min(count, static_cast<size_t>(current_idx));

    for (size_t i = 0; i < entries_to_read; i++) {
        uint64_t read_idx = (current_idx - entries_to_read + i) % MAX_ENTRIES;
        std::memcpy(&output[i], &buffer_[read_idx], sizeof(BreadcrumbEntry));
    }

    return entries_to_read;
}

// ============================================================================
// RESET
// ============================================================================
void AutonomousBreadcrumbBuffer::reset() {
    if (buffer_ != nullptr) {
        std::memset(buffer_, 0, ABB_SIZE);
        write_index_.store(0, std::memory_order_release);
    }
}

// ============================================================================
// BREADCRUMB SCOPE (RAII)
// ============================================================================
BreadcrumbScope::BreadcrumbScope(uint32_t source, uint32_t target, uint64_t func_ptr)
    : source_(source), target_(target), func_ptr_(func_ptr) {
    auto& abb = AutonomousBreadcrumbBuffer::getInstance();
    abb.recordBreadcrumb(source, target, func_ptr, 0);
}

BreadcrumbScope::~BreadcrumbScope() {
    auto& abb = AutonomousBreadcrumbBuffer::getInstance();
    abb.recordBreadcrumb(target_, source_, func_ptr_ | 0x8000000000000000ULL, 0);
}

} // namespace interlock
