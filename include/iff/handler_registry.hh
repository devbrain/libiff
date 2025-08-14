//
// Created by igor on 14/08/2025.
//

#pragma once

#include <functional>
#include <unordered_map>
#include <optional>
#include <iff/export_iff.h>
#include <iff/fourcc.hh>
#include <iff/chunk_types.hh>

namespace iff {
    
    // Forward declaration
    class chunk_reader;
    // Event types for chunk processing
    enum class chunk_event_type {
        begin,  // Before reading chunk data (reader available)
        end     // After chunk is processed (reader may be null)
    };
    // Event data passed to handlers
    struct chunk_event {
        chunk_event_type type;
        const chunk_header& header;
        chunk_reader* reader;  // nullptr for 'end' event
        std::optional<fourcc> current_form;
        std::optional<fourcc> current_container;
    };
    
    // Handler function type
    using chunk_handler = std::function<void(const chunk_event& event)>;
    
    // Handler registry with three-level precedence and event support
    class IFF_EXPORT handler_registry {
    public:
        // Register handler for chunk within specific FORM type
        void on_chunk_in_form(fourcc form_type, fourcc chunk_id, chunk_handler handler);
        
        // Register handler for chunk within specific container type
        void on_chunk_in_container(fourcc container_type, fourcc chunk_id, chunk_handler handler);
        
        // Register global handler for chunk ID
        void on_chunk(fourcc chunk_id, chunk_handler handler);
        
        // Emit event to all matching handlers
        void emit(const chunk_event& event) const;
        
        // Builder pattern for fluent API
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