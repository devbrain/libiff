/**
 * @file simple_example.cpp
 * @brief Simple example demonstrating basic libiff usage
 * 
 * This is a minimal example showing how to parse any IFF/RIFF file
 * and print information about its chunks.
 */

#include <iff/parser.hh>
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <file>\n";
        std::cout << "\n";
        std::cout << "Simple example that lists all chunks in a file.\n";
        return 1;
    }
    
    // Open the file
    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file '" << argv[1] << "'\n";
        return 1;
    }
    
    std::cout << "Parsing: " << argv[1] << "\n";
    std::cout << "====================\n\n";
    
    try {
        // Parse the file and print each chunk
        iff::for_each_chunk(file, [](const auto& chunk) {
            std::cout << "Chunk: " << chunk.header.id.to_string()
                      << " (" << chunk.header.size << " bytes)\n";
            
            // Show container context if available
            if (chunk.current_form) {
                std::cout << "  In FORM: " << chunk.current_form->to_string() << "\n";
            } else if (chunk.current_container) {
                std::cout << "  In container: " << chunk.current_container->to_string() << "\n";
            }
        });
        
        std::cout << "\nParsing completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}