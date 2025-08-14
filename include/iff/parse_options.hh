//
// Created by igor on 14/08/2025.
//

#pragma once

#include <functional>
#include <string_view>
#include <cstdint>

namespace iff {
    
    struct parse_options {
        bool strict = true;
        std::uint64_t max_chunk_size = std::uint64_t(1) << 32;  // 4GB
        bool allow_rf64 = true;
        int max_depth = 64;
        
        // Warning callback
        using warning_handler = std::function<void(
            std::uint64_t offset,
            std::string_view category,
            std::string_view message
        )>;
        warning_handler on_warning;
    };
    
} // namespace iff