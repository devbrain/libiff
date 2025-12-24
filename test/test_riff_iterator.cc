//
// Created by igor on 14/08/2025.
//
// Test RIFF iterator functionality

#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <algorithm>

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

TEST_CASE("RIFF iterator - minimal WAVE file") {
    auto is = load_test("minimal_wave.riff");
    REQUIRE(is->good());
    
    auto it = chunk_iterator::get_iterator(*is);
    REQUIRE(it != nullptr);
    
    std::vector<std::string> chunk_ids;
    std::vector<std::uint64_t> chunk_sizes;
    std::vector<int> depths;
    
    while (it->has_next()) {
        const auto& chunk = it->current();
        chunk_ids.push_back(chunk.header.id.to_string());
        chunk_sizes.push_back(chunk.header.size);
        depths.push_back(chunk.depth);
        
        // Check container types
        if (chunk.header.is_container && chunk.header.type) {
            CHECK((chunk.header.id == "RIFF"_4cc || chunk.header.id == "LIST"_4cc));
            if (chunk.header.id == "RIFF"_4cc) {
                CHECK(chunk.header.type->to_string() == "WAVE");
            }
        }
        
        it->next();
    }
    
    // Verify we parsed all chunks
    CHECK(chunk_ids.size() >= 3);  // RIFF, fmt, data
    
    // Check first chunk is RIFF container
    CHECK(chunk_ids[0] == "RIFF");
    CHECK(depths[0] == 0);
    
    // Check fmt chunk
    auto fmt_it = std::find(chunk_ids.begin(), chunk_ids.end(), "fmt ");
    CHECK(fmt_it != chunk_ids.end());
    if (fmt_it != chunk_ids.end()) {
        auto idx = std::distance(chunk_ids.begin(), fmt_it);
        CHECK(depths[idx] == 1);  // Inside RIFF
        CHECK(chunk_sizes[idx] == 16);  // Standard PCM format chunk size
    }
    
    // Check data chunk
    auto data_it = std::find(chunk_ids.begin(), chunk_ids.end(), "data");
    CHECK(data_it != chunk_ids.end());
}

TEST_CASE("RIFF iterator - chunk reader") {
    auto is = load_test("minimal_wave.riff");
    REQUIRE(is->good());
    
    bool found_fmt = false;
    bool found_data = false;
    
    for_each_chunk(*is, [&](const auto& chunk) {
        if (chunk.header.id == "fmt "_4cc) {
            found_fmt = true;
            CHECK(chunk.reader != nullptr);
            CHECK(chunk.reader->size() == 16);
            CHECK(chunk.reader->offset() == 0);
            CHECK(chunk.reader->remaining() == 16);
            
            // Read format chunk data
            std::vector<std::byte> fmt_data(16);
            auto bytes_read = chunk.reader->read(fmt_data.data(), 16);
            CHECK(bytes_read == 16);
            CHECK(chunk.reader->remaining() == 0);
        } else if (chunk.header.id == "data"_4cc) {
            found_data = true;
            CHECK(chunk.reader != nullptr);
            CHECK(chunk.reader->offset() == 0);
            
            // Skip some data
            if (chunk.reader->size() >= 100) {
                CHECK(chunk.reader->skip(100));
                CHECK(chunk.reader->offset() == 100);
            }
        }
    });
    
    CHECK(found_fmt);
    CHECK(found_data);
}

TEST_CASE("RIFF iterator - RF64 format") {
    SUBCASE("rf64_basic.rf64") {
        auto is = load_test("rf64_basic.rf64");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        bool found_rf64 = false;
        bool found_fmt = false;
        bool found_data = false;
        std::uint64_t data_size = 0;
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            
            if (chunk.header.id == "RF64"_4cc) {
                found_rf64 = true;
                CHECK(chunk.header.is_container);
                CHECK(chunk.header.type.has_value());
                if (chunk.header.type) {
                    CHECK(chunk.header.type->to_string() == "WAVE");
                }
                // RF64 container should have its size from ds64, not 0xFFFFFFFF
                CHECK(chunk.header.size != 0xFFFFFFFF);
            } else if (chunk.header.id == "ds64"_4cc) {
                // ds64 should be hidden from iteration (it's internal to RF64 handling)
                CHECK(false);  // Should not see ds64 in iteration
            } else if (chunk.header.id == "fmt "_4cc) {
                found_fmt = true;
                CHECK(!chunk.header.is_container);
                CHECK(chunk.header.size == 16);  // Standard PCM format
            } else if (chunk.header.id == "data"_4cc) {
                found_data = true;
                data_size = chunk.header.size;
                // Data chunk should have its actual size from ds64, not 0xFFFFFFFF
                CHECK(chunk.header.size != 0xFFFFFFFF);
                CHECK(chunk.header.size == 1000);  // Expected size from ds64
            }
            
            it->next();
        }
        
        CHECK(found_rf64);
        CHECK(found_fmt);
        CHECK(found_data);
        CHECK(data_size == 1000);  // Verify ds64 override was applied
    }
    
    SUBCASE("rf64_with_table.rf64") {
        auto is = load_test("rf64_with_table.rf64");
        REQUIRE(is->good());
        
        auto it = chunk_iterator::get_iterator(*is);
        REQUIRE(it != nullptr);
        
        // This file has a ds64 with an override table
        bool found_rf64 = false;
        bool found_data = false;
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            
            if (chunk.header.id == "RF64"_4cc) {
                found_rf64 = true;
                // Should have proper size from ds64
                CHECK(chunk.header.size != 0xFFFFFFFF);
            } else if (chunk.header.id == "data"_4cc) {
                found_data = true;
                // Should have proper size from ds64 table
                CHECK(chunk.header.size != 0xFFFFFFFF);
            }
            
            it->next();
        }
        
        CHECK(found_rf64);
        CHECK(found_data);
    }
}

TEST_CASE("RIFF iterator - LIST chunks") {
    // Create a simple RIFF file with LIST chunk for testing
    std::vector<std::byte> riff_data = {
        // RIFF header
        std::byte('R'), std::byte('I'), std::byte('F'), std::byte('F'),
        std::byte(60), std::byte(0), std::byte(0), std::byte(0),  // Size (little-endian)
        std::byte('W'), std::byte('A'), std::byte('V'), std::byte('E'),
        
        // fmt chunk
        std::byte('f'), std::byte('m'), std::byte('t'), std::byte(' '),
        std::byte(16), std::byte(0), std::byte(0), std::byte(0),  // Size 16
        // PCM format data (16 bytes)
        std::byte(1), std::byte(0),  // Format: PCM
        std::byte(2), std::byte(0),  // Channels: 2
        std::byte(0x44), std::byte(0xAC), std::byte(0), std::byte(0),  // Sample rate: 44100
        std::byte(0x10), std::byte(0xB1), std::byte(2), std::byte(0),  // Byte rate
        std::byte(4), std::byte(0),  // Block align
        std::byte(16), std::byte(0), // Bits per sample
        
        // LIST chunk
        std::byte('L'), std::byte('I'), std::byte('S'), std::byte('T'),
        std::byte(20), std::byte(0), std::byte(0), std::byte(0),  // Size
        std::byte('I'), std::byte('N'), std::byte('F'), std::byte('O'),  // Type
        
        // Info chunks inside LIST
        std::byte('I'), std::byte('S'), std::byte('F'), std::byte('T'),
        std::byte(8), std::byte(0), std::byte(0), std::byte(0),  // Size
        std::byte('L'), std::byte('a'), std::byte('v'), std::byte('f'),
        std::byte('5'), std::byte('8'), std::byte('.'), std::byte('0')
    };
    
    // Write to temp file
    std::filesystem::path temp_file = std::filesystem::temp_directory_path() / "test_riff_list.riff";
    {
        std::ofstream out(temp_file, std::ios::binary);
        out.write(reinterpret_cast<const char*>(riff_data.data()), riff_data.size());
    }
    
    // Parse the file
    std::ifstream is(temp_file, std::ios::binary);
    REQUIRE(is.good());
    
    auto it = chunk_iterator::get_iterator(is);
    REQUIRE(it != nullptr);
    
    bool found_list = false;
    bool found_isft = false;
    
    while (it->has_next()) {
        const auto& chunk = it->current();
        
        if (chunk.header.id == "LIST"_4cc) {
            found_list = true;
            CHECK(chunk.header.is_container);
            CHECK(chunk.header.type.has_value());
            if (chunk.header.type) {
                CHECK(chunk.header.type->to_string() == "INFO");
            }
            CHECK(chunk.current_container == "LIST"_4cc);
        } else if (chunk.header.id == "ISFT"_4cc) {
            found_isft = true;
            CHECK(!chunk.header.is_container);
            CHECK(chunk.depth == 2);  // Inside LIST which is inside RIFF
            CHECK(chunk.current_container == "LIST"_4cc);
        }
        
        it->next();
    }
    
    CHECK(found_list);
    CHECK(found_isft);
    
    // Clean up temp file
    std::filesystem::remove(temp_file);
}

TEST_CASE("RIFF iterator - padding handling") {
    // Create RIFF with odd-sized chunks to test padding
    std::vector<std::byte> riff_data = {
        // RIFF header
        std::byte('R'), std::byte('I'), std::byte('F'), std::byte('F'),
        std::byte(26), std::byte(0), std::byte(0), std::byte(0),  // Size
        std::byte('T'), std::byte('E'), std::byte('S'), std::byte('T'),
        
        // Odd-sized chunk (3 bytes)
        std::byte('O'), std::byte('D'), std::byte('D'), std::byte('1'),
        std::byte(3), std::byte(0), std::byte(0), std::byte(0),  // Size
        std::byte('A'), std::byte('B'), std::byte('C'),  // Data
        std::byte(0),  // Padding
        
        // Even-sized chunk (4 bytes)
        std::byte('E'), std::byte('V'), std::byte('E'), std::byte('N'),
        std::byte(4), std::byte(0), std::byte(0), std::byte(0),  // Size
        std::byte('D'), std::byte('E'), std::byte('F'), std::byte('G')  // Data
    };
    
    std::filesystem::path temp_file = std::filesystem::temp_directory_path() / "test_riff_padding.riff";
    {
        std::ofstream out(temp_file, std::ios::binary);
        out.write(reinterpret_cast<const char*>(riff_data.data()), riff_data.size());
    }
    
    std::ifstream is(temp_file, std::ios::binary);
    REQUIRE(is.good());
    
    std::vector<std::pair<std::string, std::uint64_t>> chunks;
    
    for_each_chunk(is, [&](const auto& chunk) {
        if (!chunk.header.is_container) {
            chunks.push_back({chunk.header.id.to_string(), chunk.header.size});
            
            // Read all data to ensure padding is handled
            auto data = chunk.reader->read_all();
            CHECK(data.size() == chunk.header.size);
        }
    });
    
    CHECK(chunks.size() == 2);
    CHECK(chunks[0].first == "ODD1");
    CHECK(chunks[0].second == 3);
    CHECK(chunks[1].first == "EVEN");
    CHECK(chunks[1].second == 4);
    
    std::filesystem::remove(temp_file);
}

TEST_CASE("RIFF iterator - RIFX big-endian format") {
    // Create a simple RIFX file (big-endian RIFF)
    std::vector<std::byte> rifx_data = {
        // RIFX header
        std::byte('R'), std::byte('I'), std::byte('F'), std::byte('X'),
        std::byte(0), std::byte(0), std::byte(0), std::byte(32),  // Size 32 (big-endian)
        std::byte('T'), std::byte('E'), std::byte('S'), std::byte('T'),
        
        // TEST chunk with big-endian size
        std::byte('D'), std::byte('A'), std::byte('T'), std::byte('A'),
        std::byte(0), std::byte(0), std::byte(0), std::byte(4),  // Size 4 (big-endian)
        std::byte(0x12), std::byte(0x34), std::byte(0x56), std::byte(0x78),
        
        // PAD chunk with big-endian size
        std::byte('P'), std::byte('A'), std::byte('D'), std::byte(' '),
        std::byte(0), std::byte(0), std::byte(0), std::byte(8),  // Size 8 (big-endian)
        std::byte('A'), std::byte('B'), std::byte('C'), std::byte('D'),
        std::byte('E'), std::byte('F'), std::byte('G'), std::byte('H')
    };
    
    std::filesystem::path temp_file = std::filesystem::temp_directory_path() / "test_rifx.rifx";
    {
        std::ofstream out(temp_file, std::ios::binary);
        out.write(reinterpret_cast<const char*>(rifx_data.data()), rifx_data.size());
    }
    
    std::ifstream is(temp_file, std::ios::binary);
    REQUIRE(is.good());
    
    auto it = chunk_iterator::get_iterator(is);
    REQUIRE(it != nullptr);
    
    std::vector<std::pair<std::string, std::uint64_t>> chunks;
    
    while (it->has_next()) {
        const auto& chunk = it->current();
        
        if (chunk.header.id == "RIFX"_4cc) {
            CHECK(chunk.header.is_container);
            CHECK(chunk.header.size == 32);  // Should be correctly read as big-endian
            CHECK(chunk.header.type->to_string() == "TEST");
        } else if (!chunk.header.is_container) {
            chunks.push_back({chunk.header.id.to_string(), chunk.header.size});
        }
        
        it->next();
    }
    
    CHECK(chunks.size() == 2);
    CHECK(chunks[0].first == "DATA");
    CHECK(chunks[0].second == 4);  // Should be 4, not 0x04000000
    CHECK(chunks[1].first == "PAD ");
    CHECK(chunks[1].second == 8);  // Should be 8, not 0x08000000
    
    std::filesystem::remove(temp_file);
}

TEST_CASE("RIFF iterator - error handling") {
    SUBCASE("invalid format detection") {
        // Create a file that doesn't start with RIFF
        std::vector<std::byte> bad_data = {
            std::byte('B'), std::byte('A'), std::byte('D'), std::byte('!'),
            std::byte(0), std::byte(0), std::byte(0), std::byte(0)
        };
        
        std::filesystem::path temp_file = std::filesystem::temp_directory_path() / "test_bad.dat";
        {
            std::ofstream out(temp_file, std::ios::binary);
            out.write(reinterpret_cast<const char*>(bad_data.data()), bad_data.size());
        }
        
        std::ifstream is(temp_file, std::ios::binary);
        CHECK_THROWS_AS(chunk_iterator::get_iterator(is), parse_error);
        
        std::filesystem::remove(temp_file);
    }
    
    SUBCASE("truncated RIFF file") {
        // Create a truncated RIFF file
        std::vector<std::byte> truncated = {
            std::byte('R'), std::byte('I'), std::byte('F'), std::byte('F'),
            std::byte(100), std::byte(0), std::byte(0), std::byte(0),  // Claims 100 bytes
            std::byte('W'), std::byte('A'), std::byte('V'), std::byte('E')
            // Missing data...
        };
        
        std::filesystem::path temp_file = std::filesystem::temp_directory_path() / "test_truncated.riff";
        {
            std::ofstream out(temp_file, std::ios::binary);
            out.write(reinterpret_cast<const char*>(truncated.data()), truncated.size());
        }
        
        std::ifstream is(temp_file, std::ios::binary);
        auto it = chunk_iterator::get_iterator(is);
        REQUIRE(it != nullptr);
        
        // Should be able to read RIFF header
        CHECK(it->has_next());
        CHECK(it->current().header.id == "RIFF"_4cc);
        
        // But advancing should eventually fail or end
        it->next();
        CHECK(!it->has_next());  // No more chunks due to truncation
        
        std::filesystem::remove(temp_file);
    }
}