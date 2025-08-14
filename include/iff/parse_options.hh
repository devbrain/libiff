/**
 * @file parse_options.hh
 * @brief Parsing options and configuration for IFF/RIFF files
 * @author Igor
 * @date 14/08/2025
 */

#pragma once

#include <functional>
#include <string_view>
#include <cstdint>

namespace iff {
    
    /**
     * @struct parse_options
     * @brief Configuration options for parsing IFF/RIFF files
     * 
     * Controls various aspects of parsing behavior including
     * strictness, size limits, and warning handling.
     */
    struct parse_options {
        /**
         * @brief Strict parsing mode
         * 
         * When true, parsing will fail on any format violations.
         * When false, attempts to continue parsing despite errors.
         */
        bool strict = true;
        
        /**
         * @brief Maximum allowed chunk size in bytes
         * 
         * Chunks larger than this will trigger an error or warning.
         * Default is 4GB.
         */
        std::uint64_t max_chunk_size = std::uint64_t(1) << 32;  // 4GB
        
        /**
         * @brief Allow RF64 format for large RIFF files
         * 
         * When true, RF64 chunks are processed normally.
         * When false, RF64 is treated as an error.
         */
        bool allow_rf64 = true;
        
        /**
         * @brief Maximum nesting depth for containers
         * 
         * Prevents stack overflow from deeply nested or circular structures.
         * Default is 64 levels.
         */
        int max_depth = 64;
        
        /**
         * @typedef warning_handler
         * @brief Callback function type for handling warnings
         * @param offset File offset where warning occurred
         * @param category Warning category (e.g., "size_limit", "depth_limit")
         * @param message Human-readable warning message
         */
        using warning_handler = std::function<void(
            std::uint64_t offset,
            std::string_view category,
            std::string_view message
        )>;
        
        /**
         * @brief Optional warning handler callback
         * 
         * If set, will be called for non-fatal issues during parsing.
         * If not set, warnings are silently ignored.
         */
        warning_handler on_warning;
    };
    
} // namespace iff