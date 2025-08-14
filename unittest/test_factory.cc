//
// Created by igor on 13/08/2025.
//

#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <sstream>

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

TEST_CASE("test factory method") {
    SUBCASE("auto-detect IFF format") {
        auto is = load_test("odd_sized_chunks.iff");
        
        // Factory method automatically detects IFF format
        auto it = chunk_iterator::get_iterator(*is);
        
        // Should successfully iterate
        CHECK(it->has_next());
        CHECK(it->current().header.id == "FORM"_4cc);
    }
    
    SUBCASE("polymorphic usage") {
        auto is = load_test("deeply_nested.iff");
        
        // Can use the abstract base type without knowing concrete type
        std::unique_ptr<chunk_iterator> it = chunk_iterator::get_iterator(*is);
        
        // Count all chunks (including containers)
        int chunk_count = 0;
        while (it->has_next()) {
            chunk_count++;
            it->next();
        }
        
        CHECK(chunk_count == 10);  // 9 containers + 1 data chunk
    }
    
    SUBCASE("works with any stream") {
        // Create IFF data in memory
        std::string iff_data;
        iff_data += "FORM";  // ID
        iff_data += std::string("\x00\x00\x00\x0C", 4);  // Size = 12 (big endian)
        iff_data += "TEST";  // Type
        iff_data += "DATA";  // Chunk ID
        iff_data += std::string("\x00\x00\x00\x04", 4);  // Size = 4
        iff_data += "abcd";  // Data
        
        std::istringstream stream(iff_data);
        
        // Factory works with any istream
        auto it = chunk_iterator::get_iterator(stream);
        
        CHECK(it->has_next());
        CHECK(it->current().header.id == "FORM"_4cc);
        CHECK(it->current().header.type.value() == "TEST"_4cc);
        
        it->next();
        CHECK(it->current().header.id == "DATA"_4cc);
        CHECK(it->current().header.size == 4);
    }
    
    SUBCASE("error on unknown format") {
        std::string bad_data = "BADX\x00\x00\x00\x00";
        std::istringstream stream(bad_data);
        
        CHECK_THROWS_AS(
            chunk_iterator::get_iterator(stream),
            iff::parse_error
        );
    }
}