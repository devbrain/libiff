//
// Created by igor on 14/08/2025.
//

#pragma once

#include <iosfwd>
#include <optional>
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <iff/export_iff.h>
#include <iff/fourcc.hh>

namespace iff {
    
    // Abstract interface for reading chunk data
    // Format-specific implementations handle padding, offset tables, etc.
    class IFF_EXPORT chunk_reader {
    public:
        virtual ~chunk_reader() = default;
        
        // Core operations
        virtual std::size_t read(void* dst, std::size_t size) = 0;
        virtual bool skip(std::size_t size) = 0;
        
        // Status queries
        virtual std::uint64_t remaining() const = 0;
        virtual std::uint64_t offset() const = 0;
        virtual std::uint64_t size() const = 0;
        
        // Get underlying stream for external parsers (Kaitai Struct, etc)
        virtual std::istream& stream() = 0;
        
        // Convenience methods with default implementations
        virtual std::optional<std::string> read_string(std::size_t size);
        virtual std::optional<fourcc> read_fourcc();
        virtual std::vector<std::byte> read_all();
        virtual std::vector<std::byte> read_bytes(std::size_t n);
    };
    
} // namespace iff