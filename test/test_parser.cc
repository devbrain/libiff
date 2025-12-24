//
// Created by igor on 13/08/2025.
//

#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>

#include "unittest_config.h"
#include <iff/parser.hh>

static std::unique_ptr<std::istream> load_test(const std::string& name) {
    static std::filesystem::path root(UNITTEST_PATH_TO_GENERATED_FILES);
    auto path = root / name;
    return std::make_unique<std::ifstream>(path, std::ios::binary);
}

TEST_CASE("test iff parsing") {
    SUBCASE("deeply nested") {
        auto is = load_test("deeply_nested.iff");
        
        int chunk_count = 0;
        CHECK_NOTHROW(
            iff::for_each_chunk(*is, [&](const auto& chunk) {
                chunk_count++;
            })
        );
        CHECK(chunk_count == 1);  // Only DATA chunk (containers are skipped)
    }
    
    SUBCASE("odd sized chunks") {
        auto is = load_test("odd_sized_chunks.iff");
        
        int chunk_count = 0;
        CHECK_NOTHROW(
            iff::for_each_chunk(*is, [&](const auto& chunk) {
                chunk_count++;
            })
        );
        CHECK(chunk_count == 5);  // 5 data chunks
    }
}