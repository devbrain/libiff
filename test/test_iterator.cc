//
// Created by igor on 13/08/2025.
//

#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

#include "unittest_config.h"
#include <iff/chunk_iterator.hh>

using namespace iff;
using namespace std::string_literals;

static std::unique_ptr<std::ifstream> load_test(const std::string& name) {
    static std::filesystem::path root(UNITTEST_PATH_TO_GENERATED_FILES);
    auto path = root / name;
    return std::make_unique<std::ifstream>(path, std::ios::binary);
}

TEST_CASE("test chunk iterator") {
    SUBCASE("iterate simple chunks") {
        auto is = load_test("odd_sized_chunks.iff");
        
        std::vector<std::string> chunk_ids;
        std::vector<std::uint64_t> chunk_sizes;
        
        auto it = chunk_iterator::get_iterator(*is);
        
        while (it->has_next()) {
            auto& chunk = it->current();
            
            // Skip containers
            if (chunk.header.is_container) {
                it->next();
                continue;
            }
            
            chunk_ids.push_back(chunk.header.id.to_string());
            chunk_sizes.push_back(chunk.header.size);
            
            // Read and verify we get the right amount of data
            if (chunk.reader) {
                auto data = chunk.reader->read_all();
                CHECK(data.size() == chunk.header.size);
            }
            
            it->next();
        }
        
        // Verify we got all chunks in order
        CHECK(chunk_ids.size() == 5);
        CHECK(chunk_ids[0] == "ODD1");
        CHECK(chunk_ids[1] == "EVN2");
        CHECK(chunk_ids[2] == "ODD3");
        CHECK(chunk_ids[3] == "ODD5");
        CHECK(chunk_ids[4] == "EVN4");
        
        CHECK(chunk_sizes[0] == 1);
        CHECK(chunk_sizes[1] == 2);
        CHECK(chunk_sizes[2] == 3);
        CHECK(chunk_sizes[3] == 5);
        CHECK(chunk_sizes[4] == 4);
    }
    
    SUBCASE("iterate nested containers depth-first") {
        auto is = load_test("deeply_nested.iff");
        
        std::vector<std::string> items;
        std::vector<int> depths;
        
        auto it = chunk_iterator::get_iterator(*is);
        
        while (it->has_next()) {
            auto& chunk = it->current();
            
            if (chunk.header.is_container) {
                items.push_back(chunk.header.id.to_string() + ":" + 
                               (chunk.header.type ? chunk.header.type->to_string() : ""));
            } else {
                items.push_back(chunk.header.id.to_string());
            }
            depths.push_back(chunk.depth);
            it->next();
        }
        
        // Verify depth-first traversal
        CHECK(items.size() == 10); // 9 containers + 1 DATA chunk
        CHECK(items[0] == "FORM:TST1");
        CHECK(items[1] == "LIST:TST2");
        CHECK(items[2] == "FORM:TST3");
        CHECK(items[3] == "LIST:TST4");
        CHECK(items[4] == "FORM:TST5");
        CHECK(items[5] == "LIST:TST6");
        CHECK(items[6] == "FORM:TST7");
        CHECK(items[7] == "LIST:TST8");
        CHECK(items[8] == "FORM:TST9");
        CHECK(items[9] == "DATA");
        
        // Verify depths
        CHECK(depths[0] == 0);  // FORM:TST1
        CHECK(depths[1] == 1);  // LIST:TST2
        CHECK(depths[2] == 2);  // FORM:TST3
        CHECK(depths[9] == 9);  // DATA chunk at deepest level
    }
    
    SUBCASE("context tracking") {
        auto is = load_test("odd_sized_chunks.iff");
        
        auto it = chunk_iterator::get_iterator(*is);
        
        // First should be the FORM container
        CHECK(it->current().header.id == "FORM"_4cc);
        CHECK(it->current().header.is_container);
        CHECK(it->current().header.type.has_value());
        CHECK(*it->current().header.type == "TEST"_4cc);
        
        it->next();
        
        // Next chunks should have FORM context
        CHECK(it->current().header.id == "ODD1"_4cc);
        CHECK(it->current().current_form.has_value());
        CHECK(*it->current().current_form == "TEST"_4cc);
        CHECK(!it->current().current_container.has_value());  // Not in LIST/CAT
    }
    
    SUBCASE("skip unused chunks") {
        auto is = load_test("odd_sized_chunks.iff");
        
        int chunks_processed = 0;
        
        auto it = chunk_iterator::get_iterator(*is);
        
        while (it->has_next()) {
            // Just count, don't read - iterator should handle skipping
            if (!it->current().header.is_container) {
                chunks_processed++;
            }
            it->next();
        }
        
        CHECK(chunks_processed == 5);
    }
}