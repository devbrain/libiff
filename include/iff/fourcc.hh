/**
 * @file fourcc.hh
 * @brief Four Character Code (FourCC) implementation for IFF/RIFF formats
 * @author Igor
 * @date 10/08/2025
 */
#pragma once
#include <array>
#include <cstring>
#include <cstdint>
#include <string>
#include <string_view>
#include <algorithm>
#include <ostream>

namespace iff {
    /**
     * @struct fourcc
     * @brief Four Character Code identifier used in IFF/RIFF formats
     * 
     * FourCC codes are 4-byte identifiers used to identify chunk types
     * and format types in IFF and RIFF files.
     */
    struct fourcc {
        std::array<char, 4> b{' ', ' ', ' ', ' '};  ///< The four character bytes

        /**
         * @brief Default constructor - creates "    " (four spaces)
         */
        constexpr fourcc() = default;

        /**
         * @brief Constructor from 4 individual characters
         * @param c0 First character
         * @param c1 Second character
         * @param c2 Third character
         * @param c3 Fourth character
         */
        constexpr fourcc(char c0, char c1, char c2, char c3)
            : b{ c0, c1, c2, c3 } {}


        /**
         * @brief Constructor from string_view with padding
         * @param sv String view (padded with spaces if less than 4 chars)
         */
        explicit fourcc(std::string_view sv) : b{' ', ' ', ' ', ' '} {
            std::copy_n(sv.begin(), std::min(sv.size(), size_t(4)), b.begin());
        }

        /**
         * @brief Constructor from C-string with padding
         * @param str C-string (padded with spaces if less than 4 chars)
         */
        fourcc(const char* str) : fourcc(std::string_view(str)) {}

        /**
         * @brief Constructor from std::string with padding
         * @param str String (padded with spaces if less than 4 chars)
         */
        explicit fourcc(const std::string& str) : fourcc(std::string_view(str)) {}

        /**
         * @brief Create fourcc from raw bytes (no padding)
         * @param data Pointer to 4 bytes of data
         * @return FourCC created from the bytes
         */
        static fourcc from_bytes(const void* data) {
            fourcc result;
            std::memcpy(result.b.data(), data, 4);
            return result;
        }


        // Convert to string
        [[nodiscard]] std::string to_string() const {
            return {b.data(), 4};
        }

        // Convert to uint32_t (for hashing)
        [[nodiscard]] std::uint32_t to_uint32() const {
            std::uint32_t result;
            std::memcpy(&result, b.data(), 4);
            return result;
        }

        // Access individual characters (useful for tests)
        constexpr char operator[](std::size_t i) const { return b[i]; }
        constexpr char& operator[](std::size_t i) { return b[i]; }



        // Comparison operators
        bool operator==(const fourcc& o) const { return b == o.b; }
        bool operator!=(const fourcc& o) const { return !(*this == o); }
        bool operator<(const fourcc& o) const { return b < o.b; }
        bool operator<=(const fourcc& o) const { return b <= o.b; }
        bool operator>(const fourcc& o) const { return b > o.b; }
        bool operator>=(const fourcc& o) const { return b >= o.b; }


        // Stream output
        friend std::ostream& operator<<(std::ostream& os, const fourcc& f) {
            os << '\'';
            for (char c : f.b) {
                if (c >= 32 && c <= 126) {
                    os << c;
                } else {
                    // Replace non-printable with dot
                    os << '.';
                }
            }
            os << '\'';
            return os;
        }
    };

    // Hash function
    struct fourcc_hash {
        std::size_t operator()(const fourcc& f) const noexcept {
            std::uint32_t v;
            std::memcpy(&v, f.b.data(), 4);
            return std::hash<std::uint32_t>{}(v);
        }
    };

    // User-defined literal for compile-time fourcc creation
    constexpr fourcc operator""_4cc(const char* str, std::size_t len) {
        return {
            len > 0 ? str[0] : ' ',
            len > 1 ? str[1] : ' ',
            len > 2 ? str[2] : ' ',
            len > 3 ? str[3] : ' '
        };
    }


}
// Specialization for std::hash
namespace std {
    template<>
    struct hash<iff::fourcc> {
        std::size_t operator()(const iff::fourcc& f) const noexcept {
            return iff::fourcc_hash{}(f);
        }
    };
}

