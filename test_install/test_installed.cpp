/**
 * Test program to verify libiff installation
 */

#include <iff/parser.hh>
#include <iff/chunk_iterator.hh>
#include <iff/fourcc.hh>
#include <iff/parse_options.hh>
#include <iff/exceptions.hh>
#include <iostream>
#include <sstream>
#include <vector>

// Create a simple RIFF file in memory
std::vector<uint8_t> create_test_riff() {
    std::vector<uint8_t> data;
    
    // RIFF header
    data.push_back('R'); data.push_back('I'); data.push_back('F'); data.push_back('F');
    
    // Size (little-endian) - will be 28 bytes (4 + 8 + 8 + 8)
    data.push_back(28); data.push_back(0); data.push_back(0); data.push_back(0);
    
    // WAVE type
    data.push_back('W'); data.push_back('A'); data.push_back('V'); data.push_back('E');
    
    // fmt chunk
    data.push_back('f'); data.push_back('m'); data.push_back('t'); data.push_back(' ');
    data.push_back(4); data.push_back(0); data.push_back(0); data.push_back(0);  // size = 4
    data.push_back(1); data.push_back(0); data.push_back(2); data.push_back(0);  // format data
    
    // data chunk
    data.push_back('d'); data.push_back('a'); data.push_back('t'); data.push_back('a');
    data.push_back(4); data.push_back(0); data.push_back(0); data.push_back(0);  // size = 4
    data.push_back(0); data.push_back(0); data.push_back(0); data.push_back(0);  // data
    
    return data;
}

int main() {
    std::cout << "Testing libiff installation...\n\n";
    
    try {
        // Test 1: Create fourcc
        std::cout << "Test 1: fourcc creation\n";
        iff::fourcc test_id("TEST");
        std::cout << "  Created fourcc: " << test_id.to_string() << "\n";
        std::cout << "  ✓ fourcc works\n\n";
        
        // Test 2: Parse options
        std::cout << "Test 2: Parse options\n";
        iff::parse_options opts;
        opts.max_chunk_size = 1024 * 1024;
        opts.max_depth = 10;
        opts.strict = true;
        std::cout << "  Created parse_options with max_chunk_size=" 
                  << opts.max_chunk_size << "\n";
        std::cout << "  ✓ parse_options works\n\n";
        
        // Test 3: Parse a simple RIFF stream
        std::cout << "Test 3: Parse RIFF stream\n";
        auto test_data = create_test_riff();
        std::string data_str(test_data.begin(), test_data.end());
        std::istringstream stream(data_str, std::ios::binary);
        
        int chunk_count = 0;
        iff::for_each_chunk(stream, [&chunk_count](const auto& chunk) {
            std::cout << "  Found chunk: " << chunk.header.id.to_string()
                      << " (" << chunk.header.size << " bytes)\n";
            chunk_count++;
        });
        
        if (chunk_count > 0) {
            std::cout << "  ✓ Parsed " << chunk_count << " chunks\n\n";
        } else {
            std::cout << "  ✗ No chunks parsed\n\n";
            return 1;
        }
        
        // Test 4: Iterator interface
        std::cout << "Test 4: Iterator interface\n";
        stream.clear();
        stream.seekg(0);
        
        auto it = iff::chunk_iterator::get_iterator(stream);
        int iter_count = 0;
        while (it->has_next()) {
            auto& chunk = it->current();
            std::cout << "  Iterator found: " << chunk.header.id.to_string() << "\n";
            it->next();
            iter_count++;
        }
        
        if (iter_count > 0) {
            std::cout << "  ✓ Iterator processed " << iter_count << " chunks\n\n";
        } else {
            std::cout << "  ✗ Iterator found no chunks\n\n";
            return 1;
        }
        
        // Test 5: Exception handling
        std::cout << "Test 5: Exception handling\n";
        try {
            std::istringstream bad_stream("BAD", std::ios::binary);
            auto bad_it = iff::chunk_iterator::get_iterator(bad_stream);
            std::cout << "  ✗ Should have thrown exception\n\n";
            return 1;
        } catch (const iff::format_error& e) {
            std::cout << "  Caught format_error: " << e.what() << "\n";
            std::cout << "  ✓ Exception handling works\n\n";
        }
        
        std::cout << "========================================\n";
        std::cout << "✓ All tests passed!\n";
        std::cout << "✓ libiff is correctly installed and working\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}