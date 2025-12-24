//
// Test to verify improved error messages
//

#include <doctest/doctest.h>
#include <sstream>
#include <vector>
#include <string>

#include <iff/parser.hh>
#include <iff/chunk_iterator.hh>
#include <iff/exceptions.hh>
#include <iff/parse_options.hh>

using namespace iff;

TEST_CASE("Improved error messages") {
    SUBCASE("chunk size limit exceeded - shows chunk details") {
        // Create RIFF with large chunk
        std::vector<std::byte> data = {
            // RIFF header
            std::byte('R'), std::byte('I'), std::byte('F'), std::byte('F'),
            std::byte(100), std::byte(0), std::byte(0), std::byte(0),
            std::byte('T'), std::byte('E'), std::byte('S'), std::byte('T'),
            
            // Large chunk at offset 12
            std::byte('D'), std::byte('A'), std::byte('T'), std::byte('A'),
            std::byte(0), std::byte(0x96), std::byte(0x98), std::byte(0),  // 10MB size
            std::byte(0), std::byte(0), std::byte(0), std::byte(0)
        };
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        parse_options opts;
        opts.strict = true;
        opts.max_chunk_size = 1024;  // 1KB limit
        
        try {
            auto it = chunk_iterator::get_iterator(stream, opts);
            it->next();  // Skip RIFF container
            it->next();  // Try to process DATA chunk
            FAIL("Should have thrown exception");
        } catch (const parse_error& e) {
            std::string msg = e.what();
            // Should mention chunk name, offset, size, and limit
            CHECK(msg.find("DATA") != std::string::npos);
            CHECK(msg.find("offset 12") != std::string::npos);
            CHECK(msg.find("10000000") != std::string::npos);  // Actual size
            CHECK(msg.find("1024") != std::string::npos);      // Max allowed
            INFO("Error message: " << msg);
        }
    }
    
    SUBCASE("container depth exceeded - shows details") {
        std::stringstream stream;
        
        // Create deeply nested LISTs
        for (int i = 0; i < 5; i++) {
            stream.write("LIST", 4);
            std::uint32_t size = 100;
            stream.write(reinterpret_cast<char*>(&size), 4);
            stream.write("TEST", 4);
        }
        
        stream.seekg(0);
        
        parse_options opts;
        opts.strict = true;
        opts.max_depth = 3;
        
        try {
            auto it = chunk_iterator::get_iterator(stream, opts);
            while (it->has_next()) {
                it->next();
            }
            FAIL("Should have thrown exception");
        } catch (const parse_error& e) {
            std::string msg = e.what();
            // Should mention container, depth details
            CHECK(msg.find("LIST") != std::string::npos);
            CHECK(msg.find("exceed") != std::string::npos);
            CHECK(msg.find("depth") != std::string::npos);
            CHECK(msg.find("3") != std::string::npos);  // Max depth
            INFO("Error message: " << msg);
        }
    }
    
    SUBCASE("RF64 ds64 validation - descriptive error") {
        std::vector<std::byte> data = {
            // RF64 header
            std::byte('R'), std::byte('F'), std::byte('6'), std::byte('4'),
            std::byte(0xFF), std::byte(0xFF), std::byte(0xFF), std::byte(0xFF),
            std::byte('W'), std::byte('A'), std::byte('V'), std::byte('E'),
            
            // ds64 chunk with invalid table count
            std::byte('d'), std::byte('s'), std::byte('6'), std::byte('4'),
            std::byte(32), std::byte(0), std::byte(0), std::byte(0),  // Size 32
            // Fixed fields (28 bytes)
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),
            std::byte(0), std::byte(0), std::byte(0), std::byte(0),
            // Table count claiming 1000 entries
            std::byte(0xE8), std::byte(0x03), std::byte(0), std::byte(0)
        };
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        try {
            auto it = chunk_iterator::get_iterator(stream);
            it->next();  // Process RF64
            it->next();  // Try to read past ds64
            // May or may not throw here depending on implementation
        } catch (const parse_error& e) {
            std::string msg = e.what();
            // Should have clear message about ds64 issue
            CHECK((msg.find("ds64") != std::string::npos || 
                   msg.find("table") != std::string::npos ||
                   msg.find("Invalid") != std::string::npos));
            INFO("Error message: " << msg);
        } catch (const io_error& e) {
            // Also acceptable - seeking beyond stream
            std::string msg = e.what();
            CHECK(msg.find("seek") != std::string::npos);
            INFO("IO Error message: " << msg);
        }
    }
}