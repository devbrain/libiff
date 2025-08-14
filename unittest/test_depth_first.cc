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
#include "../src/libiff/input.hh"

using namespace iff;
using namespace std::string_literals;

static std::unique_ptr<std::ifstream> load_test(const std::string& name) {
    static std::filesystem::path root(UNITTEST_PATH_TO_GENERATED_FILES);
    auto path = root / name;
    return std::make_unique<std::ifstream>(path, std::ios::binary);
}

TEST_CASE("verify depth-first traversal") {
    SUBCASE("deeply nested structure") {
        auto is = load_test("deeply_nested.iff");
        
        // Track traversal order with indentation to show structure
        std::vector<std::string> traversal;
        
        auto it = chunk_iterator::get_iterator(*is);
        
        while (it->has_next()) {
            auto& chunk = it->current();
            
            // Create indentation based on depth
            std::string indent(chunk.depth * 2, ' ');
            
            if (chunk.header.is_container) {
                std::string entry = indent + chunk.header.id.to_string() + ":" + 
                                   (chunk.header.type ? chunk.header.type->to_string() : "null");
                traversal.push_back(entry);
            } else {
                traversal.push_back(indent + chunk.header.id.to_string());
            }
            it->next();
        }
        
        // Print traversal for visualization
        std::cout << "\nDepth-first traversal:\n";
        for (const auto& entry : traversal) {
            std::cout << entry << "\n";
        }
        
        // Verify depth-first order:
        // We should see FORM:TST1, then immediately its children,
        // not jumping to a sibling at the same level
        CHECK(traversal[0] == "FORM:TST1");      // Root container
        CHECK(traversal[1] == "  LIST:TST2");    // First child of FORM:TST1
        CHECK(traversal[2] == "    FORM:TST3");  // First child of LIST:TST2
        CHECK(traversal[3] == "      LIST:TST4"); // First child of FORM:TST3
        // ... continues diving deeper
        
        // The DATA chunk should be last, at the deepest level
        CHECK(traversal.back() == "                  DATA");
        
        // Verify we visited all 10 items (9 containers + 1 data chunk)
        CHECK(traversal.size() == 10);
    }
    
    SUBCASE("compare with breadth-first") {
        // If this were breadth-first, we would see:
        // FORM:TST1, then LIST:TST2, FORM:TST3, LIST:TST4, etc.
        // (all at each level before going deeper)
        
        auto is = load_test("deeply_nested.iff");
        
        std::vector<int> depths;
        
        auto it = chunk_iterator::get_iterator(*is);
        
        while (it->has_next()) {
            depths.push_back(it->current().depth);
            it->next();
        }
        
        // In depth-first, depths should generally increase until we hit bottom,
        // then potentially decrease as we come back up
        bool found_max_depth = false;
        int max_depth_seen = 0;
        
        for (size_t i = 0; i < depths.size(); ++i) {
            if (depths[i] > max_depth_seen) {
                max_depth_seen = depths[i];
            }
            
            // Once we hit max depth (9 for DATA), we shouldn't go deeper
            if (depths[i] == 9) {
                found_max_depth = true;
            }
            
            // In depth-first, we go as deep as possible before backing out
            // So we should hit depth 9 (the DATA chunk) as the last item
            if (i == depths.size() - 1) {
                CHECK(depths[i] == 9);
            }
        }
        
        CHECK(found_max_depth);
        CHECK(max_depth_seen == 9);
    }
    
    SUBCASE("LIST container handling") {
        // Create a simple test to verify LIST is handled correctly
        // LIST should have a type tag and contain multiple items
        
        auto is = load_test("deeply_nested.iff");
        
        auto it = chunk_iterator::get_iterator(*is);
        
        // Find a LIST container
        while (it->has_next()) {
            if (it->current().header.id == "LIST"_4cc) {
                // LIST should have a type tag
                CHECK(it->current().header.type.has_value());
                
                // Move to next item - should be inside the LIST
                it->next();
                if (it->has_next()) {
                    // Next item should be at a deeper level
                    CHECK(it->current().depth > 1);
                }
                break;
            }
            it->next();
        }
    }
}