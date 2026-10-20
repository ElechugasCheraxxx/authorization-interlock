#pragma once
#include <cstdint>
#include <concepts>

namespace interlock::math {

/**
 * @brief Deterministic Fixed-Point Math (FP16).
 * Eliminates floating point variance across distributed agents.
 */
template<typename T, int FractionBits>
requires std::is_integral_v<T>
struct FixedPoint {
    T value;

    static constexpr float Scale = static_cast<float>(1 << FractionBits);

    constexpr FixedPoint() : value(0) {}
    explicit constexpr FixedPoint(float f) : value(static_cast<T>(f * Scale)) {}

    constexpr float toFloat() const noexcept {
        return static_cast<float>(value) / Scale;
    }

    constexpr FixedPoint operator+(const FixedPoint& other) const noexcept {
        FixedPoint res;
        res.value = value + other.value;
        return res;
    }

    constexpr FixedPoint operator-(const FixedPoint& other) const noexcept {
        FixedPoint res;
        res.value = value - other.value;
        return res;
    }

    // Basic multiplication (non-overflow protected for performance)
    constexpr FixedPoint operator*(const FixedPoint& other) const noexcept {
        FixedPoint res;
        res.value = static_cast<T>((static_cast<int64_t>(value) * other.value) >> FractionBits);
        return res;
    }
};

using Fixed16 = FixedPoint<int16_t, 8>; // 8.8 fixed point
using Fixed32 = FixedPoint<int32_t, 16>; // 16.16 fixed point

} // namespace interlock::math
