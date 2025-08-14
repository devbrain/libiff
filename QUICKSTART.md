# libiff Quickstart Guide

## Overview

libiff is a C++ library for parsing IFF (Interchange File Format) and RIFF (Resource Interchange File Format) files. It supports various formats including WAV, AVI, AIFF, and can handle large files through RF64 support.

## Installation

### Building from source

```bash
mkdir build
cd build
cmake ..
make
sudo make install
```

### CMake integration

```cmake
find_package(libiff REQUIRED)
target_link_libraries(your_target PRIVATE iff::iff)
```

## Basic Usage

### 1. Simple chunk iteration

The simplest way to read an IFF/RIFF file is to iterate through its chunks:

```cpp
#include <iff/parser.hh>
#include <iff/chunk_reader.hh>
#include <iostream>
#include <fstream>

int main() {
    std::ifstream file("audio.wav", std::ios::binary);
    
    iff::for_each_chunk(file, [](auto& chunk) {
        std::cout << "Chunk: " << chunk.header.id 
                  << ", Size: " << chunk.header.size << " bytes\n";
                  
        // Read chunk data if needed
        if (chunk.header.id == iff::fourcc("data")) {
            std::vector<uint8_t> data(chunk.header.size);
            chunk.reader->read(data.data(), data.size());
            // Process audio data...
        }
    });
    
    return 0;
}
```

### 2. Event-driven parsing with handlers

For more complex processing, use the handler registry to respond to specific chunk types:

```cpp
#include <iff/parser.hh>
#include <iff/handler_registry.hh>
#include <iff/chunk_reader.hh>
#include <iostream>
#include <fstream>

int main() {
    std::ifstream file("audio.wav", std::ios::binary);
    
    iff::handler_registry handlers;
    
    // Handle format chunk in WAVE files
    handlers.on_chunk_in_form(
        iff::fourcc("WAVE"),  // Within WAVE form
        iff::fourcc("fmt "),  // Format chunk
        [](const iff::chunk_event& event) {
            if (event.type == iff::chunk_event_type::begin) {
                std::cout << "Found WAVE format chunk\n";
                
                // Read format data
                uint16_t format_tag;
                event.reader->read(&format_tag, sizeof(format_tag));
                std::cout << "Format: " << format_tag << "\n";
            }
        }
    );
    
    // Handle data chunks globally
    handlers.on_chunk(
        iff::fourcc("data"),
        [](const iff::chunk_event& event) {
            if (event.type == iff::chunk_event_type::begin) {
                std::cout << "Data chunk size: " << event.header.size << "\n";
            }
        }
    );
    
    // Parse the file
    iff::parse(file, handlers);
    
    return 0;
}
```

### 3. Security and parse options

Control parsing behavior and set security limits:

```cpp
#include <iff/parser.hh>
#include <iff/parse_options.hh>
#include <iostream>
#include <fstream>

int main() {
    std::ifstream file("untrusted.riff", std::ios::binary);
    
    iff::parse_options options;
    options.max_chunk_size = 100 * 1024 * 1024;  // 100MB limit
    options.max_depth = 10;                       // Max nesting depth
    options.strict = false;                       // Continue on errors
    
    // Set warning handler
    options.on_warning = [](uint64_t offset, 
                           std::string_view category,
                           std::string_view message) {
        std::cerr << "Warning at " << offset << " [" << category 
                  << "]: " << message << "\n";
    };
    
    // Parse with security options
    iff::for_each_chunk(file, [](auto& chunk) {
        std::cout << "Chunk: " << chunk.header.id << "\n";
    }, options);
    
    return 0;
}
```

### 4. Working with chunk readers

Read chunk data efficiently:

```cpp
#include <iff/parser.hh>
#include <iff/chunk_reader.hh>
#include <vector>

void process_chunk(iff::chunk_reader& reader, size_t size) {
    // Read all at once
    std::vector<uint8_t> data(size);
    reader.read(data.data(), size);
    
    // Or read in chunks
    const size_t buffer_size = 4096;
    std::vector<uint8_t> buffer(buffer_size);
    size_t remaining = size;
    
    while (remaining > 0) {
        size_t to_read = std::min(buffer_size, remaining);
        reader.read(buffer.data(), to_read);
        // Process buffer...
        remaining -= to_read;
    }
    
    // Skip data
    reader.skip(1024);  // Skip 1KB
    
    // Get current position
    auto pos = reader.tell();
}
```

### 5. Advanced: Direct iterator usage

For maximum control, use chunk iterators directly:

```cpp
#include <iff/chunk_iterator.hh>
#include <iff/parse_options.hh>
#include <fstream>

int main() {
    std::ifstream file("media.avi", std::ios::binary);
    
    iff::parse_options options;
    options.max_chunk_size = 1ULL << 33;  // 8GB for video files
    
    auto iterator = iff::chunk_iterator::get_iterator(file, options);
    
    while (iterator->has_next()) {
        auto& chunk = iterator->current();
        
        if (chunk.header.is_container) {
            std::cout << "Container: " << chunk.header.id;
            if (chunk.header.type) {
                std::cout << " Type: " << *chunk.header.type;
            }
            std::cout << "\n";
        } else {
            std::cout << "  Chunk: " << chunk.header.id 
                      << " at offset " << chunk.header.file_offset << "\n";
        }
        
        iterator->next();
    }
    
    return 0;
}
```

## Common File Formats

### WAV Files
```cpp
// WAV files are RIFF with WAVE form type
handlers.on_chunk_in_form(iff::fourcc("WAVE"), iff::fourcc("fmt "), ...);
handlers.on_chunk_in_form(iff::fourcc("WAVE"), iff::fourcc("data"), ...);
```

### AVI Files
```cpp
// AVI files are RIFF with AVI form type
handlers.on_chunk_in_form(iff::fourcc("AVI "), iff::fourcc("hdrl"), ...);
handlers.on_chunk_in_container(iff::fourcc("movi"), iff::fourcc("00dc"), ...);
```

### AIFF Files
```cpp
// AIFF files use big-endian IFF-85 format
handlers.on_chunk_in_form(iff::fourcc("AIFF"), iff::fourcc("COMM"), ...);
handlers.on_chunk_in_form(iff::fourcc("AIFF"), iff::fourcc("SSND"), ...);
```

## Error Handling

The library throws exceptions for I/O errors and format violations:

```cpp
#include <iff/exceptions.hh>

try {
    iff::parse(file, handlers);
} catch (const iff::io_error& e) {
    std::cerr << "I/O error: " << e.what() << "\n";
} catch (const iff::format_error& e) {
    std::cerr << "Format error: " << e.what() << "\n";
} catch (const iff::iff_error& e) {
    std::cerr << "IFF error: " << e.what() << "\n";
}
```

## Best Practices

1. **Always use binary mode**: Open files with `std::ios::binary`
2. **Set appropriate limits**: Configure `max_chunk_size` for your use case
3. **Handle containers properly**: Containers (FORM, LIST) provide structure but don't contain data directly
4. **Check chunk IDs carefully**: Use `iff::fourcc` for safe comparison
5. **Use parse options for untrusted input**: Set strict limits and warning handlers

## Building Documentation

To build the API documentation:

```bash
cmake -DLIBIFF_BUILD_DOCUMENTATION=ON ..
make doc
```

Documentation will be generated in `build/docs/html/`.

## Next Steps

- Check the `examples/` directory for complete programs
- Review `unittest/` for advanced usage patterns