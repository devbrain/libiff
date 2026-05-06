//
// Created by igor on 14/08/2025.
//

#include <iff/chunk_reader.hh>
#include <array>
#include <algorithm>

namespace iff {
    
    // Default implementations for chunk_reader convenience methods
    std::optional<std::string> chunk_reader::read_string(std::size_t size) {
        if (size == 0) {
            return std::nullopt;
        }
        
        std::string result(size, '\0');
        std::size_t actual = read(result.data(), size);
        
        if (actual != size) {
            return std::nullopt;
        }
        
        // Trim null terminators if present
        auto null_pos = result.find('\0');
        if (null_pos != std::string::npos) {
            result.resize(null_pos);
        }
        
        return result;
    }
    
    std::optional<fourcc> chunk_reader::read_fourcc() {
        std::array<char, 4> data{};
        if (read(data.data(), 4) != 4) {
            return std::nullopt;
        }
        return fourcc(data[0], data[1], data[2], data[3]);
    }
    
    std::vector<std::byte> chunk_reader::read_all() {
        std::size_t to_read = remaining();
        std::vector<std::byte> result(to_read);
        
        if (to_read > 0) {
            std::size_t actual = read(result.data(), to_read);
            result.resize(actual);
        }
        
        return result;
    }
    
    std::vector<std::byte> chunk_reader::read_bytes(std::size_t n) {
        std::size_t to_read = std::min(n, remaining());
        std::vector<std::byte> result(to_read);
        
        if (to_read > 0) {
            std::size_t actual = read(result.data(), to_read);
            result.resize(actual);
        }
        
        return result;
    }
    
} // namespace iff