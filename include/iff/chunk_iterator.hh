/**
 * @file chunk_iterator.hh
 * @brief Chunk iterator for depth-first traversal of IFF/RIFF files
 * @author Igor
 * @date 13/08/2025
 */

#pragma once

#include <memory>
#include <stack>
#include <optional>
#include <iff/fourcc.hh>
#include <iff/chunk_header.hh>
#include <iff/chunk_reader.hh>
#include <iff/parse_options.hh>
#include <iff/export_iff.h>

namespace iff {
    
    class reader_base;
    
    /**
     * @class chunk_iterator
     * @brief Iterator for depth-first traversal of IFF/RIFF file chunks
     * 
     * This class provides a unified interface for iterating through chunks
     * in both IFF-85 and RIFF file formats. It automatically detects the
     * format and handles nested container structures.
     */
    class IFF_EXPORT chunk_iterator {
    public:
        /**
         * @brief Factory method that auto-detects format and returns appropriate iterator
         * @param stream Input stream to read from
         * @return Unique pointer to the appropriate chunk_iterator implementation
         */
        static std::unique_ptr<chunk_iterator> get_iterator(std::istream& stream);
        
        /**
         * @brief Factory method with custom parse options
         * @param stream Input stream to read from
         * @param options Parse options for handling various file format edge cases
         * @return Unique pointer to the appropriate chunk_iterator implementation
         */
        static std::unique_ptr<chunk_iterator> get_iterator(std::istream& stream, const parse_options& options);
        
        /**
         * @struct chunk_info
         * @brief Information about the current chunk being iterated
         */
        struct chunk_info {
            chunk_header header;                    ///< Header information for the current chunk
            std::unique_ptr<chunk_reader> reader;   ///< Reader for chunk data (nullptr if already consumed)
            std::optional<fourcc> current_form;     ///< Current FORM type if inside FORM container
            std::optional<fourcc> current_container;///< Current container type (LIST/CAT/PROP)
            int depth;                               ///< Nesting depth (0 = top level)
            std::size_t total_size_with_padding = 0;///< Total size including padding (IFF-85 only)
            
            // PROP support (IFF-85 only)
            bool in_list_with_props = false;        ///< True if current LIST has PROP chunks
            bool is_prop_chunk = false;             ///< True if this is a PROP chunk
        };
        
        /**
         * @brief Virtual destructor
         */
        virtual ~chunk_iterator() = default;
        
        /**
         * @brief Get current chunk information (const version)
         * @return Const reference to current chunk information
         */
        const chunk_info& current() const { return m_current; }
        
        /**
         * @brief Get current chunk information (mutable version)
         * @return Reference to current chunk information
         */
        chunk_info& current() { return m_current; }
        
        /**
         * @brief Advance to the next chunk
         */
        void next() {
            advance();
        }
        
        bool has_next() const { return !m_ended; }
        bool at_end() const { return m_ended; }
        
    protected:
        // Constructor for derived classes
        chunk_iterator() : m_ended(true), m_current{} {}
        chunk_iterator(const parse_options& opts) : m_ended(true), m_current{}, m_options(opts) {}
        
        // Advance to next chunk (to be implemented by derived classes)
        virtual void advance() = 0;
        
        // Current chunk information
        chunk_info m_current;
        bool m_ended;
        
        // Parse options for safety limits
        parse_options m_options;
        
        // Container tracking for depth-first traversal
        struct container_state {
            fourcc id;                    // FORM/LIST/CAT/PROP
            std::optional<fourcc> type;   // Type tag for FORM/LIST/PROP
            std::uint64_t end_offset;     // Where this container ends
            int depth;
            bool has_prop_chunks = false; // For LIST: true if contains PROP chunks
        };
        std::stack<container_state> m_container_stack;
    };
    
    
} // namespace iff