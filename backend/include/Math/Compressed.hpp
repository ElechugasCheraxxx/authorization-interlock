#pragma once

#include <cstdint>
#include <cmath>

namespace interlock::math {

/**
 * @brief Compressed 3D Vector using 16-bit fixed-point representation.
 * Ported from InterestingSourceCode (Engine).
 * Reduces memory footprint from 12 bytes to 6 bytes.
 */
struct CompressedVector {
    int16_t x;
    int16_t y;
    int16_t z;

    template<int SCALE = 128>
    void decompress(float& outX, float& outY, float& outZ) const {
        outX = static_cast<float>(x) / static_cast<float>(SCALE);
        outY = static_cast<float>(y) / static_cast<float>(SCALE);
        outZ = static_cast<float>(z) / static_cast<float>(SCALE);
    }

    template<int SCALE = 128>
    void compress(float inX, float inY, float inZ) {
        x = static_cast<int16_t>(inX * SCALE);
        y = static_cast<int16_t>(inY * SCALE);
        z = static_cast<int16_t>(inZ * SCALE);
    }
};

/**
 * @brief Compressed Quaternion using 16-bit signed shorts.
 * Ported from InterestingSourceCode (Engine).
 * Reduces memory footprint from 16 bytes to 8 bytes.
 */
struct CompressedQuaternion {
    int16_t x, y, z, w;

    void decompress(float& outX, float& outY, float& outZ, float& outW) const {
        outX = static_cast<float>(x) / 32767.0f;
        outY = static_cast<float>(y) / 32767.0f;
        outZ = static_cast<float>(z) / 32767.0f;
        outW = static_cast<float>(w) / 32767.0f;
    }

    void compress(float inX, float inY, float inZ, float inW) {
        x = static_cast<int16_t>(inX * 32767.0f);
        y = static_cast<int16_t>(inY * 32767.0f);
        z = static_cast<int16_t>(inZ * 32767.0f);
        w = static_cast<int16_t>(inW * 32767.0f);
    }
};

} // namespace interlock::math
