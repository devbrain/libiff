/**
 * @file aiff_parser.cpp
 * @brief Example AIFF file parser using libiff
 * 
 * This example demonstrates how to parse AIFF (Audio Interchange File Format)
 * files, which use the IFF-85 big-endian format.
 */

#include <iff/parser.hh>
#include <iff/handler_registry.hh>
#include <iff/chunk_reader.hh>
#include <iff/byte_order.hh>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cstring>
#include <cmath>

// AIFF Common chunk structure
#pragma pack(push, 1)
struct AIFFCommon {
    int16_t num_channels;
    uint32_t num_sample_frames;
    int16_t sample_size;
    uint8_t sample_rate[10];  // 80-bit IEEE extended precision
};
#pragma pack(pop)

class AIFFParser {
public:
    void parse(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file: " << filename << "\n";
            return;
        }
        
        std::cout << "Parsing AIFF file: " << filename << "\n";
        std::cout << "=====================================\n\n";
        
        iff::handler_registry handlers;
        
        // Handle COMM (Common) chunk
        handlers.on_chunk_in_form(
            iff::fourcc("AIFF"),
            iff::fourcc("COMM"),
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_common_chunk(event);
                }
            }
        );
        
        // Handle AIFC variant
        handlers.on_chunk_in_form(
            iff::fourcc("AIFC"),
            iff::fourcc("COMM"),
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    is_aifc_ = true;
                    handle_common_chunk(event);
                }
            }
        );
        
        // Handle SSND (Sound Data) chunk
        handlers.on_chunk_in_form(
            iff::fourcc("AIFF"),
            iff::fourcc("SSND"),
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_sound_data_chunk(event);
                }
            }
        );
        
        // Also handle SSND in AIFC
        handlers.on_chunk_in_form(
            iff::fourcc("AIFC"),
            iff::fourcc("SSND"),
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_sound_data_chunk(event);
                }
            }
        );
        
        // Handle NAME chunk (optional)
        handlers.on_chunk(
            iff::fourcc("NAME"),
            [](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    std::vector<char> name(event.header.size + 1, 0);
                    event.reader->read(name.data(), event.header.size);
                    std::cout << "Name: " << name.data() << "\n\n";
                }
            }
        );
        
        // Handle AUTH chunk (author)
        handlers.on_chunk(
            iff::fourcc("AUTH"),
            [](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    std::vector<char> author(event.header.size + 1, 0);
                    event.reader->read(author.data(), event.header.size);
                    std::cout << "Author: " << author.data() << "\n\n";
                }
            }
        );
        
        // Handle (c) chunk (copyright)
        handlers.on_chunk(
            iff::fourcc("(c) "),
            [](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    std::vector<char> copyright(event.header.size + 1, 0);
                    event.reader->read(copyright.data(), event.header.size);
                    std::cout << "Copyright: " << copyright.data() << "\n\n";
                }
            }
        );
        
        // Handle ANNO chunk (annotation)
        handlers.on_chunk(
            iff::fourcc("ANNO"),
            [](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    std::vector<char> annotation(event.header.size + 1, 0);
                    event.reader->read(annotation.data(), event.header.size);
                    std::cout << "Annotation: " << annotation.data() << "\n\n";
                }
            }
        );
        
        // Handle MARK chunk (markers)
        handlers.on_chunk(
            iff::fourcc("MARK"),
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_mark_chunk(event);
                }
            }
        );
        
        try {
            iff::parse(file, handlers);
            print_summary();
        } catch (const std::exception& e) {
            std::cerr << "Error parsing file: " << e.what() << "\n";
        }
    }
    
private:
    void handle_common_chunk(const iff::chunk_event& event) {
        std::cout << "Common Chunk:\n";
        std::cout << "  Size: " << event.header.size << " bytes\n";
        
        AIFFCommon comm;
        event.reader->read(&comm, sizeof(comm));
        
        // Convert from big-endian
        comm.num_channels = swap_be16(comm.num_channels);
        comm.num_sample_frames = swap_be32(comm.num_sample_frames);
        comm.sample_size = swap_be16(comm.sample_size);
        
        num_channels_ = comm.num_channels;
        num_sample_frames_ = comm.num_sample_frames;
        sample_size_ = comm.sample_size;
        sample_rate_ = parse_extended_float(comm.sample_rate);
        has_common_ = true;
        
        std::cout << "  Channels: " << comm.num_channels << "\n";
        std::cout << "  Sample Frames: " << comm.num_sample_frames << "\n";
        std::cout << "  Sample Size: " << comm.sample_size << " bits\n";
        std::cout << "  Sample Rate: " << sample_rate_ << " Hz\n";
        
        // AIFC has compression type after the common fields
        if (is_aifc_ && event.header.size > sizeof(comm)) {
            uint32_t compression_type;
            event.reader->read(&compression_type, sizeof(compression_type));
            
            char comp_str[5] = {0};
            memcpy(comp_str, &compression_type, 4);
            std::cout << "  Compression: '" << comp_str << "' (" << get_compression_name(compression_type) << ")\n";
            
            // Read compression name string if present
            uint8_t name_len;
            if (event.reader->read(&name_len, 1) == 1 && name_len > 0) {
                std::vector<char> comp_name(name_len + 1, 0);
                event.reader->read(comp_name.data(), name_len);
                std::cout << "  Compression Name: " << comp_name.data() << "\n";
            }
        }
        
        std::cout << "\n";
    }
    
    void handle_sound_data_chunk(const iff::chunk_event& event) {
        std::cout << "Sound Data Chunk:\n";
        std::cout << "  Size: " << event.header.size << " bytes\n";
        
        // Read offset and block size
        uint32_t offset, block_size;
        event.reader->read(&offset, sizeof(offset));
        event.reader->read(&block_size, sizeof(block_size));
        
        offset = swap_be32(offset);
        block_size = swap_be32(block_size);
        
        std::cout << "  Offset: " << offset << "\n";
        std::cout << "  Block Size: " << block_size << "\n";
        
        uint64_t data_size = event.header.size - 8;
        sound_data_size_ = data_size;
        
        if (has_common_) {
            double duration = static_cast<double>(num_sample_frames_) / sample_rate_;
            std::cout << "  Duration: " << std::fixed << std::setprecision(2) 
                      << duration << " seconds\n";
        }
        
        // Read first few samples
        if (data_size > 0 && offset == 0) {
            const size_t preview_bytes = std::min<size_t>(32, data_size);
            std::vector<uint8_t> preview(preview_bytes);
            event.reader->read(preview.data(), preview_bytes);
            
            std::cout << "  First " << preview_bytes << " bytes (hex): ";
            for (size_t i = 0; i < std::min<size_t>(16, preview_bytes); ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') 
                          << static_cast<int>(preview[i]) << " ";
            }
            std::cout << std::dec << "\n";
        }
        
        std::cout << "\n";
    }
    
    void handle_mark_chunk(const iff::chunk_event& event) {
        std::cout << "Marker Chunk:\n";
        std::cout << "  Size: " << event.header.size << " bytes\n";
        
        uint16_t num_markers;
        event.reader->read(&num_markers, sizeof(num_markers));
        num_markers = swap_be16(num_markers);
        
        std::cout << "  Number of Markers: " << num_markers << "\n";
        
        for (int i = 0; i < num_markers && i < 5; ++i) {  // Show first 5 markers
            uint16_t marker_id;
            uint32_t position;
            uint8_t name_len;
            
            event.reader->read(&marker_id, sizeof(marker_id));
            event.reader->read(&position, sizeof(position));
            event.reader->read(&name_len, sizeof(name_len));
            
            marker_id = swap_be16(marker_id);
            position = swap_be32(position);
            
            std::vector<char> name(name_len + 1, 0);
            event.reader->read(name.data(), name_len);
            
            // Skip padding byte if name length is even
            if (name_len % 2 == 0) {
                event.reader->skip(1);
            }
            
            std::cout << "    Marker " << marker_id << ": pos=" << position 
                      << ", name=\"" << name.data() << "\"\n";
        }
        
        if (num_markers > 5) {
            std::cout << "    ... and " << (num_markers - 5) << " more markers\n";
        }
        
        std::cout << "\n";
    }
    
    void print_summary() {
        std::cout << "=====================================\n";
        std::cout << "Summary:\n";
        
        if (is_aifc_) {
            std::cout << "  Format: AIFF-C (compressed)\n";
        } else {
            std::cout << "  Format: AIFF\n";
        }
        
        if (has_common_) {
            std::cout << "  " << num_channels_ << " channel(s), "
                      << sample_rate_ << " Hz, "
                      << sample_size_ << " bits\n";
            
            if (num_sample_frames_ > 0) {
                double duration = static_cast<double>(num_sample_frames_) / sample_rate_;
                std::cout << "  Duration: " << std::fixed << std::setprecision(2) 
                          << duration << " seconds\n";
            }
            
            if (sound_data_size_ > 0) {
                std::cout << "  Sound Data: " << sound_data_size_ << " bytes\n";
            }
        } else {
            std::cout << "  No common chunk found\n";
        }
    }
    
    // Helper functions for big-endian conversion
    uint16_t swap_be16(uint16_t val) {
        return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
    }
    
    uint32_t swap_be32(uint32_t val) {
        return ((val & 0xFF) << 24) |
               ((val & 0xFF00) << 8) |
               ((val & 0xFF0000) >> 8) |
               ((val >> 24) & 0xFF);
    }
    
    // Parse 80-bit IEEE extended precision float (simplified)
    double parse_extended_float(const uint8_t* bytes) {
        // This is a simplified conversion that works for common sample rates
        // A full implementation would need proper 80-bit float parsing
        uint16_t exponent = (bytes[0] << 8) | bytes[1];
        uint64_t mantissa = 0;
        for (int i = 0; i < 8; ++i) {
            mantissa = (mantissa << 8) | bytes[2 + i];
        }
        
        // Common sample rates (simplified)
        if (exponent == 0x400E && mantissa == 0xAC44000000000000ULL) return 44100.0;
        if (exponent == 0x400E && mantissa == 0xBB80000000000000ULL) return 48000.0;
        if (exponent == 0x400D && mantissa == 0xFA00000000000000ULL) return 32000.0;
        if (exponent == 0x400C && mantissa == 0xBF40000000000000ULL) return 24000.0;
        if (exponent == 0x400C && mantissa == 0x9F40000000000000ULL) return 20000.0;
        if (exponent == 0x400B && mantissa == 0xFF10000000000000ULL) return 16000.0;
        if (exponent == 0x400B && mantissa == 0xBF40000000000000ULL) return 12000.0;
        if (exponent == 0x400B && mantissa == 0x9F40000000000000ULL) return 10000.0;
        if (exponent == 0x400A && mantissa == 0xFA00000000000000ULL) return 8000.0;
        
        // Default approximation
        int exp = (exponent & 0x7FFF) - 16383;
        double result = ldexp(static_cast<double>(mantissa) / (1ULL << 63), exp);
        return result;
    }
    
    const char* get_compression_name(uint32_t comp_type) {
        // Convert to string for comparison
        char comp_str[5] = {0};
        comp_str[0] = (comp_type >> 24) & 0xFF;
        comp_str[1] = (comp_type >> 16) & 0xFF;
        comp_str[2] = (comp_type >> 8) & 0xFF;
        comp_str[3] = comp_type & 0xFF;
        
        if (strcmp(comp_str, "NONE") == 0) return "No compression";
        if (strcmp(comp_str, "ACE2") == 0) return "ACE 2-to-1";
        if (strcmp(comp_str, "ACE8") == 0) return "ACE 8-to-3";
        if (strcmp(comp_str, "MAC3") == 0) return "MACE 3-to-1";
        if (strcmp(comp_str, "MAC6") == 0) return "MACE 6-to-1";
        if (strcmp(comp_str, "ulaw") == 0) return "µ-law";
        if (strcmp(comp_str, "alaw") == 0) return "A-law";
        if (strcmp(comp_str, "fl32") == 0) return "32-bit float";
        if (strcmp(comp_str, "fl64") == 0) return "64-bit float";
        
        return "Unknown";
    }
    
    bool is_aifc_ = false;
    bool has_common_ = false;
    int16_t num_channels_ = 0;
    uint32_t num_sample_frames_ = 0;
    int16_t sample_size_ = 0;
    double sample_rate_ = 0.0;
    uint64_t sound_data_size_ = 0;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <aiff_file>\n";
        std::cout << "\n";
        std::cout << "This example parses an AIFF/AIFF-C file and displays:\n";
        std::cout << "  - Audio format information\n";
        std::cout << "  - Sample rate and bit depth\n";
        std::cout << "  - Duration and data size\n";
        std::cout << "  - Metadata (name, author, copyright)\n";
        std::cout << "  - Markers if present\n";
        return 1;
    }
    
    AIFFParser parser;
    parser.parse(argv[1]);
    
    return 0;
}