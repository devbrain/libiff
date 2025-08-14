//
// Created by igor on 14/08/2025.
//

#include "riff_chunk_iterator.hh"
#include <iff/exceptions.hh>
#include "input.hh"
#include "riff_chunk_reader.hh"

namespace iff {
    
    static constexpr auto RIFF = "RIFF"_4cc;
    static constexpr auto RIFX = "RIFX"_4cc;  // Big-endian RIFF (rare)
    static constexpr auto RF64 = "RF64"_4cc;  // 64-bit RIFF
    static constexpr auto LIST = "LIST"_4cc;
    
    riff_chunk_iterator::riff_chunk_iterator(std::istream& stream)
        : riff_chunk_iterator(stream, parse_options{}) {
    }
    
    riff_chunk_iterator::riff_chunk_iterator(std::istream& stream, const parse_options& options)
        : chunk_iterator(options)
        , m_reader(std::make_unique<reader>(stream))
        , m_stream(&stream) {
        m_ended = false;
        
        // Check format and byte order
        char magic[4];
        stream.read(magic, 4);
        stream.seekg(0);  // Reset to start
        
        fourcc root_id(magic[0], magic[1], magic[2], magic[3]);
        
        // Determine format and byte order
        if (root_id == RIFF) {
            m_byte_order = byte_order::little;
            m_is_rf64 = false;
        } else if (root_id == RIFX) {
            m_byte_order = byte_order::big;  // RIFX is big-endian
            m_is_rf64 = false;
        } else if (root_id == RF64) {
            m_byte_order = byte_order::little;
            m_is_rf64 = true;
        } else {
            // Should not happen - factory should have filtered this
            THROW_PARSE("Invalid RIFF format: " + root_id.to_string());
        }
        
        // Read the first chunk
        if (!read_next_chunk()) {
            m_ended = true;
        }
    }
    
    riff_chunk_iterator::~riff_chunk_iterator() = default;
    
    void riff_chunk_iterator::advance() {
        if (m_ended) {
            return;
        }
        
        // If current chunk has a reader, we need to seek past it
        if (m_current.reader) {
            m_current.reader.reset();
            
            // Seek to the position after the chunk data + padding
            // The chunk started at header.file_offset
            // Header is 8 bytes (id + size)
            // Data + padding is total_size_with_padding
            std::uint64_t next_pos = m_current.header.file_offset + 8 + m_current.total_size_with_padding;
            
            // Try to seek, but if we can't (e.g., past end of stream), just mark as ended
            try {
                m_reader->seek(next_pos, reader_base::set);
            } catch (const io_error&) {
                // Can't seek past this chunk - we've reached the end
                m_ended = true;
                return;
            }
        }
        
        // Check if we've exited any containers
        update_container_context();
        
        // Read next chunk
        if (!read_next_chunk()) {
            m_ended = true;
        }
    }
    
    bool riff_chunk_iterator::read_next_chunk() {
        // Check if we're at the end of current container
        while (!m_container_stack.empty()) {
            auto& top = m_container_stack.top();
            // std::cerr << "Position " << m_reader->tell() << " vs container end " << top.end_offset << "\n";
            if (m_reader->tell() >= top.end_offset) {
                // We've reached the end of this container
                m_container_stack.pop();
                update_container_context();
            } else {
                break;  // Still inside container
            }
        }
        
        // Check if we have enough space for a chunk header (8 bytes)
        try {
            // Save position in case we need to rollback
            std::uint64_t start_pos = m_reader->tell();
            
            // Try to read chunk header
            fourcc chunk_id = m_reader->read_fourcc();
            // Read size using the appropriate byte order
            auto chunk_size_32 = m_reader->read<std::uint32_t>(m_byte_order);
            
            // Check for ds64 chunk in RF64 format  
            // ds64 must be the first chunk after RF64 container header
            if (m_is_rf64 && chunk_id == "ds64"_4cc) {
                // This is the ds64 chunk in an RF64 file, parse it for size overrides
                std::uint64_t ds64_start = m_reader->tell();
                // std::cerr << "Found ds64 at " << (ds64_start - 8) << ", hiding it\n";
                parse_ds64_chunk(chunk_size_32);
                
                // Update the RF64 container's end offset now that we have the real size
                if (!m_container_stack.empty() && m_container_stack.top().id == RF64) {
                    auto& rf64_container = m_container_stack.top();
                    // RF64 container starts at offset 0, has 8-byte header, 4-byte type, then data
                    // The riff_size from ds64 includes the type field
                    // Use the minimum of ds64 size and actual file size to handle truncated files
                    std::uint64_t ds64_end = 8 + m_rf64_state.riff_size;
                    std::uint64_t file_size = m_reader->size();
                    rf64_container.end_offset = std::min(ds64_end, file_size);
                }
                
                // ds64 is not exposed to the user, continue to next chunk
                // parse_ds64_chunk reads the data, so we need to skip any remaining bytes and padding
                std::uint64_t bytes_read = m_reader->tell() - ds64_start;
                std::uint64_t remaining = chunk_size_32 - bytes_read;
                if (remaining > 0) {
                    m_reader->seek(m_reader->tell() + remaining, reader_base::set);
                }
                // Handle padding for odd-sized chunks
                if (chunk_size_32 & 1) {
                    m_reader->seek(m_reader->tell() + 1, reader_base::set);
                }
                return read_next_chunk();
            }
            
            // Apply RF64 size overrides if needed
            std::uint64_t chunk_size = chunk_size_32;
            
            // For RF64 containers with 0xFFFFFFFF size, we'll fix the size after parsing ds64
            // For now, use the file size as a temporary upper bound
            if (m_is_rf64 && chunk_id == RF64 && chunk_size_32 == 0xFFFFFFFF) {
                // Will be fixed after ds64 is parsed
                chunk_size = m_reader->size() - start_pos - 8;  // Remaining file size
            } else {
                chunk_size = get_size_override(chunk_id, start_pos, chunk_size_32);
            }
            
            // Check size limit
            if (chunk_size > m_options.max_chunk_size) {
                if (m_options.strict) {
                    THROW_PARSE("Chunk '" + chunk_id.to_string() + "' at offset " + 
                               std::to_string(start_pos) + " has size " + std::to_string(chunk_size) + 
                               " bytes, which exceeds maximum allowed size of " + 
                               std::to_string(m_options.max_chunk_size) + " bytes");
                } else {
                    if (m_options.on_warning) {
                        m_options.on_warning(start_pos, "size_limit", 
                            "Chunk '" + chunk_id.to_string() + "' size " + std::to_string(chunk_size) +
                            " exceeds maximum " + std::to_string(m_options.max_chunk_size) + ", clamping to limit");
                    }
                    chunk_size = m_options.max_chunk_size;
                }
            }
            
            // Create chunk header
            m_current.header = {
                .id = chunk_id,
                .size = chunk_size,
                .file_offset = start_pos,
                .is_container = (chunk_id == RIFF || chunk_id == RIFX || chunk_id == RF64 || chunk_id == LIST),
                .type = std::nullopt,
                .source = chunk_source::explicit_data
            };
            
            // Update depth and context
            m_current.depth = m_container_stack.empty() ? 0 : m_container_stack.top().depth + 1;
            update_container_context();
            
            // Handle container chunks specially
            if (m_current.header.is_container) {
                return process_container(m_current.header);
            }
            
            // For regular chunks, calculate padding
            m_current.total_size_with_padding = chunk_size;
            if (chunk_size & 1) {
                // Odd size needs padding byte (word alignment)
                m_current.total_size_with_padding++;
            }
            
            // Create chunk reader for data chunks
            std::uint64_t data_start = m_reader->tell();
            m_current.reader = std::make_unique<riff_chunk_reader>(m_reader.get(), *m_stream, data_start, chunk_size);
            
            return true;
            
        } catch (const parse_error& e) {
            // Re-throw parse errors - they indicate real problems
            throw;
        } catch (const std::exception& e) {
            // Check if we've reached end of container or file
            if (m_container_stack.empty()) {
                // End of file is not an error
                return false;
            }
            // Check if we're at container boundary
            try {
                auto current_pos = m_reader->tell();
                if (!m_container_stack.empty()) {
                    auto& top = m_container_stack.top();
                    if (current_pos >= top.end_offset) {
                        // We've reached the end of this container
                        m_container_stack.pop();
                        update_container_context();
                        // Try reading next chunk after the container
                        return read_next_chunk();
                    }
                }
                // Otherwise it's a real error
                THROW_PARSE(std::string("Failed to read chunk header: ") + e.what());
            } catch (...) {
                // If we can't get position, we're likely at EOF or stream is bad
                // Pop containers and try to continue
                if (!m_container_stack.empty()) {
                    m_container_stack.pop();
                    update_container_context();
                    return read_next_chunk();
                }
                // No more containers, we're done
                return false;
            }
        }
    }
    
    bool riff_chunk_iterator::process_container(const chunk_header& header) {
        // Check depth limit
        if (m_current.depth >= m_options.max_depth) {
            if (m_options.strict) {
                THROW_PARSE("Container '" + header.id.to_string() + "' at offset " + 
                           std::to_string(header.file_offset) + " would exceed maximum nesting depth of " + 
                           std::to_string(m_options.max_depth) + " (current depth: " + 
                           std::to_string(m_current.depth) + ")");
            } else if (m_options.on_warning) {
                m_options.on_warning(header.file_offset, "depth_limit", 
                    "Container '" + header.id.to_string() + "' would exceed maximum nesting depth " +
                    std::to_string(m_options.max_depth) + ", skipping");
            }
            // Skip this container
            m_reader->seek(header.file_offset + 8 + header.size, reader_base::set);
            return read_next_chunk();
        }
        
        // Container chunks in RIFF have a type field after the size
        try {
            fourcc container_type = m_reader->read_fourcc();
            
            // Update header with type
            m_current.header.type = container_type;
            
            // Calculate actual data size (minus the 4-byte type field)
            std::uint64_t data_size = header.size >= 4 ? header.size - 4 : 0;
            
            // Calculate padding
            m_current.total_size_with_padding = header.size;
            if (header.size & 1) {
                m_current.total_size_with_padding++;
            }
            
            // Push container onto stack for nested chunk tracking
            container_state state;
            state.id = header.id;
            state.type = container_type;
            state.end_offset = header.file_offset + 8 + header.size;  // 8 bytes header + size
            state.depth = m_current.depth;
            
            m_container_stack.push(state);
            
            // Update current context
            if (header.id == LIST) {
                m_current.current_container = header.id;
            }
            
            // For RIFF containers, update the form type
            if (header.id == RIFF || header.id == RIFX || header.id == RF64) {
                m_current.current_form = container_type;
            }
            
            // No reader for container chunks
            m_current.reader = nullptr;
            
            return true;
            
        } catch (const std::exception& e) {
            THROW_PARSE(std::string("Failed to read container type: ") + e.what());
        }
    }
    
    void riff_chunk_iterator::update_container_context() {
        // Clear contexts
        m_current.current_form = std::nullopt;
        m_current.current_container = std::nullopt;
        
        // Walk stack from bottom to find current contexts
        if (!m_container_stack.empty()) {
            // Create temporary stack to preserve order
            std::stack<container_state> temp_stack;
            
            // Move all items to temp stack (reverses order)
            while (!m_container_stack.empty()) {
                temp_stack.push(m_container_stack.top());
                m_container_stack.pop();
            }
            
            // Process from bottom (outermost container)
            while (!temp_stack.empty()) {
                const auto& info = temp_stack.top();
                
                // Update form context for RIFF containers
                if ((info.id == RIFF || info.id == RIFX || info.id == RF64) && info.type) {
                    m_current.current_form = info.type;
                }
                
                // Update container context for LIST
                if (info.id == LIST && !m_current.current_container) {
                    m_current.current_container = info.id;
                }
                
                // Move back to original stack
                m_container_stack.push(temp_stack.top());
                temp_stack.pop();
            }
        }
    }
    
    void riff_chunk_iterator::parse_ds64_chunk(std::uint64_t chunk_size) {
        // ds64 chunk must be at least 24 bytes (3 * 8-byte fields)
        if (chunk_size < 24) {
            THROW_PARSE("Invalid ds64 chunk at offset " + std::to_string(m_reader->tell() - 8) +
                       ": size " + std::to_string(chunk_size) + " bytes is too small (minimum 24 bytes required)");
        }
        
        // Read the fixed fields (ds64 is ALWAYS little-endian per spec, even in RIFX)
        m_rf64_state.riff_size = m_reader->read<std::uint64_t>(byte_order::little);
        m_rf64_state.data_size = m_reader->read<std::uint64_t>(byte_order::little);
        m_rf64_state.sample_count = m_reader->read<std::uint64_t>(byte_order::little);
        
        // Debug output
        // std::cerr << "DS64: riff_size=" << m_rf64_state.riff_size 
        //           << " data_size=" << m_rf64_state.data_size << "\n";
        
        // Check if there's a table of chunk overrides
        if (chunk_size >= 28) {  // 24 bytes fixed + 4 bytes for table count
            auto table_count = m_reader->read<std::uint32_t>(byte_order::little);
            
            // Each table entry is 12 bytes: fourcc + 8-byte size
            std::uint64_t expected_size = 24 + 4 + (table_count * 12);  // 24 for fixed fields, 4 for count
            if (chunk_size < expected_size) {
                THROW_PARSE("Invalid ds64 chunk at offset " + std::to_string(m_reader->tell() - 32) +
                           ": claims " + std::to_string(table_count) + " table entries requiring " +
                           std::to_string(expected_size) + " bytes total, but chunk size is only " +
                           std::to_string(chunk_size) + " bytes");
            }
            
            // Read the override table
            for (std::uint32_t i = 0; i < table_count; i++) {
                fourcc chunk_id = m_reader->read_fourcc();
                auto chunk_size_64 = m_reader->read<std::uint64_t>(byte_order::little);
                
                // Store the override for this chunk ID
                m_rf64_state.override_table[chunk_id].push_back(chunk_size_64);
                
                // Special handling for data chunk (backwards compatibility)
                if (chunk_id == "data"_4cc && m_rf64_state.data_size == 0) {
                    m_rf64_state.data_size = chunk_size_64;
                }
            }
        }
    }
    
    std::uint64_t riff_chunk_iterator::get_size_override(fourcc id, std::uint64_t offset, std::uint32_t size_32) {
        // If not RF64, or size is not the special marker, use the 32-bit size
        if (!m_is_rf64 || size_32 != 0xFFFFFFFF) {
            return size_32;
        }
        
        // Check for specific overrides
        if (id == RF64 || id == RIFF) {
            // Root container size override
            return m_rf64_state.riff_size;
        } else if (id == "data"_4cc && m_rf64_state.data_size > 0) {
            // Data chunk size override (from fixed fields)
            return m_rf64_state.data_size;
        }
        
        // Check the override table for this chunk ID
        auto it = m_rf64_state.override_table.find(id);
        if (it != m_rf64_state.override_table.end() && !it->second.empty()) {
            // Get the index for this chunk ID (default to 0)
            size_t& index = m_rf64_state.override_index[id];
            if (index < it->second.size()) {
                std::uint64_t size = it->second[index];
                index++;  // Move to next override for this ID
                return size;
            }
        }
        
        // No override found, but size is 0xFFFFFFFF
        // This might be an error, but return the marker for now
        return size_32;  // Keep the marker value
    }
    
} // namespace iff