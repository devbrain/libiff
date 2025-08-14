//
// Created by igor on 13/08/2025.
//

#pragma once

#include <iff/chunk_iterator.hh>
#include "input.hh"
#include <memory>

namespace iff {
    
    // IFF-85 specific iterator (internal implementation)
    class iff85_chunk_iterator : public chunk_iterator {
    public:
        // Construct from stream
        explicit iff85_chunk_iterator(std::istream& stream);
        iff85_chunk_iterator(std::istream& stream, const parse_options& options);
        
        // End iterator
        iff85_chunk_iterator() : chunk_iterator() {}
        
        ~iff85_chunk_iterator() override;
        
    protected:
        void advance() override;
        
    private:
        // Read next chunk at current position
        bool read_next_chunk();
        
        // Handle container chunks (FORM/LIST/CAT)
        bool process_container(const chunk_header& header);
        
        // Check if current position is inside a container
        void update_container_context();
        
        std::unique_ptr<reader_base> m_reader;
    };
    
} // namespace iff