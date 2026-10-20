#pragma once

#include <string_view>
#include <array>
#include <cstdint>

namespace interlock::integrity {

/**
 * @brief Ofuscación de strings en tiempo de compilación (M4 Pro Optimized)
 */
template<size_t N>
class XorString {
public:
    consteval XorString(const char* str) : _key(0xAD) {
        for (size_t i = 0; i < N; ++i) {
            _data[i] = str[i] ^ _key;
        }
    }

    std::string decrypt() const {
        std::string result;
        result.reserve(N);
        for (size_t i = 0; i < N; ++i) {
            result += static_cast<char>(_data[i] ^ _key);
        }
        return result;
    }

private:
    std::array<uint8_t, N> _data{};
    uint8_t _key;
};

#define O_STR(str) []{ constexpr auto x = interlock::integrity::XorString<sizeof(str)>(str); return x.decrypt(); }()

/**
 * @brief Control Flow Flattening macro para lógica crítica de agentes.
 * Simula el comportamiento de OLLVM -fla.
 */
#define STEALTH_BEGIN(state_var) \
    int state_var = 0; \
    while (state_var != -1) { \
        switch (state_var) {

#define STEALTH_CASE(id) case id:
#define STEALTH_NEXT(id) state_var = id; break;
#define STEALTH_END state_var = -1; break; default: state_var = -1; break; } }

} // namespace interlock::integrity
