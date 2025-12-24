//
// Test BW64 (Broadcast Wave 64-bit) format support
//

#include <doctest/doctest.h>
#include <iff/chunk_iterator.hh>
#include <iff/fourcc.hh>
#include <iff/exceptions.hh>
#include <sstream>
#include <vector>
#include <cstring>

using namespace iff;
using namespace std::string_literals;

// Helper to create a minimal BW64 file
std::vector<std::byte> create_bw64_file(bool with_ds64 = true, bool large_size = false) {
    std::vector<std::byte> data;
    
    // BW64 header
    data.push_back(std::byte('B'));
    data.push_back(std::byte('W'));
    data.push_back(std::byte('6'));
    data.push_back(std::byte('4'));
    
    // Size (little-endian)
    if (large_size) {
        // 0xFFFFFFFF marker for 64-bit size
        data.push_back(std::byte(0xFF));
        data.push_back(std::byte(0xFF));
        data.push_back(std::byte(0xFF));
        data.push_back(std::byte(0xFF));
    } else {
        // Small size (100 bytes)
        data.push_back(std::byte(100));
        data.push_back(std::byte(0));
        data.push_back(std::byte(0));
        data.push_back(std::byte(0));
    }
    
    // Type (WAVE for audio)
    data.push_back(std::byte('W'));
    data.push_back(std::byte('A'));
    data.push_back(std::byte('V'));
    data.push_back(std::byte('E'));
    
    if (with_ds64) {
        // ds64 chunk
        data.push_back(std::byte('d'));
        data.push_back(std::byte('s'));
        data.push_back(std::byte('6'));
        data.push_back(std::byte('4'));
        
        // ds64 size (24 bytes minimum)
        data.push_back(std::byte(24));
        data.push_back(std::byte(0));
        data.push_back(std::byte(0));
        data.push_back(std::byte(0));
        
        // RIFF size (64-bit, little-endian) - 72 bytes (actual content after size field)
        // 4 (WAVE) + 32 (ds64) + 24 (fmt) + 12 (data) = 72
        data.push_back(std::byte(72));
        data.push_back(std::byte(0));
        data.push_back(std::byte(0));
        data.push_back(std::byte(0));
        data.push_back(std::byte(0));
        data.push_back(std::byte(0));
        data.push_back(std::byte(0));
        data.push_back(std::byte(0));
        
        // data size (64-bit) - 0 for this test
        for (int i = 0; i < 8; i++) {
            data.push_back(std::byte(0));
        }
        
        // sample count (64-bit) - 0 for this test
        for (int i = 0; i < 8; i++) {
            data.push_back(std::byte(0));
        }
    }
    
    // fmt chunk
    data.push_back(std::byte('f'));
    data.push_back(std::byte('m'));
    data.push_back(std::byte('t'));
    data.push_back(std::byte(' '));
    
    // fmt size (16 bytes)
    data.push_back(std::byte(16));
    data.push_back(std::byte(0));
    data.push_back(std::byte(0));
    data.push_back(std::byte(0));
    
    // Format data (PCM, mono, 44100 Hz, 16-bit)
    data.push_back(std::byte(1));   // Format tag (PCM)
    data.push_back(std::byte(0));
    data.push_back(std::byte(1));   // Channels
    data.push_back(std::byte(0));
    data.push_back(std::byte(0x44)); // Sample rate (44100)
    data.push_back(std::byte(0xAC));
    data.push_back(std::byte(0));
    data.push_back(std::byte(0));
    data.push_back(std::byte(0x88)); // Byte rate
    data.push_back(std::byte(0x58));
    data.push_back(std::byte(0x01));
    data.push_back(std::byte(0));
    data.push_back(std::byte(2));   // Block align
    data.push_back(std::byte(0));
    data.push_back(std::byte(16));  // Bits per sample
    data.push_back(std::byte(0));
    
    // data chunk
    data.push_back(std::byte('d'));
    data.push_back(std::byte('a'));
    data.push_back(std::byte('t'));
    data.push_back(std::byte('a'));
    
    // data size (4 bytes)
    data.push_back(std::byte(4));
    data.push_back(std::byte(0));
    data.push_back(std::byte(0));
    data.push_back(std::byte(0));
    
    // Sample data
    data.push_back(std::byte(0));
    data.push_back(std::byte(0));
    data.push_back(std::byte(0));
    data.push_back(std::byte(0));
    
    return data;
}

TEST_CASE("BW64 format detection") {
    SUBCASE("BW64 file is recognized") {
        auto data = create_bw64_file(false);
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        auto it = chunk_iterator::get_iterator(stream);
        REQUIRE(it != nullptr);
        
        // Should successfully create iterator for BW64
        CHECK(it->has_next());
        const auto& chunk = it->current();
        CHECK(chunk.header.id == "BW64"_4cc);
        CHECK(chunk.header.is_container);
        CHECK(chunk.header.type);
        CHECK(*chunk.header.type == "WAVE"_4cc);
    }
    
    SUBCASE("BW64 with ds64") {
        auto data = create_bw64_file(true, true);
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        auto it = chunk_iterator::get_iterator(stream);
        REQUIRE(it != nullptr);
        
        std::vector<std::string> chunk_ids;
        while (it->has_next()) {
            const auto& chunk = it->current();
            chunk_ids.push_back(chunk.header.id.to_string());
            it->next();
        }
        
        // ds64 should be hidden from iteration
        REQUIRE(chunk_ids.size() == 3);
        CHECK(chunk_ids[0] == "BW64");
        CHECK(chunk_ids[1] == "fmt ");
        CHECK(chunk_ids[2] == "data");
        
        // ds64 should NOT appear in the list
        for (const auto& id : chunk_ids) {
            CHECK(id != "ds64");
        }
    }
}

TEST_CASE("BW64 size handling") {
    SUBCASE("BW64 with 64-bit size") {
        auto data = create_bw64_file(true, true);
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        auto it = chunk_iterator::get_iterator(stream);
        REQUIRE(it != nullptr);
        
        const auto& chunk = it->current();
        CHECK(chunk.header.id == "BW64"_4cc);
        // Size should be from ds64, not the 0xFFFFFFFF marker
        CHECK(chunk.header.size == 72);
    }
    
    SUBCASE("BW64 without ds64") {
        auto data = create_bw64_file(false, false);
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        auto it = chunk_iterator::get_iterator(stream);
        REQUIRE(it != nullptr);
        
        const auto& chunk = it->current();
        CHECK(chunk.header.id == "BW64"_4cc);
        CHECK(chunk.header.size == 100);
    }
}

TEST_CASE("BW64 chunk iteration") {
    SUBCASE("Iterate through BW64 chunks") {
        auto data = create_bw64_file(true);
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        auto it = chunk_iterator::get_iterator(stream);
        REQUIRE(it != nullptr);
        
        // First chunk: BW64 container
        CHECK(it->has_next());
        const auto& chunk1 = it->current();
        CHECK(chunk1.header.id == "BW64"_4cc);
        CHECK(chunk1.header.is_container);
        CHECK(chunk1.current_form == "WAVE"_4cc);
        
        // Second chunk: fmt
        it->next();
        CHECK(it->has_next());
        const auto& chunk2 = it->current();
        CHECK(chunk2.header.id == "fmt "_4cc);
        CHECK(!chunk2.header.is_container);
        CHECK(chunk2.header.size == 16);
        CHECK(chunk2.current_form == "WAVE"_4cc);
        
        // Third chunk: data
        it->next();
        CHECK(it->has_next());
        const auto& chunk3 = it->current();
        CHECK(chunk3.header.id == "data"_4cc);
        CHECK(!chunk3.header.is_container);
        CHECK(chunk3.header.size == 4);
        CHECK(chunk3.current_form == "WAVE"_4cc);
        
        // No more chunks
        it->next();
        CHECK(!it->has_next());
    }
}

TEST_CASE("BW64 compatibility") {
    SUBCASE("BW64 behaves like RF64") {
        // BW64 should use the same ds64 mechanism as RF64
        auto bw64_data = create_bw64_file(true, true);
        
        // Create equivalent RF64 file by changing magic
        auto rf64_data = bw64_data;
        rf64_data[0] = std::byte('R');
        rf64_data[1] = std::byte('F');
        rf64_data[2] = std::byte('6');
        rf64_data[3] = std::byte('4');
        
        std::istringstream bw64_stream(std::string(reinterpret_cast<char*>(bw64_data.data()), bw64_data.size()));
        std::istringstream rf64_stream(std::string(reinterpret_cast<char*>(rf64_data.data()), rf64_data.size()));
        
        auto bw64_it = chunk_iterator::get_iterator(bw64_stream);
        auto rf64_it = chunk_iterator::get_iterator(rf64_stream);
        
        REQUIRE(bw64_it != nullptr);
        REQUIRE(rf64_it != nullptr);
        
        // Both should iterate the same chunks (except root ID)
        std::vector<std::string> bw64_chunks, rf64_chunks;
        
        while (bw64_it->has_next()) {
            if (bw64_it->current().header.id != "BW64"_4cc) {
                bw64_chunks.push_back(bw64_it->current().header.id.to_string());
            }
            bw64_it->next();
        }
        
        while (rf64_it->has_next()) {
            if (rf64_it->current().header.id != "RF64"_4cc) {
                rf64_chunks.push_back(rf64_it->current().header.id.to_string());
            }
            rf64_it->next();
        }
        
        CHECK(bw64_chunks == rf64_chunks);
    }
}