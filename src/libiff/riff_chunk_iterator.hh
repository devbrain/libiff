//
// Created by igor on 14/08/2025.
//

#pragma once

#include <iff/chunk_iterator.hh>
#include "input.hh"
#include <memory>
#include <unordered_map>

namespace iff {
    
    // RF64 size override information from ds64 chunk
    struct rf64_state {
        std::uint64_t riff_size = 0;      // 64-bit size of RIFF chunk
        std::uint64_t data_size = 0;      // 64-bit size of data chunk
        std::uint64_t sample_count = 0;   // Sample count for audio
        
        // Table of chunk size overrides from ds64
        // Maps chunk ID to list of sizes (for multiple chunks with same ID)
        std::unordered_map<fourcc, std::vector<std::uint64_t>> override_table;
        std::unordered_map<fourcc, size_t> override_index;  // Track which override to use next
    };
    
    // RIFF specific iterator (internal implementation)
    class riff_chunk_iterator : public chunk_iterator {
    public:
        // Construct from stream
        explicit riff_chunk_iterator(std::istream& stream);
        riff_chunk_iterator(std::istream& stream, const parse_options& options);
        
        // End iterator
        riff_chunk_iterator() : chunk_iterator(), m_stream(nullptr) {}
        
        ~riff_chunk_iterator() override;
        
    protected:
        void advance() override;
        
    private:
        // Read next chunk at current position
        bool read_next_chunk();
        
        // Handle container chunks (RIFF/LIST)
        bool process_container(const chunk_header& header);
        
        // Check if current position is inside a container
        void update_container_context();
        
        // Parse ds64 chunk for RF64 support
        void parse_ds64_chunk(std::uint64_t chunk_size);
        
        // Get size override for chunks with 0xFFFFFFFF size
        std::uint64_t get_size_override(fourcc id, std::uint64_t offset, std::uint32_t size_32);
        
        std::unique_ptr<reader_base> m_reader;
        std::istream* m_stream;  // Pointer to the underlying stream
        
        // Format detection
        byte_order m_byte_order = byte_order::little;  // RIFF is little, RIFX is big
        bool m_is_rf64 = false;
        rf64_state m_rf64_state;
    };
    
} // namespace iff