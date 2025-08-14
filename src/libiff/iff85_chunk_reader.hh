//
// Created by igor on 13/08/2025.
//

#pragma once

#include <iff/parser.hh>
#include <memory>
#include <iosfwd>

namespace iff {
    
    class subreader;
    
    // IFF-85 specific chunk reader implementation
    // Handles padding automatically when destroyed
    class iff85_chunk_reader : public chunk_reader {
    public:
        iff85_chunk_reader(std::unique_ptr<subreader> reader, std::uint64_t chunk_size);
        ~iff85_chunk_reader();
        
        // Core operations
        std::size_t read(void* dst, std::size_t size) override;
        bool skip(std::size_t size) override;
        
        // Status queries
        std::uint64_t remaining() const override;
        std::uint64_t offset() const override;
        std::uint64_t size() const override;
        
        // Get underlying stream
        std::istream& stream() override;
        
        // IFF-85 specific
        std::uint64_t position() const;
        
    private:
        std::unique_ptr<subreader> m_reader;
        std::uint64_t m_chunk_size;  // Actual chunk size (without padding)
        std::uint64_t m_start_offset;
        std::uint64_t m_bytes_read;  // Track how many bytes have been read
    };
    
} // namespace iff