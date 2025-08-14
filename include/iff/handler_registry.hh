/**
 * @file handler_registry.hh
 * @brief Event handler registry for chunk processing
 * @author Igor
 * @date 14/08/2025
 */

#pragma once

#include <functional>
#include <unordered_map>
#include <optional>
#include <iff/export_iff.h>
#include <iff/fourcc.hh>
#include <iff/chunk_header.hh>

namespace iff {
    
    // Forward declaration
    class chunk_reader;
    
    /**
     * @enum chunk_event_type
     * @brief Types of events emitted during chunk processing
     */
    enum class chunk_event_type {
        begin,  ///< Emitted before reading chunk data (reader available)
        end     ///< Emitted after chunk is processed (reader may be null)
    };
    /**
     * @struct chunk_event
     * @brief Event data passed to chunk handlers
     * 
     * Contains all information about the current chunk being processed,
     * including its header, reader (for begin events), and context.
     */
    struct chunk_event {
        chunk_event_type type;                   ///< Type of event (begin/end)
        const chunk_header& header;              ///< Chunk header information
        chunk_reader* reader;                    ///< Reader for chunk data (nullptr for 'end' event)
        std::optional<fourcc> current_form;      ///< Current FORM type if inside a FORM
        std::optional<fourcc> current_container; ///< Current container type (LIST/CAT/PROP)
        
        // Delete default constructor - must provide at least type and header
        chunk_event() = delete;
        
        // Constructor with all fields
        chunk_event(chunk_event_type t, 
                   const chunk_header& h, 
                   chunk_reader* r,
                   std::optional<fourcc> form = std::nullopt,
                   std::optional<fourcc> container = std::nullopt)
            : type(t), header(h), reader(r), current_form(form), current_container(container) {}
    };
    
    /**
     * @typedef chunk_handler
     * @brief Function type for chunk event handlers
     */
    using chunk_handler = std::function<void(const chunk_event& event)>;
    
    /**
     * @class handler_registry
     * @brief Registry for chunk event handlers with precedence rules
     * 
     * Supports three levels of handler specificity:
     * 1. FORM-specific handlers (highest precedence)
     * 2. Container-specific handlers
     * 3. Global handlers (lowest precedence)
     * 
     * Multiple handlers can be registered for the same chunk type.
     */
    class IFF_EXPORT handler_registry {
    public:
        /**
         * @brief Register handler for chunks within a specific FORM type
         * @param form_type FORM type to match
         * @param chunk_id Chunk ID to handle
         * @param handler Handler function to call
         */
        void on_chunk_in_form(fourcc form_type, fourcc chunk_id, chunk_handler handler);
        
        /**
         * @brief Register handler for chunks within a specific container type
         * @param container_type Container type (LIST/CAT/PROP) to match
         * @param chunk_id Chunk ID to handle
         * @param handler Handler function to call
         */
        void on_chunk_in_container(fourcc container_type, fourcc chunk_id, chunk_handler handler);
        
        /**
         * @brief Register global handler for a chunk ID
         * @param chunk_id Chunk ID to handle
         * @param handler Handler function to call
         */
        void on_chunk(fourcc chunk_id, chunk_handler handler);
        
        /**
         * @brief Emit an event to all matching handlers
         * @param event Event to emit
         * 
         * Handlers are called in precedence order: FORM-specific first,
         * then container-specific, then global.
         */
        void emit(const chunk_event& event) const;
        
        /**
         * @brief Create a builder for fluent API
         * @return Builder instance
         */
        class builder;
        static builder make();
        
    private:
        struct chunk_key {
            fourcc scope;
            fourcc id;
            
            bool operator==(const chunk_key& other) const;
        };
        
        struct chunk_key_hash {
            std::size_t operator()(const chunk_key& key) const noexcept;
        };
        
        // Use multimap to allow multiple handlers per chunk
        std::unordered_multimap<chunk_key, chunk_handler, chunk_key_hash> form_handlers_;
        std::unordered_multimap<chunk_key, chunk_handler, chunk_key_hash> container_handlers_;
        std::unordered_multimap<fourcc, chunk_handler> global_handlers_;
    };
    
} // namespace iff