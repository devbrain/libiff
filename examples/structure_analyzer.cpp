/**
 * @file structure_analyzer.cpp
 * @brief Analyzes and displays the structure of any IFF/RIFF file
 * 
 * This example shows how to traverse the chunk hierarchy and
 * display the complete structure of any IFF or RIFF-based file.
 */

#include <iff/chunk_iterator.hh>
#include <iff/parse_options.hh>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>

class StructureAnalyzer {
public:
    void analyze(const std::string& filename, bool verbose = false) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file: " << filename << "\n";
            return;
        }
        
        verbose_ = verbose;
        
        std::cout << "File Structure Analysis: " << filename << "\n";
        std::cout << "=========================================\n\n";
        
        // Setup parse options
        iff::parse_options options;
        options.strict = false;  // Continue on errors
        options.max_chunk_size = 1ULL << 40;  // Allow up to 1TB chunks
        options.max_depth = 100;  // Deep nesting allowed
        
        // Set warning handler
        options.on_warning = [](uint64_t offset, 
                               std::string_view category,
                               std::string_view message) {
            std::cerr << "Warning at offset " << offset 
                      << " [" << category << "]: " << message << "\n";
        };
        
        try {
            auto it = iff::chunk_iterator::get_iterator(file, options);
            
            // Detect format
            detect_format(it.get());
            
            // Reset and analyze structure
            file.clear();
            file.seekg(0);
            it = iff::chunk_iterator::get_iterator(file, options);
            
            analyze_structure(it.get());
            
            print_summary();
            
        } catch (const std::exception& e) {
            std::cerr << "Error analyzing file: " << e.what() << "\n";
        }
    }
    
private:
    void detect_format(iff::chunk_iterator* it) {
        if (it->has_next()) {
            const auto& chunk = it->current();
            
            // Detect file format from first chunk
            if (chunk.header.id == iff::fourcc("FORM")) {
                format_ = "IFF-85";
                byte_order_ = "Big-endian";
            } else if (chunk.header.id == iff::fourcc("RIFF")) {
                format_ = "RIFF";
                byte_order_ = "Little-endian";
            } else if (chunk.header.id == iff::fourcc("RIFX")) {
                format_ = "RIFX";
                byte_order_ = "Big-endian";
            } else if (chunk.header.id == iff::fourcc("RF64")) {
                format_ = "RF64";
                byte_order_ = "Little-endian";
                supports_64bit_ = true;
            } else if (chunk.header.id == iff::fourcc("BW64")) {
                format_ = "BW64";
                byte_order_ = "Little-endian";
                supports_64bit_ = true;
            } else {
                format_ = "Unknown";
                byte_order_ = "Unknown";
            }
            
            // Get file type if it's a container
            if (chunk.header.is_container && chunk.header.type) {
                file_type_ = chunk.header.type->to_string();
            }
        }
    }
    
    void analyze_structure(iff::chunk_iterator* it) {
        std::cout << "Chunk Hierarchy:\n";
        std::cout << "----------------\n";
        
        int current_depth = -1;
        std::vector<uint64_t> container_stack;
        
        while (it->has_next()) {
            const auto& chunk = it->current();
            
            // Track depth changes
            while (current_depth >= chunk.depth) {
                if (!container_stack.empty()) {
                    container_stack.pop_back();
                }
                current_depth--;
            }
            current_depth = chunk.depth;
            
            // Display chunk info
            print_chunk(chunk);
            
            // Collect statistics
            chunk_count_++;
            total_data_size_ += chunk.header.size;
            
            if (chunk.header.is_container) {
                container_count_++;
                container_stack.push_back(chunk.header.size);
                
                // Track container types
                std::string container_type = chunk.header.id.to_string();
                if (chunk.header.type) {
                    container_type += ":" + chunk.header.type->to_string();
                }
                container_types_[container_type]++;
            } else {
                data_chunk_count_++;
                chunk_types_[chunk.header.id.to_string()]++;
                
                // Track size distribution
                if (chunk.header.size == 0) {
                    zero_size_chunks_++;
                } else if (chunk.header.size < 1024) {
                    small_chunks_++;
                } else if (chunk.header.size < 1024 * 1024) {
                    medium_chunks_++;
                } else {
                    large_chunks_++;
                }
                
                // Track largest chunk
                if (chunk.header.size > largest_chunk_size_) {
                    largest_chunk_size_ = chunk.header.size;
                    largest_chunk_id_ = chunk.header.id.to_string();
                }
            }
            
            // Track maximum depth
            if (chunk.depth > max_depth_) {
                max_depth_ = chunk.depth;
            }
            
            it->next();
        }
        
        std::cout << "\n";
    }
    
    void print_chunk(const iff::chunk_iterator::chunk_info& chunk) {
        // Indentation based on depth
        std::string indent(chunk.depth * 2, ' ');
        
        // Format chunk ID
        std::cout << indent;
        
        if (chunk.header.is_container) {
            // Container chunks in bold (using ANSI if terminal supports it)
            std::cout << "\033[1m" << chunk.header.id.to_string();
            
            if (chunk.header.type) {
                std::cout << ":" << chunk.header.type->to_string();
            }
            std::cout << "\033[0m";
            
            std::cout << " [Container, " << format_size(chunk.header.size) << "]";
        } else {
            // Data chunks
            std::cout << chunk.header.id.to_string();
            std::cout << " [" << format_size(chunk.header.size) << "]";
        }
        
        if (verbose_) {
            std::cout << " @ 0x" << std::hex << chunk.header.file_offset << std::dec;
            
            // Show context
            if (chunk.current_form) {
                std::cout << " (in FORM:" << chunk.current_form->to_string() << ")";
            } else if (chunk.current_container) {
                std::cout << " (in " << chunk.current_container->to_string() << ")";
            }
        }
        
        std::cout << "\n";
    }
    
    void print_summary() {
        std::cout << "Summary:\n";
        std::cout << "--------\n";
        std::cout << "  Format: " << format_;
        if (!file_type_.empty()) {
            std::cout << " (" << file_type_ << ")";
        }
        std::cout << "\n";
        std::cout << "  Byte Order: " << byte_order_ << "\n";
        if (supports_64bit_) {
            std::cout << "  64-bit Support: Yes\n";
        }
        std::cout << "\n";
        
        std::cout << "Statistics:\n";
        std::cout << "  Total Chunks: " << chunk_count_ << "\n";
        std::cout << "    Containers: " << container_count_ << "\n";
        std::cout << "    Data Chunks: " << data_chunk_count_ << "\n";
        std::cout << "  Total Data Size: " << format_size(total_data_size_) << "\n";
        std::cout << "  Maximum Depth: " << max_depth_ << "\n";
        std::cout << "\n";
        
        std::cout << "Size Distribution:\n";
        std::cout << "  Zero-size: " << zero_size_chunks_ << " chunks\n";
        std::cout << "  Small (<1KB): " << small_chunks_ << " chunks\n";
        std::cout << "  Medium (1KB-1MB): " << medium_chunks_ << " chunks\n";
        std::cout << "  Large (>1MB): " << large_chunks_ << " chunks\n";
        
        if (!largest_chunk_id_.empty()) {
            std::cout << "  Largest Chunk: " << largest_chunk_id_ 
                      << " (" << format_size(largest_chunk_size_) << ")\n";
        }
        std::cout << "\n";
        
        if (!chunk_types_.empty()) {
            std::cout << "Chunk Types (Top 10):\n";
            
            // Sort by frequency
            std::vector<std::pair<std::string, int>> sorted_types(
                chunk_types_.begin(), chunk_types_.end());
            std::sort(sorted_types.begin(), sorted_types.end(),
                     [](const auto& a, const auto& b) {
                         return a.second > b.second;
                     });
            
            int count = 0;
            for (const auto& [type, freq] : sorted_types) {
                std::cout << "  " << std::setw(4) << std::left << type 
                          << " : " << freq << " occurrence(s)\n";
                if (++count >= 10) break;
            }
            std::cout << "\n";
        }
        
        if (!container_types_.empty()) {
            std::cout << "Container Types:\n";
            for (const auto& [type, freq] : container_types_) {
                std::cout << "  " << type << " : " << freq << " occurrence(s)\n";
            }
        }
    }
    
    std::string format_size(uint64_t size) {
        if (size == 0) return "0 bytes";
        
        const char* units[] = {"bytes", "KB", "MB", "GB", "TB"};
        int unit_index = 0;
        double formatted_size = static_cast<double>(size);
        
        while (formatted_size >= 1024 && unit_index < 4) {
            formatted_size /= 1024;
            unit_index++;
        }
        
        if (unit_index == 0) {
            return std::to_string(size) + " bytes";
        } else {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << formatted_size 
                << " " << units[unit_index];
            return oss.str();
        }
    }
    
    // Options
    bool verbose_ = false;
    
    // File format info
    std::string format_;
    std::string file_type_;
    std::string byte_order_;
    bool supports_64bit_ = false;
    
    // Statistics
    int chunk_count_ = 0;
    int container_count_ = 0;
    int data_chunk_count_ = 0;
    uint64_t total_data_size_ = 0;
    int max_depth_ = 0;
    
    // Size distribution
    int zero_size_chunks_ = 0;
    int small_chunks_ = 0;
    int medium_chunks_ = 0;
    int large_chunks_ = 0;
    uint64_t largest_chunk_size_ = 0;
    std::string largest_chunk_id_;
    
    // Chunk type frequency
    std::map<std::string, int> chunk_types_;
    std::map<std::string, int> container_types_;
};

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cout << "Usage: " << argv[0] << " <iff/riff_file> [--verbose]\n";
        std::cout << "\n";
        std::cout << "Analyzes the structure of any IFF or RIFF-based file.\n";
        std::cout << "\n";
        std::cout << "Features:\n";
        std::cout << "  - Displays complete chunk hierarchy\n";
        std::cout << "  - Shows chunk sizes and statistics\n";
        std::cout << "  - Identifies file format (IFF, RIFF, RF64, etc.)\n";
        std::cout << "  - Counts chunk types and frequencies\n";
        std::cout << "  - Reports size distribution\n";
        std::cout << "\n";
        std::cout << "Options:\n";
        std::cout << "  --verbose    Show additional details (offsets, context)\n";
        return 1;
    }
    
    bool verbose = false;
    if (argc == 3 && std::string(argv[2]) == "--verbose") {
        verbose = true;
    }
    
    StructureAnalyzer analyzer;
    analyzer.analyze(argv[1], verbose);
    
    return 0;
}