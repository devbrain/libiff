//
// Created by igor on 14/08/2025.
//
// Security hardening tests for IFF/RIFF parser

#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <iff/parser.hh>
#include <iff/chunk_iterator.hh>
#include <iff/exceptions.hh>
#include <iff/parse_options.hh>
#include <iff/byte_order.hh>

using namespace iff;
using namespace std::string_literals;

TEST_CASE("Security - max chunk size enforcement") {
    SUBCASE("chunk exceeding max size in strict mode") {
        // Create RIFF with huge chunk size
        std::vector<std::byte> data = {
            // RIFF header
            std::byte('R'), std::byte('I'), std::byte('F'), std::byte('F'),
            std::byte(0xFF), std::byte(0xFF), std::byte(0xFF), std::byte(0x7F),  // 2GB size
            std::byte('T'), std::byte('E'), std::byte('S'), std::byte('T'),
            
            // Huge chunk
            std::byte('D'), std::byte('A'), std::byte('T'), std::byte('A'),
            std::byte(0xFF), std::byte(0xFF), std::byte(0xFF), std::byte(0x7F),  // 2GB size
            std::byte(0), std::byte(0), std::byte(0), std::byte(0)  // Some data
        };
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        parse_options opts;
        opts.strict = true;
        opts.max_chunk_size = 1024 * 1024;  // 1MB limit
        
        // Parser correctly refuses to process files with huge chunks
        // It may throw during construction or during iteration
        try {
            auto it = chunk_iterator::get_iterator(stream, opts);
            while (it && it->has_next()) {
                it->next();
            }
            CHECK(false);  // Should have thrown by now
        } catch (const std::exception&) {
            CHECK(true);  // Expected - parser refuses malformed data
        }
    }
    
    SUBCASE("chunk exceeding max size in lenient mode") {
        // Create RIFF with large chunk
        std::vector<std::byte> data = {
            // RIFF header
            std::byte('R'), std::byte('I'), std::byte('F'), std::byte('F'),
            std::byte(100), std::byte(0), std::byte(0), std::byte(0),  // Size 100
            std::byte('T'), std::byte('E'), std::byte('S'), std::byte('T'),
            
            // Large chunk (claims 10MB but only has 4 bytes)
            std::byte('D'), std::byte('A'), std::byte('T'), std::byte('A'),
            std::byte(0), std::byte(0x96), std::byte(0x98), std::byte(0),  // 10MB size
            std::byte(0), std::byte(0), std::byte(0), std::byte(0)  // Some data
        };
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        parse_options opts;
        opts.strict = false;
        opts.max_chunk_size = 1024;  // 1KB limit
        
        bool warning_called = false;
        opts.on_warning = [&warning_called](std::uint64_t, std::string_view category, std::string_view) {
            if (category == "size_limit") {
                warning_called = true;
            }
        };
        
        auto it = chunk_iterator::get_iterator(stream, opts);
        REQUIRE(it != nullptr);
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            if (chunk.header.id == "DATA"_4cc) {
                // Size should be clamped to max_chunk_size
                CHECK(chunk.header.size == 1024);
            }
            it->next();
        }
        
        CHECK(warning_called);
    }
}

TEST_CASE("Security - max depth enforcement") {
    SUBCASE("deeply nested containers in strict mode") {
        // Create deeply nested IFF
        std::stringstream stream;
        
        // Write nested FORMs
        int depth = 10;
        for (int i = 0; i < depth; i++) {
            stream.write("FORM", 4);
            // Write big-endian size (100 bytes)
            stream.put(0); stream.put(0); stream.put(0); stream.put(100);
            stream.write("TEST", 4);
        }
        
        // Add a data chunk at the deepest level
        stream.write("DATA", 4);
        // Write big-endian size (4 bytes)
        stream.put(0); stream.put(0); stream.put(0); stream.put(4);
        stream.write("ABCD", 4);
        
        stream.seekg(0);
        
        parse_options opts;
        opts.strict = true;
        opts.max_depth = 5;  // Limit to 5 levels
        
        auto it = chunk_iterator::get_iterator(stream, opts);
        REQUIRE(it != nullptr);
        
        // Should throw when trying to go deeper than max_depth
        CHECK_THROWS_AS([&]() {
            while (it->has_next()) {
                it->next();
            }
        }(), parse_error);
    }
    
    SUBCASE("deeply nested containers in lenient mode") {
        // Create nested RIFFs
        std::stringstream stream;
        
        // Build from inside out
        std::string inner = "DATA";
        inner += std::string(4, '\0');  // Size 0 (little-endian)
        
        for (int i = 0; i < 3; i++) {
            std::string container = "LIST";
            std::uint32_t size = inner.size() + 4;  // +4 for type tag
            container += std::string(reinterpret_cast<char*>(&size), 4);
            container += "TEST";
            container += inner;
            inner = container;
        }
        
        // Wrap in RIFF
        stream.write("RIFF", 4);
        std::uint32_t total_size = inner.size() + 4;
        stream.write(reinterpret_cast<char*>(&total_size), 4);
        stream.write("TEST", 4);
        stream.write(inner.c_str(), inner.size());
        
        stream.seekg(0);
        
        parse_options opts;
        opts.strict = false;
        opts.max_depth = 2;  // Very shallow limit
        
        bool depth_warning = false;
        opts.on_warning = [&depth_warning](std::uint64_t, std::string_view category, std::string_view) {
            if (category == "depth_limit") {
                depth_warning = true;
            }
        };
        
        auto it = chunk_iterator::get_iterator(stream, opts);
        REQUIRE(it != nullptr);
        
        int chunks_found = 0;
        while (it->has_next()) {
            chunks_found++;
            it->next();
        }
        
        CHECK(depth_warning);  // Should have warned about depth
        CHECK(chunks_found > 0);  // Should still process some chunks
    }
}

TEST_CASE("Security - malformed chunk headers") {
    SUBCASE("chunk size extending beyond container") {
        // RIFF container claims 20 bytes but chunk inside claims 100
        std::vector<std::byte> data = {
            // RIFF header
            std::byte('R'), std::byte('I'), std::byte('F'), std::byte('F'),
            std::byte(20), std::byte(0), std::byte(0), std::byte(0),  // Container size 20
            std::byte('T'), std::byte('E'), std::byte('S'), std::byte('T'),
            
            // Chunk claiming size beyond container
            std::byte('D'), std::byte('A'), std::byte('T'), std::byte('A'),
            std::byte(100), std::byte(0), std::byte(0), std::byte(0),  // Claims 100 bytes
            std::byte('X'), std::byte('Y'), std::byte('Z'), std::byte('W')  // Only 4 bytes
        };
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        auto it = chunk_iterator::get_iterator(stream);
        REQUIRE(it != nullptr);
        
        // Should handle gracefully - chunk reader should not read beyond container
        while (it->has_next()) {
            const auto& chunk = it->current();
            if (chunk.reader && chunk.header.id == "DATA"_4cc) {
                // Try to read all data - should be limited by container bounds
                auto data = chunk.reader->read_all();
                CHECK(data.size() <= 8);  // Only 8 bytes available in container
            }
            it->next();
        }
    }
    
    SUBCASE("truncated chunk header") {
        // File ends in middle of chunk header
        std::vector<std::byte> data = {
            std::byte('F'), std::byte('O'), std::byte('R'), std::byte('M'),
            std::byte(0), std::byte(0), std::byte(0), std::byte(16),  // Big-endian 16
            std::byte('T'), std::byte('E'), std::byte('S'), std::byte('T'),
            std::byte('D'), std::byte('A')  // Incomplete chunk ID
        };
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        auto it = chunk_iterator::get_iterator(stream);
        REQUIRE(it != nullptr);
        
        // Should handle EOF gracefully
        int chunks = 0;
        while (it->has_next()) {
            chunks++;
            it->next();
        }
        CHECK(chunks == 1);  // Only FORM should be found
    }
}

TEST_CASE("Security - RF64 with corrupt ds64") {
    SUBCASE("ds64 chunk too small") {
        std::vector<std::byte> data = {
            // RF64 header
            std::byte('R'), std::byte('F'), std::byte('6'), std::byte('4'),
            std::byte(0xFF), std::byte(0xFF), std::byte(0xFF), std::byte(0xFF),  // Size marker
            std::byte('W'), std::byte('A'), std::byte('V'), std::byte('E'),
            
            // ds64 chunk - too small (needs 28 bytes minimum)
            std::byte('d'), std::byte('s'), std::byte('6'), std::byte('4'),
            std::byte(16), std::byte(0), std::byte(0), std::byte(0),  // Only 16 bytes
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),
            std::byte(0), std::byte(0), std::byte(0), std::byte(0)
        };
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        // Should throw when trying to parse the invalid ds64
        auto it = chunk_iterator::get_iterator(stream);
        CHECK_THROWS_AS([&]() {
            while (it && it->has_next()) {
                it->next();
            }
        }(), parse_error);
    }
    
    SUBCASE("ds64 with invalid table count") {
        std::vector<std::byte> data = {
            // RF64 header
            std::byte('R'), std::byte('F'), std::byte('6'), std::byte('4'),
            std::byte(0xFF), std::byte(0xFF), std::byte(0xFF), std::byte(0xFF),
            std::byte('W'), std::byte('A'), std::byte('V'), std::byte('E'),
            
            // ds64 chunk with huge table count
            std::byte('d'), std::byte('s'), std::byte('6'), std::byte('4'),
            std::byte(32), std::byte(0), std::byte(0), std::byte(0),  // Size 32
            // Fixed fields (28 bytes)
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),  // RIFF size
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),  // data size
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),  // sample count
            // Table count claiming 1000 entries (impossible with size 32)
            std::byte(0xE8), std::byte(0x03), std::byte(0), std::byte(0)
        };
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        // Should throw when trying to parse the invalid ds64
        auto it = chunk_iterator::get_iterator(stream);
        CHECK_THROWS_AS([&]() {
            while (it && it->has_next()) {
                it->next();
            }
        }(), parse_error);
    }
}

TEST_CASE("Security - memory exhaustion protection") {
    SUBCASE("chunk reader with huge size") {
        // Create a chunk that claims huge size but has little data
        std::vector<std::byte> data = {
            std::byte('F'), std::byte('O'), std::byte('R'), std::byte('M'),
            std::byte(0), std::byte(0), std::byte(0), std::byte(100),  // Size 100
            std::byte('T'), std::byte('E'), std::byte('S'), std::byte('T'),
            
            std::byte('D'), std::byte('A'), std::byte('T'), std::byte('A'),
            std::byte(0x3F), std::byte(0xFF), std::byte(0xFF), std::byte(0xFF),  // ~1GB
            std::byte('A'), std::byte('B'), std::byte('C'), std::byte('D')
        };
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        parse_options opts;
        opts.strict = false;
        opts.max_chunk_size = 1024 * 1024;  // 1MB limit
        
        auto it = chunk_iterator::get_iterator(stream, opts);
        REQUIRE(it != nullptr);
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            if (chunk.reader && chunk.header.id == "DATA"_4cc) {
                // read_all() should not allocate gigabytes
                auto all_data = chunk.reader->read_all();
                CHECK(all_data.size() <= opts.max_chunk_size);
            }
            it->next();
        }
    }
}