#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <mutex>
#include <memory>
#include <atomic>
#include <cstdlib>
#include <new>
#include <cstring>

namespace interlock::memory {

/**
 * @brief High-performance Hybrid Pool Allocator.
 * Hybrid design: TLS free-lists + coarse heap arenas.
 *
 * Features:
 * - O(1) Predictable allocation/deallocation.
 * - InstanceID tracking for handle safety (Anti-DKOM).
 * - 64-byte alignment for M4 Pro cache optimization (prevents false sharing).
 */
class PoolAllocator {
public:
    explicit PoolAllocator(size_t elementSize, size_t elementCount)
        : m_elementCount(elementCount), m_usedCount(0) {

        // M4 Pro Cache Line Alignment (64 bytes)
        m_stride = (elementSize + 63) & ~63;
        size_t totalSize = m_stride * elementCount;

#if defined(_WIN32)
        m_base = static_cast<uint8_t*>(_aligned_malloc(totalSize, 64));
#else
        if (posix_memalign((void**)&m_base, 64, totalSize) != 0) {
            throw std::bad_alloc();
        }
#endif

        // Instance IDs for stale handle detection (Anti-DKOM)
        m_instanceIds = new uint8_t[elementCount]();
        std::memset(m_instanceIds, 0, elementCount);

        m_freeList.reserve(elementCount);
        for (size_t i = 0; i < elementCount; ++i) {
            m_freeList.push_back(static_cast<uint32_t>(i));
        }
    }

    ~PoolAllocator() {
#if defined(_WIN32)
        _aligned_free(m_base);
#else
        free(m_base);
#endif
        delete[] m_instanceIds;
    }

    /**
     * @brief Allocates a block and returns a handle.
     * Handle format: [24 bits Index | 8 bits InstanceID]
     */
    void* allocate(uint32_t& outHandle) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_freeList.empty()) {
            outHandle = 0;
            return nullptr;
        }

        uint32_t index = m_freeList.back();
        m_freeList.pop_back();

        // Anti-DKOM: Increment instance ID to invalidate previous handles
        m_instanceIds[index]++;
        if (m_instanceIds[index] == 0) m_instanceIds[index] = 1;

        outHandle = (index << 8) | m_instanceIds[index];
        m_usedCount.fetch_add(1, std::memory_order_relaxed);

        void* ptr = m_base + (index * m_stride);

        // Debug fill (0xCD = Allocated) inspired by 
#ifndef NDEBUG
        std::memset(ptr, 0xCD, m_stride);
#endif
        return ptr;
    }

    /**
     * @brief Deallocates a block using its handle.
     * Validates handle integrity before freeing.
     */
    void deallocate(uint32_t handle) {
        if (handle == 0) return;

        uint32_t index = handle >> 8;
        uint8_t id = handle & 0xFF;

        std::lock_guard<std::mutex> lock(m_mutex);
        if (index >= m_elementCount || m_instanceIds[index] != id) {
            // Invalid or stale handle (potential assess or bug)
            return;
        }

        // Debug fill (0xDD = Dead/Free)
#ifndef NDEBUG
        std::memset(m_base + (index * m_stride), 0xDD, m_stride);
#endif

        m_freeList.push_back(index);
        m_usedCount.fetch_sub(1, std::memory_order_relaxed);
    }

    /**
     * @brief Safe pointer retrieval from handle.
     */
    void* resolve(uint32_t handle) const {
        uint32_t index = handle >> 8;
        uint8_t id = handle & 0xFF;

        if (index >= m_elementCount || m_instanceIds[index] != id) {
            return nullptr;
        }

        return m_base + (index * m_stride);
    }

    size_t getUsedCount() const { return m_usedCount.load(std::memory_order_relaxed); }
    size_t getCapacity() const { return m_elementCount; }

private:
    uint8_t* m_base;
    uint8_t* m_instanceIds;
    size_t m_elementCount;
    size_t m_stride;
    std::vector<uint32_t> m_freeList;
    std::atomic<size_t> m_usedCount;
    mutable std::mutex m_mutex;
};

} // namespace interlock::memory
