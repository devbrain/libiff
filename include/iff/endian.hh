//
// Created by igor on 10/08/2025.
//

#pragma once

#include <cstdint>
#include <type_traits>

#include <iff/iff_config.h>

namespace iff {
    // Platform endianness detection using CMake-generated config
#if LIBIFF_BIG_ENDIAN
    constexpr bool is_big_endian = true;
    constexpr bool is_little_endian = false;
#else
    constexpr bool is_big_endian = false;
    constexpr bool is_little_endian = true;
#endif

    // Byte swapping functions
    inline uint16_t swap16(uint16_t x) {
        return (x << 8) | (x >> 8);
    }

    inline uint32_t swap32(uint32_t x) {
        return ((x << 24) | ((x << 8) & 0x00FF0000) |
                ((x >> 8) & 0x0000FF00) | (x >> 24));
    }

    inline uint64_t swap64(uint64_t x) {
        return ((x << 56) |
                ((x << 40) & 0x00FF000000000000ULL) |
                ((x << 24) & 0x0000FF0000000000ULL) |
                ((x << 8) & 0x000000FF00000000ULL) |
                ((x >> 8) & 0x00000000FF000000ULL) |
                ((x >> 24) & 0x0000000000FF0000ULL) |
                ((x >> 40) & 0x000000000000FF00ULL) |
                (x >> 56));
    }

    inline float swap_float(float x) {
        union {
            float f;
            uint32_t u;
        } data;
        data.f = x;
        data.u = swap32(data.u);
        return data.f;
    }

    // Conditional byte swapping based on platform
    inline uint16_t swap16le(uint16_t x) {
        return is_little_endian ? x : swap16(x);
    }

    inline uint16_t swap16be(uint16_t x) {
        return is_big_endian ? x : swap16(x);
    }

    inline uint32_t swap32le(uint32_t x) {
        return is_little_endian ? x : swap32(x);
    }

    inline uint32_t swap32be(uint32_t x) {
        return is_big_endian ? x : swap32(x);
    }

    inline uint64_t swap64le(uint64_t x) {
        return is_little_endian ? x : swap64(x);
    }

    inline uint64_t swap64be(uint64_t x) {
        return is_big_endian ? x : swap64(x);
    }

    inline float swap_float_le(float x) {
        return is_little_endian ? x : swap_float(x);
    }

    inline float swap_float_be(float x) {
        return is_big_endian ? x : swap_float(x);
    }

    template<typename T>
    struct is_byte_swappable {
        static constexpr bool value =
            (std::is_integral_v <T> && (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8)) ||
            (std::is_floating_point_v <T> && (sizeof(T) == 4 || sizeof(T) == 8)) ||
            std::is_enum_v <T>;
    };

    template<typename T>
    inline constexpr bool is_byte_swappable_v = is_byte_swappable <T>::value;

    // Generic swap_byte_order implementation
    template<typename T>
    constexpr T swap_byte_order(T x) noexcept {
        static_assert(is_byte_swappable_v <T>,
                      "swap_byte_order only supports integral types (1,2,4,8 bytes), "
                      "floating point types (float, double), and enum types");

        if constexpr (sizeof(T) == 1) {
            // Single byte types don't need swapping
            return x;
        } else if constexpr (std::is_integral_v <T>) {
            // Integer types
            if constexpr (sizeof(T) == 2) {
                if constexpr (std::is_signed_v <T>) {
                    return static_cast <T>(swap16(static_cast <uint16_t>(x)));
                } else {
                    return swap16(x);
                }
            } else if constexpr (sizeof(T) == 4) {
                if constexpr (std::is_signed_v <T>) {
                    return static_cast <T>(swap32(static_cast <uint32_t>(x)));
                } else {
                    return swap32(x);
                }
            } else if constexpr (sizeof(T) == 8) {
                if constexpr (std::is_signed_v <T>) {
                    return static_cast <T>(swap64(static_cast <uint64_t>(x)));
                } else {
                    return swap64(x);
                }
            }
        } else if constexpr (std::is_floating_point_v <T>) {
            // Floating point types
            if constexpr (sizeof(T) == 4) {
                return swap_float(x);
            } else if constexpr (sizeof(T) == 8) {
                return swap_double(x);
            }
        } else if constexpr (std::is_enum_v <T>) {
            // Enum types - swap underlying type
            using underlying = std::underlying_type_t <T>;
            return static_cast <T>(swap_byte_order(static_cast <underlying>(x)));
        }
        // should not be here. make compiler happy
        return x;
    }
}
