/**
 * @file parser.hh
 * @brief IFF/RIFF file parsing utilities
 * @author Igor
 * @date 12/08/2025
 */

#pragma once

#include <iosfwd>
#include <iff/handler_registry.hh>
#include <iff/chunk_iterator.hh>
#include <iff/parse_options.hh>

namespace iff {
    
    /**
     * @brief Parse a stream with handler registry and custom options
     * 
     * Iterates through chunks in the stream and emits begin/end events
     * to registered handlers for each non-container chunk.
     * 
     * @param stream Input stream containing IFF/RIFF data
     * @param handlers Registry of event handlers to process chunks
     * @param options Parse options for controlling parsing behavior
     */
    inline void parse(std::istream& stream, const handler_registry& handlers, const parse_options& options) {
        auto it = chunk_iterator::get_iterator(stream, options);
        
        while (it->has_next()) {
            auto& chunk = it->current();
            
            // Skip containers - they're just structure
            if (chunk.header.is_container) {
                it->next();
                continue;
            }
            
            // Emit begin event
            chunk_event begin_event(
                chunk_event_type::begin,
                chunk.header,
                chunk.reader.get(),
                chunk.current_form,
                chunk.current_container
            );
            handlers.emit(begin_event);
            
            // Emit end event
            chunk_event end_event(
                chunk_event_type::end,
                chunk.header,
                nullptr,
                chunk.current_form,
                chunk.current_container
            );
            handlers.emit(end_event);
            
            it->next();
        }
    }
    
    /**
     * @brief Parse a stream with handler registry
     * 
     * Iterates through chunks in the stream and emits begin/end events
     * to registered handlers for each non-container chunk.
     * Uses default parse options.
     * 
     * @param stream Input stream containing IFF/RIFF data
     * @param handlers Registry of event handlers to process chunks
     */
    inline void parse(std::istream& stream, const handler_registry& handlers) {
        parse(stream, handlers, parse_options{});
    }
    
    /**
     * @brief Simple functional interface for iterating chunks with custom options
     * 
     * Calls the provided function for each non-container chunk in the stream.
     * 
     * @tparam Func Callable type accepting chunk_iterator::chunk_info&
     * @param stream Input stream containing IFF/RIFF data
     * @param func Function to call for each chunk
     * @param options Parse options for controlling parsing behavior
     */
    template<typename Func>
    void for_each_chunk(std::istream& stream, Func func, const parse_options& options) {
        auto it = chunk_iterator::get_iterator(stream, options);
        
        while (it->has_next()) {
            auto& chunk = it->current();
            
            if (!chunk.header.is_container) {
                func(chunk);
            }
            
            it->next();
        }
    }
    
    /**
     * @brief Simple functional interface for iterating chunks
     * 
     * Calls the provided function for each non-container chunk in the stream.
     * Uses default parse options.
     * 
     * @tparam Func Callable type accepting chunk_iterator::chunk_info&
     * @param stream Input stream containing IFF/RIFF data
     * @param func Function to call for each chunk
     */
    template<typename Func>
    void for_each_chunk(std::istream& stream, Func func) {
        for_each_chunk(stream, func, parse_options{});
    }
    
} // namespace iff