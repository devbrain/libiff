//
// Created by igor on 14/08/2025.
//

#include <iff/handler_registry.hh>
#include <algorithm>

namespace iff {
    
    // Handler registry implementation
    bool handler_registry::chunk_key::operator==(const chunk_key& other) const {
        return scope == other.scope && id == other.id;
    }
    
    std::size_t handler_registry::chunk_key_hash::operator()(const chunk_key& key) const noexcept {
        return std::hash<std::uint32_t>{}(key.scope.to_uint32()) ^
               (std::hash<std::uint32_t>{}(key.id.to_uint32()) << 1);
    }
    
    void handler_registry::on_chunk_in_form(fourcc form_type, fourcc chunk_id, chunk_handler handler) {
        form_handlers_.emplace(chunk_key{form_type, chunk_id}, std::move(handler));
    }
    
    void handler_registry::on_chunk_in_container(fourcc container_type, fourcc chunk_id, chunk_handler handler) {
        container_handlers_.emplace(chunk_key{container_type, chunk_id}, std::move(handler));
    }
    
    void handler_registry::on_chunk(fourcc chunk_id, chunk_handler handler) {
        global_handlers_.emplace(chunk_id, std::move(handler));
    }
    
    void handler_registry::emit(const chunk_event& event) const {
        // Collect all matching handlers with proper precedence
        std::vector<const chunk_handler*> handlers_to_call;
        
        // First collect form-specific handlers (highest priority)
        if (event.current_form) {
            auto range = form_handlers_.equal_range({*event.current_form, event.header.id});
            for (auto it = range.first; it != range.second; ++it) {
                handlers_to_call.push_back(&it->second);
            }
        }
        
        // Then container-specific handlers (medium priority)
        if (event.current_container) {
            auto range = container_handlers_.equal_range({*event.current_container, event.header.id});
            for (auto it = range.first; it != range.second; ++it) {
                handlers_to_call.push_back(&it->second);
            }
        }
        
        // Finally global handlers (lowest priority)
        auto range = global_handlers_.equal_range(event.header.id);
        for (auto it = range.first; it != range.second; ++it) {
            handlers_to_call.push_back(&it->second);
        }
        
        // Call all collected handlers
        for (const auto* handler : handlers_to_call) {
            (*handler)(event);
        }
    }
    
} // namespace iff