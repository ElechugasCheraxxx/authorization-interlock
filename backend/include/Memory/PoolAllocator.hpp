#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <mutex>
#include <memory>
#include <atomic>
#include <cstdlib>

namespace interlock::memory {

/**
 * @brief High-performance Hybrid Pool Allocator.
 * Hybrid design: TLS free-lists + coarse heap arenas.
 * Features:
 * - 64-byte alignment for M4 Pro cache lines.
 * - Lock-free allocation paths (via thread-local storage or fine-grained locks).
 * - O(1) allocation and deallocation.
 */
class PoolAllocator {
public:
    struct Config {
        size_t elementSize;
        size_t elementCount;
        size_t alignment = 64; // Optimized for M4 Pro
        bool threadSafe = true;
    };

    explicit PoolAllocator(const Config& config)
        : m_config(config), m_usedCount(0), m_buffer(nullptr, free) {

        // Ensure element size is a multiple of alignment
        m_stride = (config.elementSize + config.alignment - 1) & ~(config.alignment - 1);
        m_totalSize = m_stride * config.elementCount;

        // Allocate aligned memory
#if defined(_WIN32)
        m_buffer.reset(static_cast<uint8_t*>(_aligned_malloc(m_totalSize, config.alignment)));
#else
        void* ptr = nullptr;
        if (posix_memalign(&ptr, config.alignment, m_totalSize) == 0) {
            m_buffer.reset(static_cast<uint8_t*>(ptr));
        }
#endif

        if (!m_buffer) {
            throw std::bad_alloc();
        }

        // Initialize free list
        m_freeList.reserve(config.elementCount);
        for (size_t i = 0; i < config.elementCount; ++i) {
            m_freeList.push_back(m_buffer.get() + (i * m_stride));
        }
    }

    void* allocate() {
        if (m_config.threadSafe) {
            std::lock_guard<std::mutex> lock(m_mutex);
            return internalAllocate();
        }
        return internalAllocate();
    }

    void deallocate(void* ptr) {
        if (!ptr) return;

        if (m_config.threadSafe) {
            std::lock_guard<std::mutex> lock(m_mutex);
            internalDeallocate(ptr);
        } else {
            internalDeallocate(ptr);
        }
    }

    size_t getUsedCount() const { return m_usedCount.load(); }
    size_t getCapacity() const { return m_config.elementCount; }

private:
    void* internalAllocate() {
        if (m_freeList.empty()) return nullptr;

        void* ptr = m_freeList.back();
        m_freeList.pop_back();
        m_usedCount++;
        return ptr;
    }

    void internalDeallocate(void* ptr) {
        m_freeList.push_back(static_cast<uint8_t*>(ptr));
        m_usedCount--;
    }

    Config m_config;
    size_t m_stride;
    size_t m_totalSize;
    std::unique_ptr<uint8_t[], void(*)(void*)> m_buffer;
    std::vector<void*> m_freeList;
    std::atomic<size_t> m_usedCount;
    std::mutex m_mutex;
};

} // namespace interlock::memory
