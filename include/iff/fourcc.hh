//
// Created by igor on 10/08/2025.
//
#pragma once
#include <array>
#include <cstring>
#include <cstdint>
#include <string>
#include <string_view>
#include <algorithm>
#include <ostream>
#include <iomanip>

namespace iff {
    struct fourcc {
        std::array<char, 4> b{' ', ' ', ' ', ' '};

        // Default constructor - creates "    " (four spaces)
        constexpr fourcc() = default;

        // Constructor from 4 individual chars
        constexpr fourcc(char c0, char c1, char c2, char c3)
            : b{ c0, c1, c2, c3 } {}

        constexpr fourcc(std::byte c0, std::byte c1, std::byte c2, std::byte c3)
            : b{ static_cast<char>(c0), static_cast<char>(c1), static_cast<char>(c2), static_cast<char>(c3) } {}

        // Constructor from string_view with padding (runtime)
        explicit fourcc(std::string_view sv) : b{' ', ' ', ' ', ' '} {
            std::copy_n(sv.begin(), std::min(sv.size(), size_t(4)), b.begin());
        }

        // Constructor from C-string with padding (runtime)
        fourcc(const char* str) : fourcc(std::string_view(str)) {}

        // Constructor from std::string with padding (runtime)
        explicit fourcc(const std::string& str) : fourcc(std::string_view(str)) {}

        // Constructor from raw bytes (no padding)
        static fourcc from_bytes(const void* data) {
            fourcc result;
            std::memcpy(result.b.data(), data, 4);
            return result;
        }

        // Constructor from uint32_t (native byte order)
        explicit fourcc(std::uint32_t value) {
            std::memcpy(b.data(), &value, 4);
        }

        // Convert to string
        [[nodiscard]] std::string to_string() const {
            return {b.data(), 4};
        }

        // Convert to string_view
        [[nodiscard]] std::string_view to_string_view() const {
            return {b.data(), 4};
        }

        // Convert to uint32_t (native byte order)
        [[nodiscard]] std::uint32_t to_uint32() const {
            std::uint32_t result;
            std::memcpy(&result, b.data(), 4);
            return result;
        }

        // Write to bytes
        void to_bytes(void* dest) const {
            std::memcpy(dest, b.data(), 4);
        }

        // Access individual characters
        constexpr char operator[](std::size_t i) const { return b[i]; }
        constexpr char& operator[](std::size_t i) { return b[i]; }

        // Iterators
        [[nodiscard]] constexpr auto begin() const { return b.begin(); }
        [[nodiscard]] constexpr auto end() const { return b.end(); }
        constexpr auto begin() { return b.begin(); }
        constexpr auto end() { return b.end(); }

        // Comparison operators
        bool operator==(const fourcc& o) const { return b == o.b; }
        bool operator!=(const fourcc& o) const { return !(*this == o); }
        bool operator<(const fourcc& o) const { return b < o.b; }
        bool operator<=(const fourcc& o) const { return b <= o.b; }
        bool operator>(const fourcc& o) const { return b > o.b; }
        bool operator>=(const fourcc& o) const { return b >= o.b; }

        // Check if contains only printable ASCII
        [[nodiscard]] bool is_printable() const {
            return std::all_of(b.begin(), b.end(), [](char c) {
                return c >= 32 && c <= 126;
            });
        }

        // Check if contains spaces (padding)
        [[nodiscard]] bool has_padding() const {
            return std::any_of(b.begin(), b.end(), [](char c) {
                return c == ' ';
            });
        }

        // Trim trailing spaces
        [[nodiscard]] std::string to_string_trimmed() const {
            auto str = to_string();
            str.erase(str.find_last_not_of(' ') + 1);
            return str;
        }

        // Stream output
        friend std::ostream& operator<<(std::ostream& os, const fourcc& f) {
            // Check if hex format is set
            if (os.flags() & std::ios::hex) {
                // Save and restore format flags
                auto flags = os.flags();
                auto fill = os.fill();
                os << "0x" << std::hex << std::setfill('0') << std::setw(8)
                   << f.to_uint32();
                os.flags(flags);
                os.fill(fill);
            } else {
                // Default: output as quoted string
                os << '\'';
                for (char c : f.b) {
                    if (c >= 32 && c <= 126) {
                        os << c;
                    } else {
                        // Escape non-printable characters
                        os << "\\x" << std::hex << std::setfill('0') << std::setw(2)
                           << static_cast<unsigned>(static_cast<unsigned char>(c))
                           << std::dec;
                    }
                }
                os << '\'';
            }
            return os;
        }
    };

    // Hash function
    struct fourcc_hash {
        std::size_t operator()(const fourcc& f) const noexcept {
            std::uint32_t v;
            std::memcpy(&v, f.b.data(), 4);
            // Better hash mixing using FNV-1a constants
            return (static_cast<std::size_t>(v) * 0x9E3779B1u) ^ 0x85EBCA6Bu;
        }
    };

    // User-defined literal for compile-time fourcc creation
    constexpr fourcc operator""_4cc(const char* str, std::size_t len) {
        if (len > 4) {
            throw std::invalid_argument("FourCC literal must be 4 characters or less");
        }
        return {
            len > 0 ? str[0] : ' ',
            len > 1 ? str[1] : ' ',
            len > 2 ? str[2] : ' ',
            len > 3 ? str[3] : ' '
        };
    }

    // Macro for compile-time validated fourcc (uses string literals)
#define FOURCC(str) ::iff::fourcc(str)

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

