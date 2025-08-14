//
// Created by igor on 14/08/2025.
//

#include "riff_chunk_reader.hh"
#include <iff/exceptions.hh>
#include <algorithm>

namespace iff {
    
    riff_chunk_reader::riff_chunk_reader(reader_base* reader, std::istream& stream, std::uint64_t start_offset, std::uint64_t size)
        : m_reader(reader)
        , m_stream(stream)
        , m_start_offset(start_offset)
        , m_size(size)
        , m_bytes_read(0) {
        if (!m_reader) {
            THROW_PARSE("Invalid reader provided to riff_chunk_reader");
        }
    }
    
    riff_chunk_reader::~riff_chunk_reader() = default;
    
    std::size_t riff_chunk_reader::read(void* dst, std::size_t size) {
        if (!dst) {
            return 0;
        }
        
        // Limit read to remaining bytes in chunk
        std::uint64_t available = remaining();
        std::size_t to_read = static_cast<std::size_t>(std::min(static_cast<std::uint64_t>(size), available));
        
        if (to_read == 0) {
            return 0;
        }
        
        // Seek to current position in chunk
        m_reader->seek(m_start_offset + m_bytes_read, reader_base::set);
        
        // Read data
        std::size_t bytes_read = m_reader->read(dst, to_read);
        m_bytes_read += bytes_read;
        
        return bytes_read;
    }
    
    bool riff_chunk_reader::skip(std::size_t size) {
        // Check if we can skip that many bytes
        if (size > remaining()) {
            return false;
        }
        
        m_bytes_read += size;
        return true;
    }
    
    std::uint64_t riff_chunk_reader::offset() const {
        // Return current position within the chunk (0 = beginning of chunk)
        return m_bytes_read;
    }
    
    std::uint64_t riff_chunk_reader::remaining() const {
        return m_size - m_bytes_read;
    }
    
    
} // namespace iff