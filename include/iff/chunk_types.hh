//
// Created by igor on 14/08/2025.
//

#pragma once

#include <optional>
#include <cstdint>
#include <iff/fourcc.hh>

namespace iff {
    
    // Source of chunk data
    enum class chunk_source {
        explicit_data,    // Normal chunk in file
        prop_default      // Synthesized from PROP defaults
    };
    
    // Chunk header information
    struct chunk_header {
        fourcc id;
        std::uint64_t size;           // Payload size (excluding padding)
        std::uint64_t file_offset;    // Offset to chunk header in file
        bool is_container;
        std::optional<fourcc> type;   // For container chunks
        chunk_source source = chunk_source::explicit_data;
    };
    

    
} // namespace iff