//
// Created by igor on 13/08/2025.
//

#pragma once

#include <memory>
#include <stack>
#include <optional>
#include <iff/fourcc.hh>
#include <iff/chunk_types.hh>
#include <iff/chunk_reader.hh>
#include <iff/parse_options.hh>
#include <iff/export_iff.h>

namespace iff {
    
    class reader_base;
    
    // Chunk iterator for depth-first traversal of IFF/RIFF files
    class IFF_EXPORT chunk_iterator {
    public:
        // Factory method - auto-detects format and returns appropriate iterator
        static std::unique_ptr<chunk_iterator> get_iterator(std::istream& stream);
        static std::unique_ptr<chunk_iterator> get_iterator(std::istream& stream, const parse_options& options);
        
        // Information about current chunk
        struct chunk_info {
            chunk_header header;
            std::unique_ptr<chunk_reader> reader;  // nullptr if already consumed
            std::optional<fourcc> current_form;    // Current FORM type if inside FORM
            std::optional<fourcc> current_container; // Current container type (LIST/CAT)
            int depth;                              // Nesting depth (0 = top level)
            std::size_t total_size_with_padding = 0; // For IFF-85, includes padding
        };
        
        // Destructor
        virtual ~chunk_iterator() = default;
        
        // Core interface methods (better for polymorphic use)
        const chunk_info& current() const { return m_current; }
        chunk_info& current() { return m_current; }
        
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
            fourcc id;                    // FORM/LIST/CAT
            std::optional<fourcc> type;   // Type tag for FORM/LIST
            std::uint64_t end_offset;     // Where this container ends
            int depth;
        };
        std::stack<container_state> m_container_stack;
    };
    
    
} // namespace iff