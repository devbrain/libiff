//
// Created by igor on 14/08/2025.
//
// Comprehensive edge case tests for IFF parser

#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <numeric>

#include "unittest_config.h"
#include <iff/parser.hh>
#include <iff/chunk_iterator.hh>
#include <iff/exceptions.hh>

using namespace iff;
using namespace std::string_literals;

static std::unique_ptr<std::ifstream> load_test(const std::string& name) {
    static std::filesystem::path root(UNITTEST_PATH_TO_GENERATED_FILES);
    auto path = root / name;
    return std::make_unique<std::ifstream>(path, std::ios::binary);
}

TEST_CASE("edge cases - zero sized chunks") {
    SUBCASE("zero_sized_chunks.iff") {
        auto is = load_test("zero_sized_chunks.iff");
        REQUIRE(is->good());
        
        std::vector<std::pair<std::string, std::uint64_t>> chunks;
        
        for_each_chunk(*is, [&](const auto& chunk) {
            chunks.push_back({chunk.header.id.to_string(), chunk.header.size});
            
            // Zero-sized chunks should still have valid readers
            REQUIRE(chunk.reader != nullptr);
            CHECK(chunk.reader->size() == chunk.header.size);
            CHECK(chunk.reader->remaining() == chunk.header.size);
            CHECK(chunk.reader->offset() == 0);
            
            // Reading from zero-sized chunk should return empty
            if (chunk.header.size == 0) {
                auto data = chunk.reader->read_all();
                CHECK(data.empty());
                CHECK(chunk.reader->remaining() == 0);
            }
        });
        
        // Verify we have the expected chunks
        REQUIRE(chunks.size() == 4);
        CHECK(chunks[0].first == "ZERO");
        CHECK(chunks[0].second == 0);
        CHECK(chunks[1].first == "DATA");
        CHECK(chunks[1].second == 2);  // "ABCD" in hex is 2 bytes (0xABCD)
        CHECK(chunks[2].first == "NULL");
        CHECK(chunks[2].second == 0);
        CHECK(chunks[3].first == "MORE");
        CHECK(chunks[3].second == 5);  // "1234567890" in hex is 5 bytes
    }
}

TEST_CASE("edge cases - empty containers") {
    SUBCASE("empty_containers.iff") {
        auto is = load_test("empty_containers.iff");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        std::vector<std::string> structure;
        std::vector<int> depths;
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            depths.push_back(chunk.depth);
            
            if (chunk.header.is_container) {
                structure.push_back(chunk.header.id.to_string() + ":" + 
                    (chunk.header.type ? chunk.header.type->to_string() : ""));
            } else {
                structure.push_back(chunk.header.id.to_string());
            }
            
            it->next();
        }
        
        // Check structure includes empty containers
        CHECK(structure.size() >= 5);
        
        // Find empty FORM
        auto form_it = std::find(structure.begin(), structure.end(), "FORM:EMTY");
        CHECK(form_it != structure.end());
        
        // Find empty LIST
        auto list_it = std::find(structure.begin(), structure.end(), "LIST:VOID");
        CHECK(list_it != structure.end());
        
        // Verify DATA chunks exist
        CHECK(std::count(structure.begin(), structure.end(), "DATA") == 2);
    }
}

TEST_CASE("edge cases - CAT container") {
    SUBCASE("cat_container.iff") {
        auto is = load_test("cat_container.iff");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        bool found_cat = false;
        std::vector<std::string> form_types;
        std::vector<std::string> data_chunks;
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            
            if (chunk.header.id == "CAT "_4cc) {
                found_cat = true;
                CHECK(chunk.header.is_container);
                // CAT should not have a type tag
                CHECK(!chunk.header.type.has_value());
            } else if (chunk.header.id == "FORM"_4cc && chunk.header.type) {
                form_types.push_back(chunk.header.type->to_string());
            } else if (!chunk.header.is_container) {
                data_chunks.push_back(chunk.header.id.to_string());
            }
            
            it->next();
        }
        
        CHECK(found_cat);
        
        // Should have 3 FORMs
        CHECK(form_types.size() == 3);
        CHECK(form_types[0] == "TST1");
        CHECK(form_types[1] == "TST2");
        CHECK(form_types[2] == "TST3");
        
        // Should have 3 data chunks
        CHECK(data_chunks.size() == 3);
        CHECK(data_chunks[0] == "DAT1");
        CHECK(data_chunks[1] == "DAT2");
        CHECK(data_chunks[2] == "DAT3");
    }
}

TEST_CASE("edge cases - containers only") {
    SUBCASE("containers_only.iff") {
        auto is = load_test("containers_only.iff");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        int container_count = 0;
        int data_chunk_count = 0;
        int max_depth = 0;
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            max_depth = std::max(max_depth, chunk.depth);
            
            if (chunk.header.is_container) {
                container_count++;
            } else {
                data_chunk_count++;
            }
            
            it->next();
        }
        
        // Should have only containers, no data chunks
        CHECK(container_count > 0);
        CHECK(data_chunk_count == 0);
        CHECK(max_depth >= 2);  // Nested containers
    }
}

TEST_CASE("edge cases - many small chunks") {
    SUBCASE("many_small_chunks.iff") {
        auto is = load_test("many_small_chunks.iff");
        REQUIRE(is->good());
        
        std::vector<std::string> chunk_ids;
        std::vector<std::uint64_t> chunk_sizes;
        int total_bytes = 0;
        
        for_each_chunk(*is, [&](const auto& chunk) {
            chunk_ids.push_back(chunk.header.id.to_string());
            chunk_sizes.push_back(chunk.header.size);
            
            auto data = chunk.reader->read_all();
            total_bytes += data.size();
            
            // Verify size matches
            CHECK(data.size() == chunk.header.size);
        });
        
        // Should have 20 chunks
        CHECK(chunk_ids.size() == 20);
        
        // Verify first few chunks
        CHECK(chunk_ids[0] == "CH01");
        CHECK(chunk_sizes[0] == 1);
        CHECK(chunk_ids[1] == "CH02");
        CHECK(chunk_sizes[1] == 2);
        CHECK(chunk_ids[19] == "CH20");
        CHECK(chunk_sizes[19] == 4);
        
        // Total bytes should match sum of sizes
        auto expected_total = std::accumulate(chunk_sizes.begin(), chunk_sizes.end(), 0ULL);
        CHECK(total_bytes == expected_total);
    }
}

TEST_CASE("edge cases - alternating odd/even sizes") {
    SUBCASE("alternating_sizes.iff") {
        auto is = load_test("alternating_sizes.iff");
        REQUIRE(is->good());
        
        std::vector<std::pair<std::string, std::uint64_t>> chunks;
        
        for_each_chunk(*is, [&](const auto& chunk) {
            chunks.push_back({chunk.header.id.to_string(), chunk.header.size});
            
            // Read all data to ensure padding is handled correctly
            auto data = chunk.reader->read_all();
            CHECK(data.size() == chunk.header.size);
            
            // After reading all, remaining should be 0
            CHECK(chunk.reader->remaining() == 0);
        });
        
        // Verify alternating sizes
        CHECK(chunks.size() == 8);
        
        // Check odd sizes
        CHECK(chunks[0].second == 1);   // ODD1
        CHECK(chunks[2].second == 3);   // ODD3
        CHECK(chunks[4].second == 11);  // OD11
        CHECK(chunks[6].second == 99);  // OD99
        
        // Check even sizes
        CHECK(chunks[1].second == 2);   // EVN2
        CHECK(chunks[3].second == 4);   // EVN4
        CHECK(chunks[5].second == 12);  // EV12
        CHECK(chunks[7].second == 100); // EV100
        
        // All odd sizes should require padding
        for (size_t i = 0; i < chunks.size(); i += 2) {
            CHECK((chunks[i].second & 1) == 1);  // Odd
        }
        
        // All even sizes should not require padding
        for (size_t i = 1; i < chunks.size(); i += 2) {
            CHECK((chunks[i].second & 1) == 0);  // Even
        }
    }
}

TEST_CASE("edge cases - complex PROP inheritance") {
    SUBCASE("complex_prop_list.iff") {
        auto is = load_test("complex_prop_list.iff");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        bool found_list = false;
        bool found_prop = false;
        int form_count = 0;
        std::vector<std::string> prop_chunks;
        std::vector<std::string> form_chunks;
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            
            if (chunk.header.id == "LIST"_4cc) {
                found_list = true;
                CHECK(chunk.header.is_container);
                if (chunk.header.type) {
                    CHECK(chunk.header.type->to_string() == "8SVX");
                }
            } else if (chunk.header.id == "PROP"_4cc) {
                found_prop = true;
                CHECK(chunk.header.is_container);
                if (chunk.header.type) {
                    CHECK(chunk.header.type->to_string() == "8SVX");
                }
            } else if (chunk.header.id == "FORM"_4cc) {
                form_count++;
                CHECK(chunk.header.is_container);
                if (chunk.header.type) {
                    CHECK(chunk.header.type->to_string() == "8SVX");
                }
            } else if (!chunk.header.is_container) {
                // Track which container owns this chunk
                if (chunk.current_container && *chunk.current_container == "PROP"_4cc) {
                    prop_chunks.push_back(chunk.header.id.to_string());
                } else if (chunk.current_form && *chunk.current_form == "8SVX"_4cc) {
                    form_chunks.push_back(chunk.header.id.to_string());
                }
            }
            
            it->next();
        }
        
        CHECK(found_list);
        CHECK(found_prop);
        CHECK(form_count == 3);  // Three FORMs in the LIST
        
        // PROP should contain VHDR and CHAN
        CHECK(prop_chunks.size() == 2);
        if (prop_chunks.size() >= 2) {
            CHECK(prop_chunks[0] == "VHDR");
            CHECK(prop_chunks[1] == "CHAN");
        }
        
        // Forms should have NAME and BODY chunks, one also has VHDR override
        CHECK(form_chunks.size() >= 6);  // At least NAME and BODY for each FORM
    }
}

TEST_CASE("edge cases - reader operations") {
    SUBCASE("test skip on various chunk sizes") {
        auto is = load_test("alternating_sizes.iff");
        REQUIRE(is->good());
        
        for_each_chunk(*is, [](const auto& chunk) {
            if (chunk.reader && chunk.header.size > 0) {
                auto initial_remaining = chunk.reader->remaining();
                
                // Skip half the chunk
                auto to_skip = chunk.header.size / 2;
                if (to_skip > 0) {
                    CHECK(chunk.reader->skip(to_skip));
                    CHECK(chunk.reader->offset() == to_skip);
                    CHECK(chunk.reader->remaining() == initial_remaining - to_skip);
                }
                
                // Try to skip beyond chunk boundary (should fail)
                CHECK_FALSE(chunk.reader->skip(chunk.header.size * 2));
                
                // Position should not change after failed skip
                CHECK(chunk.reader->offset() == to_skip);
            }
        });
    }
    
    SUBCASE("test read operations") {
        auto is = load_test("many_small_chunks.iff");
        REQUIRE(is->good());
        
        for_each_chunk(*is, [](const auto& chunk) {
            if (chunk.reader && chunk.header.size > 0) {
                // Test reading byte by byte
                std::vector<std::byte> byte_by_byte;
                while (chunk.reader->remaining() > 0) {
                    std::byte buffer[1];
                    auto read = chunk.reader->read(buffer, 1);
                    if (read == 1) {
                        byte_by_byte.push_back(buffer[0]);
                    } else {
                        break;
                    }
                }
                
                CHECK(byte_by_byte.size() == chunk.header.size);
                CHECK(chunk.reader->remaining() == 0);
                CHECK(chunk.reader->offset() == chunk.header.size);
            }
        });
    }
}

TEST_CASE("edge cases - context tracking") {
    SUBCASE("verify container context in nested structures") {
        auto is = load_test("complex_prop_list.iff");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            
            // Non-container chunks should have proper context
            if (!chunk.header.is_container) {
                // Should either be in PROP or in a FORM
                CHECK((chunk.current_container.has_value() || chunk.current_form.has_value()));
                
                if (chunk.header.id == "VHDR"_4cc || chunk.header.id == "CHAN"_4cc) {
                    // These can be in PROP or FORM
                    if (chunk.current_container == "PROP"_4cc) {
                        // In PROP, current_form should be empty
                        CHECK(!chunk.current_form.has_value());
                    } else {
                        // In FORM, current_form should be 8SVX
                        CHECK(chunk.current_form == "8SVX"_4cc);
                    }
                }
            }
            
            it->next();
        }
    }
}

TEST_CASE("edge cases - error conditions") {
    SUBCASE("reading beyond chunk boundaries") {
        auto is = load_test("zero_sized_chunks.iff");
        REQUIRE(is->good());
        
        for_each_chunk(*is, [](const auto& chunk) {
            if (chunk.reader) {
                // Try to read more than chunk size
                std::vector<std::byte> buffer(chunk.header.size + 100);
                auto read = chunk.reader->read(buffer.data(), buffer.size());
                
                // Should only read up to chunk size
                CHECK(read <= chunk.header.size);
                
                // After reading all, further reads should return 0
                auto more = chunk.reader->read(buffer.data(), 1);
                CHECK(more == 0);
            }
        });
    }
    
    SUBCASE("invalid operations on exhausted reader") {
        auto is = load_test("many_small_chunks.iff");
        REQUIRE(is->good());
        
        for_each_chunk(*is, [](const auto& chunk) {
            if (chunk.reader && chunk.header.size > 0) {
                // Exhaust the reader
                chunk.reader->read_all();
                
                CHECK(chunk.reader->remaining() == 0);
                CHECK(chunk.reader->offset() == chunk.header.size);
                
                // Further operations should handle gracefully
                std::byte buffer[10];
                CHECK(chunk.reader->read(buffer, 10) == 0);
                CHECK_FALSE(chunk.reader->skip(1));
                
                auto data = chunk.reader->read_all();
                CHECK(data.empty());
            }
        });
    }
}