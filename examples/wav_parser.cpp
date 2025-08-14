/**
 * @file wav_parser.cpp
 * @brief Example WAV file parser using libiff
 * 
 * This example demonstrates how to parse WAV audio files,
 * extract format information, and process audio data.
 */

#include <iff/parser.hh>
#include <iff/handler_registry.hh>
#include <iff/chunk_reader.hh>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <cstring>

struct WaveFormat {
    uint16_t format_tag;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t avg_bytes_per_sec;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

class WavParser {
public:
    void parse(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file: " << filename << "\n";
            return;
        }
        
        std::cout << "Parsing WAV file: " << filename << "\n";
        std::cout << "=====================================\n\n";
        
        iff::handler_registry handlers;
        
        // Handle the format chunk
        handlers.on_chunk_in_form(
            iff::fourcc("WAVE"),
            iff::fourcc("fmt "),
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_format_chunk(event);
                }
            }
        );
        
        // Handle the data chunk
        handlers.on_chunk_in_form(
            iff::fourcc("WAVE"),
            iff::fourcc("data"),
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_data_chunk(event);
                }
            }
        );
        
        // Handle fact chunk (for compressed formats)
        handlers.on_chunk_in_form(
            iff::fourcc("WAVE"),
            iff::fourcc("fact"),
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_fact_chunk(event);
                }
            }
        );
        
        // Handle LIST chunks (metadata)
        handlers.on_chunk_in_form(
            iff::fourcc("WAVE"),
            iff::fourcc("LIST"),
            [](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    std::cout << "Found LIST chunk\n";
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
    void handle_format_chunk(const iff::chunk_event& event) {
        std::cout << "Format Chunk:\n";
        std::cout << "  Size: " << event.header.size << " bytes\n";
        
        if (event.header.size < sizeof(WaveFormat)) {
            std::cerr << "  Warning: Format chunk too small\n";
            return;
        }
        
        WaveFormat fmt;
        event.reader->read(&fmt, sizeof(fmt));
        
        format_ = fmt;
        has_format_ = true;
        
        std::cout << "  Format Tag: " << fmt.format_tag << " (" << get_format_name(fmt.format_tag) << ")\n";
        std::cout << "  Channels: " << fmt.channels << "\n";
        std::cout << "  Sample Rate: " << fmt.sample_rate << " Hz\n";
        std::cout << "  Avg Bytes/Sec: " << fmt.avg_bytes_per_sec << "\n";
        std::cout << "  Block Align: " << fmt.block_align << "\n";
        std::cout << "  Bits/Sample: " << fmt.bits_per_sample << "\n";
        
        // Check for extended format
        if (event.header.size > sizeof(WaveFormat)) {
            uint16_t cb_size;
            event.reader->read(&cb_size, sizeof(cb_size));
            std::cout << "  Extended Format Size: " << cb_size << " bytes\n";
            
            if (fmt.format_tag == 0xFFFE && cb_size >= 22) { // WAVE_FORMAT_EXTENSIBLE
                uint16_t valid_bits;
                uint32_t channel_mask;
                uint8_t subformat[16];
                
                event.reader->read(&valid_bits, sizeof(valid_bits));
                event.reader->read(&channel_mask, sizeof(channel_mask));
                event.reader->read(subformat, sizeof(subformat));
                
                std::cout << "  Valid Bits/Sample: " << valid_bits << "\n";
                std::cout << "  Channel Mask: 0x" << std::hex << channel_mask << std::dec << "\n";
            }
        }
        
        std::cout << "\n";
    }
    
    void handle_data_chunk(const iff::chunk_event& event) {
        std::cout << "Data Chunk:\n";
        std::cout << "  Size: " << event.header.size << " bytes\n";
        
        if (has_format_) {
            uint64_t total_samples = event.header.size / format_.block_align;
            double duration = static_cast<double>(total_samples) / format_.sample_rate;
            
            std::cout << "  Total Samples: " << total_samples << "\n";
            std::cout << "  Duration: " << std::fixed << std::setprecision(2) 
                      << duration << " seconds\n";
            
            // Read first few samples as example
            if (event.header.size > 0) {
                const size_t preview_bytes = std::min<size_t>(64, event.header.size);
                std::vector<uint8_t> preview(preview_bytes);
                event.reader->read(preview.data(), preview_bytes);
                
                std::cout << "  First " << preview_bytes << " bytes (hex): ";
                for (size_t i = 0; i < std::min<size_t>(16, preview_bytes); ++i) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0') 
                              << static_cast<int>(preview[i]) << " ";
                }
                std::cout << std::dec << "\n";
            }
        }
        
        data_size_ = event.header.size;
        std::cout << "\n";
    }
    
    void handle_fact_chunk(const iff::chunk_event& event) {
        std::cout << "Fact Chunk:\n";
        std::cout << "  Size: " << event.header.size << " bytes\n";
        
        if (event.header.size >= 4) {
            uint32_t sample_count;
            event.reader->read(&sample_count, sizeof(sample_count));
            std::cout << "  Sample Count: " << sample_count << "\n";
        }
        
        std::cout << "\n";
    }
    
    void print_summary() {
        std::cout << "=====================================\n";
        std::cout << "Summary:\n";
        
        if (has_format_) {
            std::cout << "  Format: " << get_format_name(format_.format_tag) << "\n";
            std::cout << "  " << format_.channels << " channel(s), "
                      << format_.sample_rate << " Hz, "
                      << format_.bits_per_sample << " bits\n";
            
            if (data_size_ > 0) {
                uint64_t total_samples = data_size_ / format_.block_align;
                double duration = static_cast<double>(total_samples) / format_.sample_rate;
                std::cout << "  Duration: " << std::fixed << std::setprecision(2) 
                          << duration << " seconds\n";
            }
        } else {
            std::cout << "  No format chunk found\n";
        }
    }
    
    const char* get_format_name(uint16_t format_tag) {
        switch (format_tag) {
            case 0x0001: return "PCM";
            case 0x0003: return "IEEE Float";
            case 0x0006: return "A-law";
            case 0x0007: return "µ-law";
            case 0x0011: return "IMA ADPCM";
            case 0x0016: return "ITU G.723 ADPCM";
            case 0x0031: return "GSM 6.10";
            case 0x0040: return "ITU G.721 ADPCM";
            case 0x0050: return "MPEG";
            case 0x0055: return "MP3";
            case 0xFFFE: return "Extensible";
            default: return "Unknown";
        }
    }
    
    WaveFormat format_{};
    bool has_format_ = false;
    uint64_t data_size_ = 0;
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <wav_file>\n";
        std::cout << "\n";
        std::cout << "This example parses a WAV file and displays:\n";
        std::cout << "  - Audio format information\n";
        std::cout << "  - Sample rate and bit depth\n";
        std::cout << "  - Duration and data size\n";
        std::cout << "  - First few bytes of audio data\n";
        return 1;
    }
    
    WavParser parser;
    parser.parse(argv[1]);
    
    return 0;
}