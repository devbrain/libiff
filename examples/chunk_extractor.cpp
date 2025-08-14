/**
 * @file chunk_extractor.cpp
 * @brief Extract specific chunks from IFF/RIFF files
 * 
 * This example shows how to search for and extract specific chunks
 * from files, saving them as separate files or displaying their contents.
 */

#include <iff/parser.hh>
#include <iff/chunk_reader.hh>
#include <iff/parse_options.hh>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <sstream>

class ChunkExtractor {
public:
    struct ExtractedChunk {
        iff::fourcc id;
        uint64_t size;
        uint64_t offset;
        std::vector<uint8_t> data;
        std::optional<iff::fourcc> parent_form;
        std::optional<iff::fourcc> parent_container;
    };
    
    void extract(const std::string& filename, 
                const std::string& chunk_id,
                bool save_to_file = false,
                bool show_hex = false) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file: " << filename << "\n";
            return;
        }
        
        target_chunk_id_ = iff::fourcc(chunk_id.c_str());
        save_to_file_ = save_to_file;
        show_hex_ = show_hex;
        
        std::cout << "Extracting chunks with ID: '" << chunk_id << "'\n";
        std::cout << "From file: " << filename << "\n";
        std::cout << "=========================================\n\n";
        
        // Parse options
        iff::parse_options options;
        options.strict = false;
        options.max_chunk_size = 1ULL << 32;  // 4GB max
        
        // Extract matching chunks
        iff::for_each_chunk(file, [this](auto& chunk) {
            if (chunk.header.id == target_chunk_id_) {
                extract_chunk(chunk);
            }
        }, options);
        
        // Print summary
        print_summary();
        
        // Save extracted chunks if requested
        if (save_to_file_ && !extracted_chunks_.empty()) {
            save_chunks(filename);
        }
    }
    
    void extract_all(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file: " << filename << "\n";
            return;
        }
        
        std::cout << "Extracting all data chunks from: " << filename << "\n";
        std::cout << "=========================================\n\n";
        
        // Parse options
        iff::parse_options options;
        options.strict = false;
        options.max_chunk_size = 1ULL << 32;
        
        // Extract all non-container chunks
        iff::for_each_chunk(file, [this](auto& chunk) {
            extract_chunk(chunk);
        }, options);
        
        // Print summary
        print_summary();
        
        // Group by chunk type
        std::map<std::string, std::vector<ExtractedChunk*>> by_type;
        for (auto& chunk : extracted_chunks_) {
            by_type[chunk.id.to_string()].push_back(&chunk);
        }
        
        std::cout << "\nChunks by Type:\n";
        std::cout << "---------------\n";
        for (const auto& [type, chunks] : by_type) {
            uint64_t total_size = 0;
            for (const auto* chunk : chunks) {
                total_size += chunk->size;
            }
            std::cout << "  " << type << ": " << chunks.size() << " chunk(s), "
                      << format_size(total_size) << " total\n";
        }
    }
    
private:
    void extract_chunk(const iff::chunk_iterator::chunk_info& chunk_info) {
        ExtractedChunk chunk;
        chunk.id = chunk_info.header.id;
        chunk.size = chunk_info.header.size;
        chunk.offset = chunk_info.header.file_offset;
        chunk.parent_form = chunk_info.current_form;
        chunk.parent_container = chunk_info.current_container;
        
        // Read chunk data
        try {
            chunk.data.resize(chunk.size);
            size_t bytes_read = chunk_info.reader->read(chunk.data.data(), chunk.size);
            
            if (bytes_read != chunk.size) {
                std::cerr << "Warning: Only read " << bytes_read 
                          << " of " << chunk.size << " bytes\n";
                chunk.data.resize(bytes_read);
            }
            
            extracted_chunks_.push_back(std::move(chunk));
            
            // Display chunk info
            display_chunk(extracted_chunks_.back());
            
        } catch (const std::exception& e) {
            std::cerr << "Error reading chunk: " << e.what() << "\n";
        }
    }
    
    void display_chunk(const ExtractedChunk& chunk) {
        std::cout << "Found: " << chunk.id.to_string();
        
        if (chunk.parent_form) {
            std::cout << " (in FORM:" << chunk.parent_form->to_string() << ")";
        } else if (chunk.parent_container) {
            std::cout << " (in " << chunk.parent_container->to_string() << ")";
        }
        
        std::cout << "\n";
        std::cout << "  Offset: 0x" << std::hex << chunk.offset << std::dec << "\n";
        std::cout << "  Size: " << chunk.size << " bytes\n";
        
        if (show_hex_ && !chunk.data.empty()) {
            std::cout << "  Data (first 256 bytes):\n";
            display_hex_dump(chunk.data.data(), 
                           std::min<size_t>(256, chunk.data.size()));
        }
        
        // Try to interpret as text if printable
        if (is_text_chunk(chunk)) {
            std::cout << "  Content (text):\n    \"";
            for (size_t i = 0; i < std::min<size_t>(200, chunk.data.size()); ++i) {
                char c = static_cast<char>(chunk.data[i]);
                if (c >= 32 && c <= 126) {
                    std::cout << c;
                } else if (c == '\n') {
                    std::cout << "\\n";
                } else if (c == '\r') {
                    std::cout << "\\r";
                } else if (c == '\t') {
                    std::cout << "\\t";
                } else {
                    std::cout << ".";
                }
            }
            if (chunk.data.size() > 200) {
                std::cout << "...";
            }
            std::cout << "\"\n";
        }
        
        std::cout << "\n";
    }
    
    void display_hex_dump(const uint8_t* data, size_t size) {
        const size_t bytes_per_line = 16;
        
        for (size_t offset = 0; offset < size; offset += bytes_per_line) {
            // Offset
            std::cout << "    " << std::hex << std::setw(8) 
                      << std::setfill('0') << offset << "  ";
            
            // Hex bytes
            for (size_t i = 0; i < bytes_per_line; ++i) {
                if (offset + i < size) {
                    std::cout << std::hex << std::setw(2) 
                              << std::setfill('0') 
                              << static_cast<int>(data[offset + i]) << " ";
                } else {
                    std::cout << "   ";
                }
                
                if (i == 7) std::cout << " ";
            }
            
            std::cout << " |";
            
            // ASCII representation
            for (size_t i = 0; i < bytes_per_line && offset + i < size; ++i) {
                char c = static_cast<char>(data[offset + i]);
                if (c >= 32 && c <= 126) {
                    std::cout << c;
                } else {
                    std::cout << ".";
                }
            }
            
            std::cout << "|\n";
        }
        std::cout << std::dec;
    }
    
    bool is_text_chunk(const ExtractedChunk& chunk) {
        // Check common text chunk IDs
        std::string id = chunk.id.to_string();
        if (id == "NAME" || id == "AUTH" || id == "(c) " || 
            id == "ANNO" || id == "COMT" || id == "TEXT") {
            return true;
        }
        
        // Check if content looks like text (heuristic)
        if (chunk.data.size() < 4) return false;
        
        int printable = 0;
        int non_printable = 0;
        size_t check_size = std::min<size_t>(100, chunk.data.size());
        
        for (size_t i = 0; i < check_size; ++i) {
            char c = static_cast<char>(chunk.data[i]);
            if ((c >= 32 && c <= 126) || c == '\n' || c == '\r' || c == '\t') {
                printable++;
            } else if (c != 0) {
                non_printable++;
            }
        }
        
        // If >80% printable, consider it text
        return printable > 0 && 
               (static_cast<double>(printable) / (printable + non_printable)) > 0.8;
    }
    
    void save_chunks(const std::string& source_filename) {
        // Extract base name without using filesystem
        size_t last_slash = source_filename.find_last_of("/\\");
        size_t last_dot = source_filename.find_last_of(".");
        std::string base_name;
        if (last_slash != std::string::npos) {
            if (last_dot != std::string::npos && last_dot > last_slash) {
                base_name = source_filename.substr(last_slash + 1, last_dot - last_slash - 1);
            } else {
                base_name = source_filename.substr(last_slash + 1);
            }
        } else {
            if (last_dot != std::string::npos) {
                base_name = source_filename.substr(0, last_dot);
            } else {
                base_name = source_filename;
            }
        }
        
        std::cout << "Saving extracted chunks...\n";
        
        int index = 0;
        for (const auto& chunk : extracted_chunks_) {
            std::ostringstream filename;
            filename << base_name << "_" 
                     << chunk.id.to_string() << "_"
                     << std::setw(3) << std::setfill('0') << index
                     << ".chunk";
            
            std::ofstream out(filename.str(), std::ios::binary);
            if (out) {
                out.write(reinterpret_cast<const char*>(chunk.data.data()), 
                         chunk.data.size());
                std::cout << "  Saved: " << filename.str() 
                          << " (" << chunk.data.size() << " bytes)\n";
            } else {
                std::cerr << "  Failed to save: " << filename.str() << "\n";
            }
            
            index++;
        }
    }
    
    void print_summary() {
        std::cout << "Summary:\n";
        std::cout << "--------\n";
        std::cout << "  Chunks extracted: " << extracted_chunks_.size() << "\n";
        
        if (!extracted_chunks_.empty()) {
            uint64_t total_size = 0;
            for (const auto& chunk : extracted_chunks_) {
                total_size += chunk.data.size();
            }
            std::cout << "  Total data size: " << format_size(total_size) << "\n";
            
            // Find unique chunk types
            std::set<std::string> unique_types;
            for (const auto& chunk : extracted_chunks_) {
                unique_types.insert(chunk.id.to_string());
            }
            std::cout << "  Unique chunk types: " << unique_types.size() << "\n";
        }
    }
    
    std::string format_size(uint64_t size) {
        if (size < 1024) {
            return std::to_string(size) + " bytes";
        } else if (size < 1024 * 1024) {
            return std::to_string(size / 1024) + " KB";
        } else if (size < 1024 * 1024 * 1024) {
            return std::to_string(size / (1024 * 1024)) + " MB";
        } else {
            return std::to_string(size / (1024 * 1024 * 1024)) + " GB";
        }
    }
    
    iff::fourcc target_chunk_id_{0};
    bool save_to_file_ = false;
    bool show_hex_ = false;
    std::vector<ExtractedChunk> extracted_chunks_;
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <file> [chunk_id] [options]\n";
        std::cout << "\n";
        std::cout << "Extract chunks from IFF/RIFF files.\n";
        std::cout << "\n";
        std::cout << "Examples:\n";
        std::cout << "  " << argv[0] << " audio.wav data\n";
        std::cout << "    Extract all 'data' chunks\n";
        std::cout << "\n";
        std::cout << "  " << argv[0] << " file.iff NAME --hex\n";
        std::cout << "    Extract NAME chunks and show hex dump\n";
        std::cout << "\n";
        std::cout << "  " << argv[0] << " video.avi movi --save\n";
        std::cout << "    Extract and save movi chunks to files\n";
        std::cout << "\n";
        std::cout << "  " << argv[0] << " file.riff\n";
        std::cout << "    Extract all chunks (summary only)\n";
        std::cout << "\n";
        std::cout << "Options:\n";
        std::cout << "  --hex     Show hex dump of chunk data\n";
        std::cout << "  --save    Save chunks to separate files\n";
        return 1;
    }
    
    ChunkExtractor extractor;
    
    if (argc == 2) {
        // Extract all chunks
        extractor.extract_all(argv[1]);
    } else {
        // Parse options
        bool save = false;
        bool hex = false;
        
        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--save") {
                save = true;
            } else if (arg == "--hex") {
                hex = true;
            }
        }
        
        // Extract specific chunk type
        extractor.extract(argv[1], argv[2], save, hex);
    }
    
    return 0;
}