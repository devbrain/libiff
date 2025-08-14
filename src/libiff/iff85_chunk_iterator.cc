//
// Created by igor on 13/08/2025.
//

#include "iff85_chunk_iterator.hh"
#include <iff/exceptions.hh>
#include "input.hh"
#include "iff85_chunk_reader.hh"


namespace iff {
    
    static constexpr auto FORM = "FORM"_4cc;
    static constexpr auto LIST = "LIST"_4cc;
    static constexpr auto CAT  = "CAT "_4cc;
    static constexpr auto PROP = "PROP"_4cc;
    
    iff85_chunk_iterator::iff85_chunk_iterator(std::istream& stream)
        : iff85_chunk_iterator(stream, parse_options{}) {
    }
    
    iff85_chunk_iterator::iff85_chunk_iterator(std::istream& stream, const parse_options& options)
        : chunk_iterator(options)
        , m_reader(std::make_unique<reader>(stream)) {
        m_ended = false;
        
        // Read the first chunk
        if (!read_next_chunk()) {
            m_ended = true;
        }
    }
    
    iff85_chunk_iterator::~iff85_chunk_iterator() = default;
    
    void iff85_chunk_iterator::advance() {
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
    
    bool iff85_chunk_iterator::read_next_chunk() {
        // Check if we're at the end of current container
        while (!m_container_stack.empty()) {
            auto& top = m_container_stack.top();
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
            auto chunk_size = m_reader->read<std::uint32_t>(byte_order::big);
            
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
                .is_container = (chunk_id == FORM || chunk_id == LIST || chunk_id == CAT || chunk_id == PROP),
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
            
            // For regular chunks, create a reader
            // Calculate total size including padding
            std::size_t total_size = chunk_size;
            if (chunk_size & 1) {
                total_size += 1;  // Add padding byte
            }
            
            // Create subreader for the chunk (will be limited by iff85_chunk_reader)
            auto subreader = m_reader->create_subreader(total_size);
            m_current.reader = std::make_unique<iff85_chunk_reader>(std::move(subreader), chunk_size);
            
            // Store the total size so advance() knows how much to skip
            m_current.total_size_with_padding = total_size;
            
            return true;
            
        } catch (const parse_error& ) {
            // Re-throw parse errors - they indicate real problems
            throw;
        } catch (const std::exception& ) {
            // No more chunks or error reading (likely EOF)
            return false;
        }
    }
    
    bool iff85_chunk_iterator::process_container(const chunk_header& header) {
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
        
        try {
            std::optional<fourcc> type_tag;
            std::uint64_t content_end;
            
            // CAT chunks don't have a type tag, just concatenated chunks
            if (header.id == CAT) {
                type_tag = std::nullopt;
                content_end = m_reader->tell() + header.size;
            } else {
                // FORM, LIST, and PROP have a type tag
                type_tag = m_reader->read_fourcc();
                content_end = m_reader->tell() + header.size - 4;  // -4 for type tag already read
            }
            
            // Update header with type
            m_current.header.type = type_tag;
            
            // Push container onto stack for tracking
            container_state state {
                .id = header.id,
                .type = type_tag,
                .end_offset = content_end,
                .depth = m_current.depth
            };
            m_container_stack.push(state);
            
            // Update current context
            update_container_context();
            
            // Don't create a reader for container chunks
            m_current.reader = nullptr;
            
            // For depth-first: The container is now the current item.
            // The NEXT call to advance() will read the first child inside this container.
            // The stack ensures we stay within container boundaries.
            
            return true;
            
        } catch (const std::exception& ) {
            return false;
        }
    }
    
    void iff85_chunk_iterator::update_container_context() {
        // Update current form and container based on stack
        m_current.current_form = std::nullopt;
        m_current.current_container = std::nullopt;
        
        // Search stack for innermost FORM and container
        std::stack<container_state> temp = m_container_stack;
        while (!temp.empty()) {
            const auto& state = temp.top();
            
            if (state.id == FORM && !m_current.current_form) {
                m_current.current_form = state.type;
            }
            
            if ((state.id == LIST || state.id == CAT || state.id == PROP) && !m_current.current_container) {
                m_current.current_container = state.id;  // Store the container ID itself for tracking
            }
            
            temp.pop();
        }
    }
    
    
} // namespace iff