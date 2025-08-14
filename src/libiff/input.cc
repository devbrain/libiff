//
// Created by igor on 12/08/2025.
//

#include <istream>

#include "input.hh"

namespace iff {
    // reader_base implementation
    std::unique_ptr<subreader> reader_base::create_subreader(std::size_t size) {
        std::uint64_t pos = tell();
        return std::make_unique<subreader>(this, pos, size, false);
    }

    std::unique_ptr<subreader> reader_base::create_subreader(std::istream& is) {
        auto start = is.tellg();
        THROW_IO_IF(start == std::streampos(-1), "Failed to get stream position");
        
        is.seekg(0, std::ios::end);
        auto end = is.tellg();
        THROW_IO_IF(end == std::streampos(-1), "Failed to get stream end position");
        
        is.seekg(start, std::ios::beg);
        auto size = end - start;
        
        return std::make_unique<subreader>(new reader(is), start, size, true);
    }

    fourcc reader_base::read_fourcc() {
        std::array<char, 4> data;
        std::size_t actual = read(data.data(), 4);
        THROW_IO_IF(actual != 4, "Failed to read FourCC");
        return fourcc(data[0], data[1], data[2], data[3]);
    }

    // reader implementation
    reader::reader(std::istream& is) : m_stream(is) {}

    std::size_t reader::read(void* dst, std::size_t size) {
        THROW_IO_UNLESS(dst, "Null buffer in read");
        
        if (size == 0) {
            return 0;
        }
        
        THROW_IO_UNLESS(m_stream.good(), "Stream in bad state");

        m_stream.read(static_cast<char*>(dst), static_cast<std::streamsize>(size));
        std::size_t bytes_read = static_cast<std::size_t>(m_stream.gcount());
        
        THROW_IO_IF(m_stream.bad(), "Stream read failed");
        return bytes_read;
    }

    void reader::seek(std::uint64_t offset, whence_t whence) {
        m_stream.clear();

        std::ios_base::seekdir dir;
        switch (whence) {
            case set:
                dir = std::ios_base::beg;
                break;
            case cur:
                dir = std::ios_base::cur;
                break;
            case end:
                dir = std::ios_base::end;
                break;
            default:
                THROW_IO("Invalid whence value:", static_cast<int>(whence));
        }
        
        m_stream.seekg(static_cast<std::streamoff>(offset), dir);
        if (m_stream.fail()) {
            // Get current position and stream size for better error message
            auto current = m_stream.tellg();
            m_stream.seekg(0, std::ios::end);
            auto size = m_stream.tellg();
            
            std::string error = "Cannot seek to offset " + std::to_string(offset);
            if (whence == reader_base::set) {
                error += " (absolute)";
            } else if (whence == reader_base::cur) {
                error += " (relative)";
            }
            
            if (size != std::streampos(-1)) {
                error += " - stream size is only " + std::to_string(size) + " bytes";
            }
            
            if (offset > static_cast<std::uint64_t>(size)) {
                error += " (attempted to seek beyond end of stream)";
            }
            
            THROW_IO(error);
        }
    }

    std::uint64_t reader::tell() const {
        std::streampos pos = const_cast<std::istream&>(m_stream).tellg();
        THROW_IO_IF(pos == std::streampos(-1), "Tell failed");
        return static_cast<std::uint64_t>(pos);
    }

    std::uint64_t reader::size() const {
        // Save current position
        std::streampos current_pos = const_cast<std::istream&>(m_stream).tellg();
        THROW_IO_IF(current_pos == std::streampos(-1), "Tell failed in size()");
        
        // Seek to end to get size
        const_cast<std::istream&>(m_stream).seekg(0, std::ios_base::end);
        std::streampos end_pos = const_cast<std::istream&>(m_stream).tellg();
        
        // Restore original position
        const_cast<std::istream&>(m_stream).seekg(current_pos);
        
        THROW_IO_IF(end_pos == std::streampos(-1), "Failed to get stream size");
        return static_cast<std::uint64_t>(end_pos);
    }

    // subreader implementation
    subreader::subreader(reader_base* parent, std::uint64_t start, std::size_t size, bool release_parent)
        : m_parent(parent), m_start(start), m_size(size), m_position(0), m_release_parent(release_parent) {}

    subreader::~subreader() {
        if (m_release_parent) {
            delete m_parent;
        }
    }

    std::size_t subreader::read(void* dst, std::size_t size) {
        THROW_IO_UNLESS(dst, "Null buffer in subreader::read");
        
        if (size == 0) {
            return 0;
        }

        // Check how much we can read within our region
        std::size_t available = remaining();
        if (available == 0) {
            return 0;  // EOF-like behavior
        }

        size = std::min(size, available);

        // Seek parent to our current position
        m_parent->seek(m_start + m_position, reader_base::set);
        
        // Read from parent
        std::size_t actual = m_parent->read(dst, size);
        
        // Update our position
        m_position += actual;
        return actual;
    }

    void subreader::seek(std::uint64_t offset, whence_t whence) {
        std::uint64_t new_pos;

        switch (whence) {
            case set:
                new_pos = offset;
                break;
            case cur:
                new_pos = m_position + offset;
                break;
            case end:
                new_pos = m_size - offset;
                break;
            default:
                THROW_IO("Invalid whence value in subreader");
        }
        
        // Check bounds
        THROW_IO_IF(new_pos > m_size, "Seek beyond subreader bounds:", new_pos, ">", m_size);
        
        m_position = new_pos;
    }

    std::uint64_t subreader::tell() const {
        return m_position;
    }

    std::uint64_t subreader::size() const {
        return m_size;
    }

    std::size_t subreader::remaining() const {
        return m_size - m_position;
    }

    std::istream& subreader::get_stream() {
        // Pass through to parent
        return m_parent->get_stream();
    }
}
