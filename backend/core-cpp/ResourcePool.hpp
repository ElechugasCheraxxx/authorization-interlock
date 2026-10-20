#ifndef INTERLOCK_TACTICAL_POOL_HPP
#define INTERLOCK_TACTICAL_POOL_HPP

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <mutex>
#include <optional>

namespace interlock {

/**
 * @brief Modern C++23 implementation of the GTA-style memory pool.
 * Optimized for high-frequency allocation/deallocation of agents and sessions.
 */
template<typename T, size_t MaxSize = 1024>
class ResourcePool {
public:
    ResourcePool() : m_top(0) {
        m_objects = static_cast<T*>(operator new[](MaxSize * sizeof(T)));
        m_flags.assign(MaxSize, 0x80); // 0x80 means free
    }

    ~ResourcePool() {
        for (size_t i = 0; i < MaxSize; ++i) {
            if (!(m_flags[i] & 0x80)) {
                m_objects[i].~T();
            }
        }
        operator delete[](m_objects);
    }

    template<typename... Args>
    T* construct(Args&&... args) {
        std::lock_guard<std::mutex> lock(m_mutex);

        size_t start = m_top;
        do {
            if (m_flags[m_top] & 0x80) {
                T* ptr = new (&m_objects[m_top]) T(std::forward<Args>(args)...);
                m_flags[m_top] &= 0x7F; // Mark as used
                m_flags[m_top]++;      // Increment version/identifier bit if needed

                size_t allocated_idx = m_top;
                m_top = (m_top + 1) % MaxSize;
                return ptr;
            }
            m_top = (m_top + 1) % MaxSize;
        } while (m_top != start);

        return nullptr; // Pool exhausted
    }

    void destroy(T* object) {
        if (!object) return;

        std::lock_guard<std::mutex> lock(m_mutex);
        size_t idx = object - m_objects;

        if (idx >= MaxSize || (m_flags[idx] & 0x80)) {
            return; // Already free or out of bounds
        }

        m_objects[idx].~T();
        m_flags[idx] |= 0x80; // Mark as free
        m_top = idx; // Optimization: try to reuse this immediately
    }

    T* get(size_t index) {
        if (index >= MaxSize || (m_flags[index] & 0x80)) return nullptr;
        return &m_objects[index];
    }

private:
    T* m_objects;
    std::vector<uint8_t> m_flags;
    size_t m_top;
    std::mutex m_mutex;
};

} // namespace interlock

#endif // INTERLOCK_TACTICAL_POOL_HPP
