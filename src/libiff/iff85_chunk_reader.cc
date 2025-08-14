//
// Created by igor on 13/08/2025.
//

#include "iff85_chunk_reader.hh"
#include "input.hh"
#include <iostream>
#include <array>
#include <algorithm>

namespace iff {
    
    iff85_chunk_reader::iff85_chunk_reader(std::unique_ptr<subreader> reader, std::uint64_t chunk_size)
        : m_reader(std::move(reader))
        , m_chunk_size(chunk_size)
        , m_start_offset(0)
        , m_bytes_read(0) {
        if (m_reader) {
            m_start_offset = m_reader->start_offset();
        }
    }
    
    iff85_chunk_reader::~iff85_chunk_reader() {
        // Nothing to do - the container has already created a subreader with padding included
        // and will advance past it all
    }
    
    std::size_t iff85_chunk_reader::read(void* dst, std::size_t size) {
        if (!m_reader || !dst || size == 0) {
            return 0;
        }
        
        // Limit reads to the actual chunk size (excluding padding)
        std::uint64_t available = m_chunk_size - m_bytes_read;
        if (available == 0) {
            return 0;  // Already read all chunk data
        }
        
        size = std::min(size, static_cast<std::size_t>(available));
        
        try {
            std::size_t actual = m_reader->read(dst, size);
            m_bytes_read += actual;
            return actual;
        } catch (...) {
            // Swallow errors, return 0 for handler simplicity
            return 0;
        }
    }
    
    bool iff85_chunk_reader::skip(std::size_t size) {
        if (!m_reader) {
            return false;
        }
        
        // Limit skip to actual chunk size
        std::uint64_t available = m_chunk_size - m_bytes_read;
        if (size > available) {
            return false;
        }
        
        try {
            std::uint64_t pos = m_reader->tell();
            m_reader->seek(pos + size, reader_base::set);
            m_bytes_read += size;
            return true;
        } catch (...) {
            return false;
        }
    }
    
    std::uint64_t iff85_chunk_reader::remaining() const {
        if (!m_reader) {
            return 0;
        }
        // Return remaining bytes in the actual chunk (excluding padding)
        return m_chunk_size - m_bytes_read;
    }
    
    std::uint64_t iff85_chunk_reader::offset() const {
        // Return current position within the chunk (0 = beginning of chunk)
        return m_bytes_read;
    }
    
    std::uint64_t iff85_chunk_reader::size() const {
        return m_chunk_size;
    }
    
    std::uint64_t iff85_chunk_reader::position() const {
        // Return how many bytes have been read from the chunk
        return m_bytes_read;
    }
    
    std::istream& iff85_chunk_reader::stream() {
        if (!m_reader) {
            THROW_IO("No underlying stream available");
        }
        
        // Get the stream from the reader chain
        return m_reader->get_stream();
    }
    
    
} // namespace iff