/**
 * @file chunk_header.hh
 * @brief Chunk header structure for IFF/RIFF files
 * @author Igor
 * @date 14/08/2025
 */

#pragma once

#include <optional>
#include <cstdint>
#include <iff/fourcc.hh>

namespace iff {
    
    /**
     * @struct chunk_header
     * @brief Header information for a chunk in IFF/RIFF files
     * 
     * Contains all metadata about a chunk including its ID, size,
     * position in the file, and container information.
     */
    struct chunk_header {
        fourcc id;                         ///< Chunk identifier (4 characters)
        std::uint64_t size = 0;           ///< Payload size in bytes (excluding padding)
        std::uint64_t file_offset = 0;    ///< Absolute offset to chunk header in file
        bool is_container = false;        ///< True if this is a container chunk (FORM/LIST/etc)
        std::optional<fourcc> type;       ///< Container type (e.g., "WAVE" for RIFF WAVE)
        
        /**
         * @brief Default constructor
         * 
         * Initializes with default values: empty fourcc, zero size/offset,
         * not a container, no type.
         */
        chunk_header() = default;
    };
    

    
} // namespace iff