# libiff

A modern C++ library for parsing IFF (Interchange File Format) and RIFF (Resource Interchange File Format) files.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![CMake](https://img.shields.io/badge/CMake-3.10+-064F8C.svg)](https://cmake.org/)

## Features

- **Multi-format support**: Parse IFF-85, RIFF, RF64, RIFX, and BW64 files
- **Common formats**: Built-in support for WAV, AVI, AIFF, and more
- **Event-driven API**: Register handlers for specific chunk types
- **Iterator interface**: Direct control over chunk traversal
- **Large file support**: Handle files >4GB with RF64/BW64
- **Security focused**: Configurable limits for chunk size and nesting depth
- **Zero dependencies**: Header-only core with no external dependencies
- **Modern C++**: Written in C++17 with clean, idiomatic code
- **Comprehensive testing**: Extensive test suite with edge case coverage

## Quick Example

```cpp
#include <iff/parser.hh>
#include <iostream>
#include <fstream>

int main() {
    std::ifstream file("audio.wav", std::ios::binary);
    
    iff::for_each_chunk(file, [](auto& chunk) {
        std::cout << "Found chunk: " << chunk.header.id 
                  << " (" << chunk.header.size << " bytes)\n";
    });
    
    return 0;
}
```

## Installation

### From Source

```bash
git clone https://github.com/yourusername/libiff.git
cd libiff
mkdir build && cd build
cmake ..
make
sudo make install
```

### CMake Integration

Add to your `CMakeLists.txt`:

```cmake
find_package(libiff REQUIRED)
target_link_libraries(your_target PRIVATE iff::iff)
```

Or use FetchContent:

```cmake
include(FetchContent)
FetchContent_Declare(
    libiff
    GIT_REPOSITORY https://github.com/yourusername/libiff.git
    GIT_TAG main
)
FetchContent_MakeAvailable(libiff)
target_link_libraries(your_target PRIVATE iff::iff)
```

## Usage Examples

### Parse a WAV file

```cpp
#include <iff/parser.hh>
#include <iff/handler_registry.hh>

iff::handler_registry handlers;

// Handle format chunk
handlers.on_chunk_in_form(
    iff::fourcc("WAVE"), 
    iff::fourcc("fmt "),
    [](const iff::chunk_event& event) {
        if (event.type == iff::chunk_event_type::begin) {
            // Read and process format data
            uint16_t format;
            event.reader->read(&format, sizeof(format));
            std::cout << "Audio format: " << format << "\n";
        }
    }
);

std::ifstream file("audio.wav", std::ios::binary);
iff::parse(file, handlers);
```

### Security Options

```cpp
iff::parse_options options;
options.max_chunk_size = 100 * 1024 * 1024;  // 100MB limit
options.max_depth = 10;                       // Max nesting depth
options.strict = false;                       // Continue on errors

iff::parse(file, handlers, options);
```

See [QUICKSTART.md](QUICKSTART.md) for more examples.

## Supported Formats

| Format | Description | Status |
|--------|-------------|--------|
| IFF-85 | Original EA-85 IFF | ✅ Full support |
| RIFF | Resource Interchange File Format | ✅ Full support |
| RF64 | 64-bit RIFF for large files | ✅ Full support |
| RIFX | Big-endian RIFF | ✅ Full support |
| BW64 | Broadcast Wave 64-bit | ✅ Full support |

### Common File Types

- **Audio**: WAV, AIFF, BW64
- **Video**: AVI
- **Images**: WebP (VP8)
- **3D**: Maya IFF
- **Games**: Various game formats using IFF chunks

## API Overview

### Core Components

- **`chunk_iterator`**: Low-level iteration over chunks
- **`parser`**: High-level parsing with event handlers
- **`handler_registry`**: Event handler management
- **`chunk_reader`**: Safe chunk data reading
- **`parse_options`**: Security and behavior configuration

### Key Classes

```cpp
// Parse with handlers
iff::handler_registry handlers;
handlers.on_chunk(chunk_id, handler_function);
iff::parse(stream, handlers);

// Simple iteration
iff::for_each_chunk(stream, [](auto& chunk) {
    // Process chunk
});

// Direct iterator control
auto it = iff::chunk_iterator::get_iterator(stream);
while (it->has_next()) {
    auto& chunk = it->current();
    // Process chunk
    it->next();
}
```

## Building

### Requirements

- C++17 compatible compiler
- CMake 3.10 or higher
- (Optional) Doxygen for documentation

### Build Options

```bash
cmake -DLIBIFF_BUILD_TESTS=ON \
      -DLIBIFF_BUILD_DOCUMENTATION=ON \
      -DCMAKE_BUILD_TYPE=Release \
      ..
```

### Running Tests

```bash
make test
# or
ctest --verbose
```

## Documentation

API documentation can be built with Doxygen:

```bash
cmake -DLIBIFF_BUILD_DOCUMENTATION=ON ..
make doc
```

View documentation at `build/docs/html/index.html`.

## Project Structure

```
libiff/
├── include/iff/        # Public headers
│   ├── parser.hh       # High-level parsing API
│   ├── chunk_iterator.hh
│   ├── handler_registry.hh
│   └── ...
├── src/libiff/         # Implementation
├── unittest/           # Test suite
├── examples/           # Example programs
└── docs/              # Documentation
```

## Contributing

Contributions are welcome! Please feel free to submit pull requests.

### Development

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

### Code Style

- Use modern C++ features (C++17)
- Follow existing code formatting
- Add tests for new functionality
- Update documentation as needed

## Performance

- **Zero-copy reading**: Chunks are read directly from stream
- **Lazy evaluation**: Data is only read when requested
- **Efficient iteration**: Minimal overhead for chunk traversal
- **Memory safe**: No manual memory management required


