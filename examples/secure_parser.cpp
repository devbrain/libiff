/**
 * @file secure_parser.cpp
 * @brief Demonstrates security-hardened parsing of untrusted files
 * 
 * This example shows how to safely parse potentially malicious or
 * corrupted IFF/RIFF files using security features and error handling.
 */

#include <iff/parser.hh>
#include <iff/chunk_iterator.hh>
#include <iff/parse_options.hh>
#include <iff/exceptions.hh>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <atomic>
#include <thread>
#include <csignal>
#include <sstream>
#include <algorithm>

class SecureParser {
public:
    struct SecurityViolation {
        enum Type {
            SIZE_LIMIT_EXCEEDED,
            DEPTH_LIMIT_EXCEEDED,
            INVALID_CHUNK_SIZE,
            TRUNCATED_FILE,
            CIRCULAR_REFERENCE,
            MEMORY_EXHAUSTION,
            TIMEOUT
        };
        
        Type type;
        uint64_t offset;
        std::string description;
        std::string chunk_id;
    };
    
    struct ParseResult {
        bool success = false;
        int chunks_parsed = 0;
        int warnings_count = 0;
        int errors_count = 0;
        std::vector<SecurityViolation> violations;
        std::chrono::milliseconds parse_time;
        uint64_t max_memory_used = 0;
    };
    
    ParseResult parse_untrusted(const std::string& filename,
                                uint64_t max_chunk_size = 100 * 1024 * 1024,  // 100MB
                                int max_depth = 10,
                                int timeout_seconds = 30) {
        ParseResult result;
        auto start_time = std::chrono::steady_clock::now();
        
        std::cout << "Secure Parsing: " << filename << "\n";
        std::cout << "=========================================\n";
        std::cout << "Security Settings:\n";
        std::cout << "  Max chunk size: " << format_size(max_chunk_size) << "\n";
        std::cout << "  Max nesting depth: " << max_depth << "\n";
        std::cout << "  Timeout: " << timeout_seconds << " seconds\n";
        std::cout << "\n";
        
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file: " << filename << "\n";
            result.errors_count++;
            return result;
        }
        
        // Get file size for validation
        file.seekg(0, std::ios::end);
        uint64_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::cout << "File size: " << format_size(file_size) << "\n\n";
        
        // Configure parse options with strict security settings
        iff::parse_options options;
        options.strict = false;  // Continue on errors to find all issues
        options.max_chunk_size = max_chunk_size;
        options.max_depth = max_depth;
        options.allow_rf64 = true;  // Allow but validate RF64
        
        // Warning handler to track security issues
        options.on_warning = [&result, this](uint64_t offset,
                                            std::string_view category,
                                            std::string_view message) {
            result.warnings_count++;
            
            SecurityViolation violation;
            violation.offset = offset;
            violation.description = std::string(message);
            
            // Categorize the warning
            if (category == "size_limit") {
                violation.type = SecurityViolation::SIZE_LIMIT_EXCEEDED;
                std::cout << "⚠️  Size limit exceeded at offset 0x" 
                          << std::hex << offset << std::dec << "\n";
            } else if (category == "depth_limit") {
                violation.type = SecurityViolation::DEPTH_LIMIT_EXCEEDED;
                std::cout << "⚠️  Depth limit exceeded at offset 0x"
                          << std::hex << offset << std::dec << "\n";
            } else if (category == "truncated") {
                violation.type = SecurityViolation::TRUNCATED_FILE;
                std::cout << "⚠️  Truncated data at offset 0x"
                          << std::hex << offset << std::dec << "\n";
            } else {
                std::cout << "⚠️  Warning [" << category << "] at offset 0x"
                          << std::hex << offset << std::dec 
                          << ": " << message << "\n";
            }
            
            result.violations.push_back(violation);
        };
        
        // Set up timeout mechanism
        std::atomic<bool> timeout_flag(false);
        std::thread timeout_thread([&timeout_flag, timeout_seconds]() {
            std::this_thread::sleep_for(std::chrono::seconds(timeout_seconds));
            timeout_flag = true;
        });
        timeout_thread.detach();
        
        // Track memory usage
        size_t peak_memory = 0;
        
        try {
            std::cout << "Starting parse...\n";
            std::cout << "----------------------------------------\n";
            
            // Create iterator with security options
            auto it = iff::chunk_iterator::get_iterator(file, options);
            
            // Validate chunks
            while (it->has_next() && !timeout_flag) {
                const auto& chunk = it->current();
                
                // Validate chunk
                if (!validate_chunk(chunk, file_size, result)) {
                    result.errors_count++;
                }
                
                // Track statistics
                result.chunks_parsed++;
                
                // Simulate memory tracking
                size_t estimated_memory = sizeof(chunk) + chunk.header.size;
                if (estimated_memory > peak_memory) {
                    peak_memory = estimated_memory;
                }
                
                // Display progress for large files
                if (result.chunks_parsed % 100 == 0) {
                    std::cout << "  Processed " << result.chunks_parsed 
                              << " chunks...\n";
                }
                
                it->next();
            }
            
            if (timeout_flag) {
                SecurityViolation violation;
                violation.type = SecurityViolation::TIMEOUT;
                violation.description = "Parse timeout exceeded";
                result.violations.push_back(violation);
                std::cout << "❌ Parse timeout!\n";
            } else {
                result.success = true;
                std::cout << "✅ Parse completed successfully\n";
            }
            
        } catch (const iff::format_error& e) {
            std::cout << "❌ Format error: " << e.what() << "\n";
            result.errors_count++;
            
            SecurityViolation violation;
            violation.type = SecurityViolation::INVALID_CHUNK_SIZE;
            violation.description = e.what();
            result.violations.push_back(violation);
            
        } catch (const iff::io_error& e) {
            std::cout << "❌ I/O error: " << e.what() << "\n";
            result.errors_count++;
            
            SecurityViolation violation;
            violation.type = SecurityViolation::TRUNCATED_FILE;
            violation.description = e.what();
            result.violations.push_back(violation);
            
        } catch (const std::bad_alloc& e) {
            std::cout << "❌ Memory exhaustion: " << e.what() << "\n";
            result.errors_count++;
            
            SecurityViolation violation;
            violation.type = SecurityViolation::MEMORY_EXHAUSTION;
            violation.description = "Failed to allocate memory";
            result.violations.push_back(violation);
            
        } catch (const std::exception& e) {
            std::cout << "❌ Unexpected error: " << e.what() << "\n";
            result.errors_count++;
        }
        
        // Calculate parse time
        auto end_time = std::chrono::steady_clock::now();
        result.parse_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        result.max_memory_used = peak_memory;
        
        // Print results
        print_results(result);
        
        return result;
    }
    
private:
    bool validate_chunk(const iff::chunk_iterator::chunk_info& chunk,
                       uint64_t file_size,
                       ParseResult& result) {
        bool valid = true;
        
        // Check if chunk size exceeds file size
        if (chunk.header.file_offset + chunk.header.size + 8 > file_size) {
            std::cout << "⚠️  Chunk '" << chunk.header.id.to_string() 
                      << "' size exceeds file boundary\n";
            
            SecurityViolation violation;
            violation.type = SecurityViolation::INVALID_CHUNK_SIZE;
            violation.offset = chunk.header.file_offset;
            violation.chunk_id = chunk.header.id.to_string();
            violation.description = "Chunk size exceeds file size";
            result.violations.push_back(violation);
            
            valid = false;
        }
        
        // Check for suspiciously large chunks
        if (chunk.header.size > 1ULL << 30) {  // > 1GB
            std::cout << "⚠️  Suspiciously large chunk '" 
                      << chunk.header.id.to_string() 
                      << "': " << format_size(chunk.header.size) << "\n";
        }
        
        // Check for zero-size containers (could indicate corruption)
        if (chunk.header.is_container && chunk.header.size == 0) {
            std::cout << "⚠️  Zero-size container '" 
                      << chunk.header.id.to_string() << "'\n";
        }
        
        // Check chunk ID for validity (should be printable ASCII)
        std::string id = chunk.header.id.to_string();
        for (char c : id) {
            if (c < 32 || c > 126) {
                std::cout << "⚠️  Non-printable character in chunk ID at offset 0x"
                          << std::hex << chunk.header.file_offset << std::dec << "\n";
                valid = false;
                break;
            }
        }
        
        return valid;
    }
    
    void print_results(const ParseResult& result) {
        std::cout << "\n";
        std::cout << "Security Analysis Results:\n";
        std::cout << "=========================================\n";
        
        if (result.success) {
            std::cout << "✅ File appears to be safe to parse\n";
        } else {
            std::cout << "❌ File may be corrupted or malicious\n";
        }
        
        std::cout << "\nStatistics:\n";
        std::cout << "  Chunks parsed: " << result.chunks_parsed << "\n";
        std::cout << "  Warnings: " << result.warnings_count << "\n";
        std::cout << "  Errors: " << result.errors_count << "\n";
        std::cout << "  Parse time: " << result.parse_time.count() << " ms\n";
        std::cout << "  Peak memory: " << format_size(result.max_memory_used) << "\n";
        
        if (!result.violations.empty()) {
            std::cout << "\nSecurity Violations:\n";
            std::cout << "-------------------\n";
            
            for (const auto& violation : result.violations) {
                std::cout << "  • ";
                
                switch (violation.type) {
                    case SecurityViolation::SIZE_LIMIT_EXCEEDED:
                        std::cout << "Size limit exceeded";
                        break;
                    case SecurityViolation::DEPTH_LIMIT_EXCEEDED:
                        std::cout << "Nesting depth exceeded";
                        break;
                    case SecurityViolation::INVALID_CHUNK_SIZE:
                        std::cout << "Invalid chunk size";
                        break;
                    case SecurityViolation::TRUNCATED_FILE:
                        std::cout << "Truncated file";
                        break;
                    case SecurityViolation::CIRCULAR_REFERENCE:
                        std::cout << "Circular reference detected";
                        break;
                    case SecurityViolation::MEMORY_EXHAUSTION:
                        std::cout << "Memory exhaustion";
                        break;
                    case SecurityViolation::TIMEOUT:
                        std::cout << "Parse timeout";
                        break;
                }
                
                if (!violation.chunk_id.empty()) {
                    std::cout << " in chunk '" << violation.chunk_id << "'";
                }
                
                if (violation.offset > 0) {
                    std::cout << " at offset 0x" << std::hex << violation.offset 
                              << std::dec;
                }
                
                if (!violation.description.empty()) {
                    std::cout << "\n    " << violation.description;
                }
                
                std::cout << "\n";
            }
        }
        
        // Recommendations
        std::cout << "\nRecommendations:\n";
        std::cout << "----------------\n";
        
        if (result.violations.empty() && result.errors_count == 0) {
            std::cout << "  ✅ File appears safe for normal parsing\n";
        } else {
            if (result.errors_count > 0) {
                std::cout << "  ⚠️  Handle this file with caution\n";
                std::cout << "  ⚠️  Enable strict parsing mode for production\n";
            }
            
            if (result.warnings_count > 5) {
                std::cout << "  ⚠️  Consider validating file source\n";
            }
            
            for (const auto& violation : result.violations) {
                if (violation.type == SecurityViolation::SIZE_LIMIT_EXCEEDED) {
                    std::cout << "  ⚠️  Increase chunk size limit if file is trusted\n";
                    break;
                }
            }
            
            for (const auto& violation : result.violations) {
                if (violation.type == SecurityViolation::DEPTH_LIMIT_EXCEEDED) {
                    std::cout << "  ⚠️  Increase depth limit if deep nesting is expected\n";
                    break;
                }
            }
        }
    }
    
    std::string format_size(uint64_t size) {
        if (size < 1024) {
            return std::to_string(size) + " bytes";
        } else if (size < 1024 * 1024) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) 
                << (static_cast<double>(size) / 1024) << " KB";
            return oss.str();
        } else if (size < 1024 * 1024 * 1024) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) 
                << (static_cast<double>(size) / (1024 * 1024)) << " MB";
            return oss.str();
        } else {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) 
                << (static_cast<double>(size) / (1024 * 1024 * 1024)) << " GB";
            return oss.str();
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 5) {
        std::cout << "Usage: " << argv[0] << " <file> [max_chunk_mb] [max_depth] [timeout_sec]\n";
        std::cout << "\n";
        std::cout << "Securely parse potentially malicious IFF/RIFF files.\n";
        std::cout << "\n";
        std::cout << "Parameters:\n";
        std::cout << "  file          File to parse\n";
        std::cout << "  max_chunk_mb  Maximum chunk size in MB (default: 100)\n";
        std::cout << "  max_depth     Maximum nesting depth (default: 10)\n";
        std::cout << "  timeout_sec   Parse timeout in seconds (default: 30)\n";
        std::cout << "\n";
        std::cout << "Examples:\n";
        std::cout << "  " << argv[0] << " untrusted.riff\n";
        std::cout << "    Parse with default security settings\n";
        std::cout << "\n";
        std::cout << "  " << argv[0] << " large.wav 500 20 60\n";
        std::cout << "    Allow 500MB chunks, depth 20, 60 second timeout\n";
        std::cout << "\n";
        std::cout << "Security Features:\n";
        std::cout << "  • Chunk size validation\n";
        std::cout << "  • Nesting depth limits\n";
        std::cout << "  • Parse timeout protection\n";
        std::cout << "  • Memory exhaustion prevention\n";
        std::cout << "  • Truncation detection\n";
        std::cout << "  • Invalid data detection\n";
        return 1;
    }
    
    // Parse command line arguments
    uint64_t max_chunk_mb = 100;
    int max_depth = 10;
    int timeout_sec = 30;
    
    if (argc >= 3) {
        max_chunk_mb = std::stoul(argv[2]);
    }
    if (argc >= 4) {
        max_depth = std::stoi(argv[3]);
    }
    if (argc >= 5) {
        timeout_sec = std::stoi(argv[4]);
    }
    
    SecureParser parser;
    auto result = parser.parse_untrusted(
        argv[1],
        max_chunk_mb * 1024 * 1024,
        max_depth,
        timeout_sec
    );
    
    // Return non-zero exit code if issues found
    return (result.errors_count > 0 || !result.violations.empty()) ? 1 : 0;
}