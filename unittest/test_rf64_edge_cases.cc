//
// Test RF64/BW64 edge cases and error conditions using generated test files
//

#include <doctest/doctest.h>
#include <iff/chunk_iterator.hh>
#include <iff/exceptions.hh>
#include <iff/fourcc.hh>
#include <iff/parse_options.hh>
#include <sstream>
#include <vector>
#include <map>
#include "test_utils.hh"

using namespace iff;
using namespace std::string_literals;

TEST_CASE("RF64 ds64 chunk validation") {
    SUBCASE("ds64 chunk too small") {
        auto data = load_test_data("rf64_invalid_ds64.rf64");
        
        std::istringstream stream(std::string(reinterpret_cast<const char*>(data.data()), data.size()));
        
        parse_options opts;
        opts.max_chunk_size = 0x200000000ULL;  // 8GB max
        
        auto iter = chunk_iterator::get_iterator(stream, opts);
        
        // Invalid ds64 should cause parse issues
        bool parsed_successfully = false;
        if (iter) {
            try {
                while (iter->has_next()) {
                    iter->next();
                    parsed_successfully = true;
                }
            } catch (const std::exception&) {
                // Expected - invalid ds64
            }
        }
        
        CHECK(!parsed_successfully);
    }
    
    SUBCASE("ds64 not first chunk") {
        auto data = load_test_data("rf64_ds64_not_first.rf64");
        
        std::istringstream stream(std::string(reinterpret_cast<const char*>(data.data()), data.size()));
        
        parse_options opts;
        opts.max_chunk_size = 0x200000000ULL;
        
        auto iter = chunk_iterator::get_iterator(stream, opts);
        
        // The RF64 container should be found
        CHECK(iter != nullptr);
        CHECK(iter->has_next());
        
        // Navigate through all chunks
        // Note: ds64 is intentionally hidden from iteration as it's metadata
        // The parser should still work even when ds64 is not first (though it violates spec)
        bool found_fmt = false;
        bool found_data = false;
        int user_visible_chunks = 0;
        
        while (iter->has_next()) {
            const auto& chunk = iter->current();
            
            // Count non-container chunks
            if (chunk.header.id != fourcc("RF64")) {
                user_visible_chunks++;
                if (chunk.header.id == fourcc("fmt ")) {
                    found_fmt = true;
                } else if (chunk.header.id == fourcc("data")) {
                    found_data = true;
                }
            }
            
            iter->next();
        }
        
        CHECK(found_fmt);
        CHECK(found_data);
        CHECK(user_visible_chunks >= 2);  // Should see at least fmt and data
    }
}

TEST_CASE("RF64 with 64-bit sizes") {
    auto data = load_test_data("rf64_basic.rf64");
    
    std::istringstream stream(std::string(reinterpret_cast<const char*>(data.data()), data.size()));
    
    parse_options opts;
    opts.max_chunk_size = 0x200000000ULL;  // 8GB max to handle large chunks
    
    auto iter = chunk_iterator::get_iterator(stream, opts);
    
    CHECK(iter != nullptr);
    if (iter && iter->has_next()) {
        const auto& chunk = iter->current();
        CHECK(chunk.header.id == fourcc("RF64"));
        
        // Verify we can iterate through chunks
        if (chunk.reader) {
            auto child_iter = chunk_iterator::get_iterator(chunk.reader->stream(), opts);
            
            // Should have fmt and data chunks (ds64 is hidden as metadata)
            int chunk_count = 0;
            bool found_fmt = false;
            bool found_data = false;
            
            while (child_iter && child_iter->has_next()) {
                const auto& child = child_iter->current();
                chunk_count++;
                if (child.header.id == fourcc("fmt ")) {
                    found_fmt = true;
                } else if (child.header.id == fourcc("data")) {
                    found_data = true;
                }
                child_iter->next();
            }
            
            CHECK(found_fmt);
            CHECK(found_data);
            CHECK(chunk_count >= 2);  // At least fmt and data
        }
    }
}

TEST_CASE("RF64 multiple chunks with same ID") {
    auto data = load_test_data("rf64_multiple_same_id.rf64");
    
    std::istringstream stream(std::string(reinterpret_cast<const char*>(data.data()), data.size()));
    
    parse_options opts;
    opts.max_chunk_size = 0x200000000ULL;
    
    auto iter = chunk_iterator::get_iterator(stream, opts);
    
    CHECK(iter != nullptr);
    if (iter && iter->has_next()) {
        const auto& chunk = iter->current();
        
        if (chunk.reader) {
            auto child_iter = chunk_iterator::get_iterator(chunk.reader->stream(), opts);
            
            // Count chunks with same ID
            std::map<fourcc, int> id_counts;
            while (child_iter && child_iter->has_next()) {
                const auto& child = child_iter->current();
                id_counts[child.header.id]++;
                child_iter->next();
            }
            
            // Check if any ID appears multiple times
            bool has_duplicates = false;
            for (const auto& [id, count] : id_counts) {
                if (count > 1) {
                    has_duplicates = true;
                    break;
                }
            }
            
            CHECK(has_duplicates);
        }
    }
}

TEST_CASE("RF64 odd-sized chunks") {
    auto data = load_test_data("rf64_odd_sized_chunks.rf64");
    
    std::istringstream stream(std::string(reinterpret_cast<const char*>(data.data()), data.size()));
    
    parse_options opts;
    opts.max_chunk_size = 0x200000000ULL;
    
    auto iter = chunk_iterator::get_iterator(stream, opts);
    
    CHECK(iter != nullptr);
    if (iter && iter->has_next()) {
        const auto& chunk = iter->current();
        
        if (chunk.reader) {
            auto child_iter = chunk_iterator::get_iterator(chunk.reader->stream(), opts);
            
            // Check that odd-sized chunks are handled properly with padding
            while (child_iter && child_iter->has_next()) {
                const auto& child = child_iter->current();
                // Verify we can read each chunk without errors
                if (child.reader) {
                    std::vector<char> buffer(1024);
                    child.reader->read(buffer.data(), buffer.size());
                    // No exception means padding was handled correctly
                }
                child_iter->next();
            }
        }
    }
}