//
// Created by igor on 14/08/2025.
//

#pragma once

#include <iff/chunk_reader.hh>
#include "input.hh"
#include <memory>

namespace iff {
    
    class riff_chunk_reader : public chunk_reader {
    public:
        riff_chunk_reader(reader_base* reader, std::istream& stream, std::uint64_t start_offset, std::uint64_t size);
        ~riff_chunk_reader() override;
        
        // Core operations
        std::size_t read(void* dst, std::size_t size) override;
        bool skip(std::size_t size) override;
        
        // Status queries
        std::uint64_t remaining() const override;
        std::uint64_t offset() const override;
        std::uint64_t size() const override { return m_size; }
        
        // Get underlying stream
        std::istream& stream() override { return m_stream; }
        
    private:
        reader_base* m_reader;  // Non-owning pointer to the stream reader
        std::istream& m_stream;       // Reference to the underlying stream
        std::uint64_t m_start_offset;  // Offset where chunk data starts in file
        std::uint64_t m_size;          // Chunk data size
        std::uint64_t m_bytes_read;    // Bytes read so far
    };
    
} // namespace iff