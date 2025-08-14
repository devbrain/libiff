//
// Created by igor on 13/08/2025.
//

#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "unittest_config.h"
#include <iff/parser.hh>

using namespace iff;
using namespace std::string_literals;

static std::unique_ptr<std::ifstream> load_test(const std::string& name) {
    static std::filesystem::path root(UNITTEST_PATH_TO_GENERATED_FILES);
    auto path = root / name;
    return std::make_unique<std::ifstream>(path, std::ios::binary);
}

TEST_CASE("test simple parser") {
    SUBCASE("parse with handlers") {
        auto is = load_test("odd_sized_chunks.iff");
        
        std::vector<std::string> chunks_seen;
        
        handler_registry handlers;
        handlers.on_chunk("ODD1"_4cc, [&](const chunk_event& event) {
            if (event.type == chunk_event_type::begin) {
                chunks_seen.push_back("ODD1");
            }
        });
        handlers.on_chunk("EVN2"_4cc, [&](const chunk_event& event) {
            if (event.type == chunk_event_type::begin) {
                chunks_seen.push_back("EVN2");
            }
        });
        
        // Simple one-line parse!
        parse(*is, handlers);
        
        CHECK(chunks_seen.size() == 2);
        CHECK(chunks_seen[0] == "ODD1");
        CHECK(chunks_seen[1] == "EVN2");
    }
    
    SUBCASE("for_each_chunk lambda") {
        auto is = load_test("odd_sized_chunks.iff");
        
        std::vector<std::string> chunk_ids;
        std::vector<std::uint64_t> chunk_sizes;
        
        // Even simpler - just a lambda!
        for_each_chunk(*is, [&](const auto& chunk) {
            chunk_ids.push_back(chunk.header.id.to_string());
            chunk_sizes.push_back(chunk.header.size);
        });
        
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
    
    SUBCASE("process chunk data") {
        auto is = load_test("odd_sized_chunks.iff");
        
        std::size_t total_bytes = 0;
        
        for_each_chunk(*is, [&](const auto& chunk) {
            if (chunk.reader) {
                auto data = chunk.reader->read_all();
                total_bytes += data.size();
            }
        });
        
        CHECK(total_bytes == 15);  // 1 + 2 + 3 + 5 + 4
    }
    
    SUBCASE("filter specific chunks") {
        auto is = load_test("odd_sized_chunks.iff");
        
        std::vector<std::string> odd_chunks;
        
        for_each_chunk(*is, [&](const auto& chunk) {
            // Only process chunks starting with "ODD"
            std::string id = chunk.header.id.to_string();
            if (id.substr(0, 3) == "ODD") {
                odd_chunks.push_back(id);
            }
        });
        
        CHECK(odd_chunks.size() == 3);
        CHECK(odd_chunks[0] == "ODD1");
        CHECK(odd_chunks[1] == "ODD3");
        CHECK(odd_chunks[2] == "ODD5");
    }
}