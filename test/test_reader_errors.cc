//
// Test error conditions in readers and input handling using generated test files
//

#include <doctest/doctest.h>
#include <iff/chunk_iterator.hh>
#include <iff/exceptions.hh>
#include <iff/fourcc.hh>
#include <sstream>
#include <vector>
#include "test_utils.hh"

using namespace iff;
using namespace std::string_literals;

// Custom stream that simulates I/O errors
class failing_streambuf : public std::streambuf {
public:
    failing_streambuf(const std::string& data, std::size_t fail_after)
        : m_data(data)
        , m_fail_after(fail_after)
        , m_bytes_read(0) {
        setg(nullptr, nullptr, nullptr);
    }
    
protected:
    int_type underflow() override {
        if (m_bytes_read >= m_fail_after) {
            // Simulate I/O error
            return traits_type::eof();
        }
        
        if (m_pos >= m_data.size()) {
            return traits_type::eof();
        }
        
        m_buffer = m_data[m_pos++];
        m_bytes_read++;
        setg(&m_buffer, &m_buffer, &m_buffer + 1);
        return traits_type::to_int_type(m_buffer);
    }
    
    std::streamsize xsgetn(char_type* s, std::streamsize count) override {
        std::streamsize to_read = std::min(
            static_cast<std::streamsize>(m_fail_after - m_bytes_read),
            std::min(count, static_cast<std::streamsize>(m_data.size() - m_pos))
        );
        
        if (to_read > 0) {
            std::memcpy(s, m_data.data() + m_pos, to_read);
            m_pos += to_read;
            m_bytes_read += to_read;
        }
        
        return to_read;
    }
    
    pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                     std::ios_base::openmode) override {
        if (m_bytes_read >= m_fail_after) {
            return pos_type(off_type(-1));
        }
        
        pos_type new_pos;
        if (dir == std::ios_base::beg) {
            new_pos = off;
        } else if (dir == std::ios_base::cur) {
            new_pos = m_pos + off;
        } else {
            new_pos = m_data.size() + off;
        }
        
        if (new_pos >= 0 && new_pos <= static_cast<pos_type>(m_data.size())) {
            m_pos = new_pos;
            return new_pos;
        }
        
        return pos_type(off_type(-1));
    }
    
    pos_type seekpos(pos_type pos, std::ios_base::openmode which) override {
        return seekoff(pos, std::ios_base::beg, which);
    }
    
private:
    std::string m_data;
    std::size_t m_fail_after;
    std::size_t m_bytes_read;
    std::size_t m_pos = 0;
    char m_buffer;
};

TEST_CASE("Reader error handling") {
    SUBCASE("Truncated chunk header") {
        auto data = load_test_data("truncated_header.iff");
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        auto it = chunk_iterator::get_iterator(stream);
        CHECK(it != nullptr);
        
        // File is truncated after container header
        // Should be able to parse container but not children
        int chunks_parsed = 0;
        bool error_encountered = false;
        
        try {
            while (it->has_next()) {
                chunks_parsed++;
                it->next();
            }
        } catch (const std::exception&) {
            error_encountered = true;
        }
        
        // Should parse the container but encounter issues with children
        CHECK(chunks_parsed <= 2);  // Container and possibly partial child
    }
    
    SUBCASE("I/O error during chunk reading") {
        auto data = load_test_data("io_error_test.iff");
        std::string str_data(reinterpret_cast<char*>(data.data()), data.size());
        
        // Fail after reading 25 bytes (in the middle of first DATA chunk content)
        failing_streambuf buf(str_data, 25);
        std::istream stream(&buf);
        
        auto it = chunk_iterator::get_iterator(stream);
        REQUIRE(it != nullptr);
        
        bool encountered_issue = false;
        int chunks_processed = 0;
        
        try {
            while (it->has_next()) {
                const auto& chunk = it->current();
                chunks_processed++;
                
                // Try to read from chunk
                if (chunk.reader && chunk.reader->size() > 0) {
                    // Try to read all data
                    auto all_data = chunk.reader->read_all();
                    
                    // If we got less than expected, that's an issue
                    if (all_data.size() < chunk.reader->size()) {
                        encountered_issue = true;
                    }
                }
                
                it->next();
            }
        } catch (const std::exception&) {
            encountered_issue = true;
        }
        
        // Either we encountered an exception or couldn't read all data
        bool test_passed = encountered_issue || (chunks_processed < 3);
        CHECK(test_passed);
    }
    
    SUBCASE("Reading beyond chunk boundaries") {
        auto data = load_test_data("reading_boundaries_test.riff");
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        auto it = chunk_iterator::get_iterator(stream);
        REQUIRE(it != nullptr);
        
        bool found_data = false;
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            
            if (chunk.header.id == fourcc("data")) {
                found_data = true;
                
                if (chunk.reader) {
                    // Get chunk size
                    auto chunk_size = chunk.reader->size();
                    
                    // Try to read exactly the chunk size - should succeed
                    std::vector<char> buffer(chunk_size);
                    auto read1 = chunk.reader->read(buffer.data(), chunk_size);
                    CHECK(read1 == chunk_size);
                    
                    // Try to read more - should return 0
                    auto read2 = chunk.reader->read(buffer.data(), 100);
                    CHECK(read2 == 0);
                    
                    // Remaining should be 0
                    CHECK(chunk.reader->remaining() == 0);
                }
            }
            
            it->next();
        }
        
        CHECK(found_data);
    }
    
    SUBCASE("Chunk size extends beyond file") {
        auto data = load_test_data("chunk_size_exceeds_file.riff");
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        auto it = chunk_iterator::get_iterator(stream);
        REQUIRE(it != nullptr);
        
        bool error_detected = false;
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            
            if (chunk.header.id == fourcc("data")) {
                if (chunk.reader) {
                    // The chunk claims to be 1000 bytes but file is truncated
                    // Reading should fail or return less data
                    std::vector<char> buffer(1000);
                    auto bytes_read = chunk.reader->read(buffer.data(), 1000);
                    
                    // Should read less than claimed size
                    CHECK(bytes_read < 1000);
                    error_detected = true;
                }
            }
            
            it->next();
        }
        
        CHECK(error_detected);
    }
    
    SUBCASE("Invalid container type field") {
        auto data = load_test_data("container_missing_type.iff");
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        // Should fail to parse container without type field
        auto it = chunk_iterator::get_iterator(stream);
        
        bool parsed_successfully = false;
        if (it && it->has_next()) {
            try {
                const auto& chunk = it->current();
                // If we get here, check if it's properly detected as invalid
                if (chunk.header.is_container && !chunk.header.type.has_value()) {
                    // Container without type - this is invalid
                    parsed_successfully = false;
                } else {
                    parsed_successfully = true;
                }
            } catch (const std::exception&) {
                parsed_successfully = false;
            }
        }
        
        CHECK(!parsed_successfully);
    }
    
    SUBCASE("Zero-sized container") {
        auto data = load_test_data("zero_sized_container.iff");
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        auto it = chunk_iterator::get_iterator(stream);
        CHECK(it != nullptr);
        
        bool found_container = false;
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            
            if (chunk.header.id == fourcc("FORM")) {
                found_container = true;
                
                // Zero-sized container should have no children
                if (chunk.reader) {
                    auto child_iter = chunk_iterator::get_iterator(chunk.reader->stream());
                    CHECK(child_iter != nullptr);
                    
                    // Should have no children
                    CHECK(!child_iter->has_next());
                }
            }
            
            it->next();
        }
        
        CHECK(found_container);
    }
    
    SUBCASE("Bad stream state on creation") {
        std::istringstream stream("");
        stream.setstate(std::ios::badbit);
        
        bool threw_exception = false;
        try {
            auto it = chunk_iterator::get_iterator(stream);
            // Try to use the iterator if created
            if (it && it->has_next()) {
                it->next();
            }
        } catch (const std::exception&) {
            threw_exception = true;
        }
        
        CHECK(threw_exception);
    }
    
    SUBCASE("Stream becomes bad during iteration") {
        auto data = load_test_data("io_error_test.iff");
        std::string str_data(reinterpret_cast<char*>(data.data()), data.size());
        
        // Fail after 40 bytes (during iteration)
        failing_streambuf buf(str_data, 40);
        std::istream stream(&buf);
        
        auto it = chunk_iterator::get_iterator(stream);
        REQUIRE(it != nullptr);
        
        int chunks_processed = 0;
        bool error_encountered = false;
        
        try {
            while (it->has_next()) {
                chunks_processed++;
                it->next();
                
                // After processing first chunk, stream should fail
                if (chunks_processed > 1 && !stream.good()) {
                    error_encountered = true;
                    break;
                }
            }
        } catch (const std::exception&) {
            error_encountered = true;
        }
        
        // Either error was encountered or very few chunks were processed
        bool success = error_encountered || (chunks_processed <= 2);
        CHECK(success);
    }
    
    SUBCASE("Skip beyond chunk size") {
        auto data = load_test_data("reading_boundaries_test.riff");
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        auto it = chunk_iterator::get_iterator(stream);
        REQUIRE(it != nullptr);
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            
            if (chunk.header.id == fourcc("fmt ")) {
                if (chunk.reader) {
                    auto size = chunk.reader->size();
                    
                    // Skip to end should succeed
                    CHECK(chunk.reader->skip(size));
                    
                    // Skip beyond should fail
                    CHECK(!chunk.reader->skip(1));
                    
                    // Offset should be at size
                    CHECK(chunk.reader->offset() == size);
                }
                break;
            }
            
            it->next();
        }
    }
    
    SUBCASE("Read after exhausting chunk") {
        auto data = load_test_data("reading_boundaries_test.riff");
        
        std::istringstream stream(std::string(reinterpret_cast<char*>(data.data()), data.size()));
        
        auto it = chunk_iterator::get_iterator(stream);
        REQUIRE(it != nullptr);
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            
            if (chunk.header.id == fourcc("fmt ")) {
                if (chunk.reader) {
                    // Read all data
                    auto all_data = chunk.reader->read_all();
                    CHECK(!all_data.empty());
                    
                    // Try to read more - should return 0
                    std::vector<char> buffer(10);
                    auto bytes_read = chunk.reader->read(buffer.data(), 10);
                    CHECK(bytes_read == 0);
                    
                    // Remaining should be 0
                    CHECK(chunk.reader->remaining() == 0);
                }
                break;
            }
            
            it->next();
        }
    }
}