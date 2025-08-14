# libiff - Modern C++ IFF/RIFF Parser Library

A clean, iterator-based parser for IFF-85 (Interchange File Format) with planned RIFF support.

## Features

- **Iterator-based parsing** - Clean abstraction for traversing chunks
- **Format auto-detection** - Automatically detects IFF-85 vs RIFF
- **Lazy evaluation** - Chunks are read on-demand
- **Proper padding handling** - Format-specific alignment handled transparently
- **Modern C++17** - Clean, idiomatic API

## Quick Start

### Simple Functional API (90% of use cases)

```cpp
#include <iff/simple_parser.hh>

// Process all data chunks in a file
iff::for_each_chunk(stream, [](const auto& chunk) {
    if (chunk.header.id == "DATA"_4cc) {
        auto data = chunk.reader->read_all();
        // Process data...
    }
});
```

### Event-Driven API (When you need multiple handlers)

```cpp
#include <iff/parser.hh>

// Set up handlers for specific chunks
iff::handler_registry handlers;

handlers.on_chunk("COMM"_4cc, [](const chunk_event& e) {
    if (e.type == chunk_event_type::begin) {
        // Read audio format info
    }
});

handlers.on_chunk("SSND"_4cc, [](const chunk_event& e) {
    if (e.type == chunk_event_type::begin) {
        // Process audio data
    }
});

// Parse with handlers
iff::parse(stream, handlers);
```

### Direct Iterator Usage (Advanced)

```cpp
#include <iff/chunk_iterator.hh>

// Create iterator (auto-detects format)
auto it = chunk_iterator::get_iterator(stream);

// Iterate through all chunks (depth-first)
while (it->has_next()) {
    const auto& chunk = it->current();
    
    if (!chunk.header.is_container) {
        // Process data chunk
        auto data = chunk.reader->read_all();
    }
    
    it->next();
}
```

## Architecture

The library uses a clean layered architecture:

1. **Iterator Layer** (`chunk_iterator`)
   - Abstract interface with format-specific implementations
   - Handles depth-first traversal and container boundaries
   - Manages padding and alignment

2. **Reader Layer** (`chunk_reader`)  
   - Abstract interface for reading chunk data
   - Format-specific padding handling
   - Provides stream access for external parsers

3. **API Layer**
   - `for_each_chunk()` - Simple functional interface
   - Handler registry - Event-driven processing

## Supported Formats

- **IFF-85** ✅ Full support with proper 2-byte padding
- **RIFF** 🚧 Planned (currently throws not_implemented_error)
- **RF64** 🚧 Planned for large file support

## Building

```bash
mkdir build && cd build
cmake ..
make
make test
```

## Dependencies

- C++17 compiler
- CMake 3.15+
- doctest (included as submodule)

## Examples

### AIFF Audio File Parser

```cpp
void parse_aiff(std::istream& stream) {
    iff::for_each_chunk(stream, [](const auto& chunk) {
        switch (chunk.header.id.value()) {
            case "COMM"_4cc.value():
                // Read common chunk (sample rate, channels, etc)
                break;
            case "SSND"_4cc.value():
                // Read sound data
                break;
        }
    });
}
```

### Find All Chunks of Specific Type

```cpp
std::vector<std::vector<std::byte>> find_all_data_chunks(std::istream& stream) {
    std::vector<std::vector<std::byte>> results;
    
    iff::for_each_chunk(stream, [&](const auto& chunk) {
        if (chunk.header.id == "DATA"_4cc) {
            results.push_back(chunk.reader->read_all());
        }
    });
    
    return results;
}
```

## License

See LICENSE file for details.