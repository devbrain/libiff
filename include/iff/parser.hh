//
// Created by igor on 12/08/2025.
//

#pragma once

#include <iosfwd>
#include <iff/handler_registry.hh>
#include <iff/chunk_iterator.hh>

namespace iff {
    
    // Parse with handler registry - iterates chunks and emits events
    inline void parse(std::istream& stream, const handler_registry& handlers) {
        auto it = chunk_iterator::get_iterator(stream);
        
        while (it->has_next()) {
            auto& chunk = it->current();
            
            // Skip containers - they're just structure
            if (chunk.header.is_container) {
                it->next();
                continue;
            }
            
            // Emit begin event
            chunk_event begin_event{
                .type = chunk_event_type::begin,
                .header = chunk.header,
                .reader = chunk.reader.get(),
                .current_form = chunk.current_form,
                .current_container = chunk.current_container
            };
            handlers.emit(begin_event);
            
            // Emit end event
            chunk_event end_event{
                .type = chunk_event_type::end,
                .header = chunk.header,
                .reader = nullptr,
                .current_form = chunk.current_form,
                .current_container = chunk.current_container
            };
            handlers.emit(end_event);
            
            it->next();
        }
    }
    
    // Simple functional interface - just call a lambda for each chunk
    template<typename Func>
    void for_each_chunk(std::istream& stream, Func func) {
        auto it = chunk_iterator::get_iterator(stream);
        
        while (it->has_next()) {
            auto& chunk = it->current();
            
            if (!chunk.header.is_container) {
                func(chunk);
            }
            
            it->next();
        }
    }
    
} // namespace iff