# Installation Guide for libiff

## Prerequisites

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.20 or higher
- (Optional) Doxygen for documentation
- (Optional) pkg-config for easier library discovery

## Basic Installation

### From Source

```bash
# Clone the repository
git clone https://github.com/yourusername/libiff.git
cd libiff

# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
make -j$(nproc)

# Run tests (optional)
make test

# Install (may require sudo)
sudo make install
```

### Installation Locations

By default, the library installs to:
- Headers: `/usr/local/include/iff/`
- Library: `/usr/local/lib/`
- CMake config: `/usr/local/lib/cmake/libiff/`
- pkg-config: `/usr/local/lib/pkgconfig/`

### Custom Installation Prefix

To install to a custom location:

```bash
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install ..
make install
```

## Build Options

Configure the build with these CMake options:

```bash
cmake [options] ..
```

Available options:

| Option | Default | Description |
|--------|---------|-------------|
| `-DLIBIFF_BUILD_SHARED=ON/OFF` | ON | Build as shared library |
| `-DLIBIFF_BUILD_FPIC=ON/OFF` | OFF | Build with position independent code |
| `-DLIBIFF_BUILD_DOCUMENTATION=ON/OFF` | OFF | Build Doxygen documentation |
| `-DLIBIFF_BUILD_EXAMPLES=ON/OFF` | OFF | Build example programs |
| `-DCMAKE_BUILD_TYPE=Debug/Release` | Release | Build configuration |

Example with multiple options:

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DLIBIFF_BUILD_EXAMPLES=ON \
      -DLIBIFF_BUILD_DOCUMENTATION=ON \
      ..
```

## Using libiff in Your Project

### Method 1: CMake find_package

After installation, use libiff in your CMake project:

```cmake
find_package(libiff REQUIRED)
target_link_libraries(your_target PRIVATE iff::iff)
```

### Method 2: CMake FetchContent

Include libiff directly without installation:

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

### Method 3: pkg-config

If pkg-config is available:

```bash
g++ -std=c++17 your_file.cpp `pkg-config --cflags --libs libiff`
```

Or in a Makefile:

```makefile
CXXFLAGS += $(shell pkg-config --cflags libiff)
LDFLAGS += $(shell pkg-config --libs libiff)
```

### Method 4: Manual Compilation

```bash
g++ -std=c++17 -I/usr/local/include your_file.cpp -L/usr/local/lib -liff
```

## Creating Packages

### Source Package

```bash
make package_source
```

Creates: `libiff-1.0.0.tar.gz`

### Binary Packages

```bash
make package
```

Creates platform-specific packages:
- Linux: `.tar.gz`, `.deb`, `.rpm`
- macOS: `.tar.gz`
- Windows: `.zip`

### Debian Package

For Debian/Ubuntu systems:

```bash
# Configure with Release build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
cpack -G DEB

# Install the generated package
sudo dpkg -i libiff-1.0.0-Linux.deb
```

### RPM Package

For Red Hat/Fedora systems:

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
cpack -G RPM

# Install the generated package
sudo rpm -i libiff-1.0.0-Linux.rpm
```

## Uninstallation

If you installed from source:

```bash
cd build
sudo make uninstall
```

For package installations:
- Debian/Ubuntu: `sudo apt remove libiff`
- Red Hat/Fedora: `sudo yum remove libiff`

## Troubleshooting

### CMake Can't Find libiff

Make sure the installation directory is in CMake's search path:

```bash
cmake -Dlibiff_DIR=/usr/local/lib/cmake/libiff ..
```

### Library Not Found at Runtime

Add the library path to your system:

```bash
# Temporary
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Permanent (Linux)
echo '/usr/local/lib' | sudo tee /etc/ld.so.conf.d/libiff.conf
sudo ldconfig
```

### Permission Denied During Installation

Use sudo for system-wide installation or specify a user-writable prefix:

```bash
cmake -DCMAKE_INSTALL_PREFIX=$HOME/.local ..
make install
```

## Platform-Specific Notes

### Windows

On Windows with MSVC:
```cmd
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
cmake --install . --config Release
```

### macOS

On macOS, you might need to specify the SDK:
```bash
cmake -DCMAKE_OSX_SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk ..
```

## Verification

After installation, verify with:

```bash
# Check if headers are installed
ls /usr/local/include/iff/

# Check if library is installed
ls /usr/local/lib/libiff*

# Check pkg-config
pkg-config --modversion libiff

# Compile a test program
echo '#include <iff/parser.hh>
int main() { return 0; }' > test.cpp
g++ -std=c++17 test.cpp -liff -o test && echo "Success!"
```