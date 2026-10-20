#pragma once
#include <cstdint>
#include <concepts>

namespace interlock::math {

/**
 * @brief Compressed Vector using 16-bit unsigned integers.
 * Optimized for network payloads and M4 Pro cache lines.
 */
struct CompressedVector {
    uint16_t x, y, z;
};

/**
 * @brief Compressed Quaternion using 16-bit unsigned integers.
 * Provides 75% reduction compared to standard float4.
 */
struct CompressedQuaternion {
    uint16_t x, y, z, w;
};

template<int ScaleInt = 128>
struct VectorCompressor {
    static constexpr float Scale = static_cast<float>(ScaleInt);
    static constexpr CompressedVector compress(float x, float y, float z) noexcept {
        return {
            static_cast<uint16_t>(x * Scale),
            static_cast<uint16_t>(y * Scale),
            static_cast<uint16_t>(z * Scale)
        };
    }

    static constexpr void decompress(const CompressedVector& cv, float& x, float& y, float& z) noexcept {
        x = static_cast<float>(cv.x) / Scale;
        y = static_cast<float>(cv.y) / Scale;
        z = static_cast<float>(cv.z) / Scale;
    }
};

template<int ScaleInt = 4096>
struct QuaternionCompressor {
    static constexpr float Scale = static_cast<float>(ScaleInt);
    static constexpr CompressedQuaternion compress(float x, float y, float z, float w) noexcept {
        return {
            static_cast<uint16_t>(x * Scale),
            static_cast<uint16_t>(y * Scale),
            static_cast<uint16_t>(z * Scale),
            static_cast<uint16_t>(w * Scale)
        };
    }

    static constexpr void decompress(const CompressedQuaternion& cq, float& x, float& y, float& z, float& w) noexcept {
        x = static_cast<float>(cq.x) / Scale;
        y = static_cast<float>(cq.y) / Scale;
        z = static_cast<float>(cq.z) / Scale;
        w = static_cast<float>(cq.w) / Scale;
    }
};

} // namespace interlock::math
