/**
 * @file chunk_reader.hh
 * @brief Abstract interface for reading chunk data from IFF/RIFF files
 * @author Igor
 * @date 14/08/2025
 */

#pragma once

#include <iosfwd>
#include <optional>
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <iff/export_iff.h>
#include <iff/fourcc.hh>

namespace iff {
    
    /**
     * @class chunk_reader
     * @brief Abstract interface for reading chunk data
     * 
     * Format-specific implementations handle padding, offset tables,
     * and other format-specific details. This provides a uniform interface
     * for reading data from both IFF-85 and RIFF chunks.
     */
    class IFF_EXPORT chunk_reader {
    public:
        /**
         * @brief Virtual destructor
         */
        virtual ~chunk_reader() = default;
        
        /**
         * @brief Read data from the chunk
         * @param dst Destination buffer
         * @param size Number of bytes to read
         * @return Number of bytes actually read
         */
        virtual std::size_t read(void* dst, std::size_t size) = 0;
        
        /**
         * @brief Skip bytes in the chunk
         * @param size Number of bytes to skip
         * @return True if skip was successful
         */
        virtual bool skip(std::size_t size) = 0;
        
        /**
         * @brief Get number of bytes remaining in chunk
         * @return Bytes remaining
         */
        virtual std::uint64_t remaining() const = 0;
        
        /**
         * @brief Get current offset within chunk
         * @return Current offset from chunk start
         */
        virtual std::uint64_t offset() const = 0;
        
        /**
         * @brief Get total chunk size
         * @return Total size of chunk data
         */
        virtual std::uint64_t size() const = 0;
        
        /**
         * @brief Get underlying stream for external parsers
         * @return Reference to the underlying input stream
         */
        virtual std::istream& stream() = 0;
        
        /**
         * @brief Read a string of specified size
         * @param size Number of bytes to read
         * @return String if successful, nullopt on error
         */
        virtual std::optional<std::string> read_string(std::size_t size);
        
        /**
         * @brief Read a FourCC code
         * @return FourCC if successful, nullopt on error
         */
        virtual std::optional<fourcc> read_fourcc();
        
        /**
         * @brief Read all remaining data in chunk
         * @return Vector containing all remaining bytes
         */
        virtual std::vector<std::byte> read_all();
        
        /**
         * @brief Read specified number of bytes
         * @param n Number of bytes to read
         * @return Vector containing the read bytes
         */
        virtual std::vector<std::byte> read_bytes(std::size_t n);
    };
    
} // namespace iff