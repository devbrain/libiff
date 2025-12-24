# Packaging Verification Checklist

This document verifies that the CMake packaging has been properly configured for libiff.

## ✅ Completed Tasks

### 1. CMake Package Configuration
- [x] Created `cmake/libiffConfig.cmake.in` - Package configuration template
- [x] Created `cmake/libiffConfigVersion.cmake.in` - Version compatibility file
- [x] Added `include(CMakePackageConfigHelpers)` in main CMakeLists.txt
- [x] Added `configure_package_config_file()` for config generation
- [x] Added `write_basic_package_version_file()` for version file

### 2. Installation Configuration
- [x] Added `include(GNUInstallDirs)` for standard paths
- [x] Configured header installation with `install(DIRECTORY include/iff ...)`
- [x] Configured library installation with `install(TARGETS iff ...)`
- [x] Added target export with `install(EXPORT libiffTargets ...)`
- [x] Set library VERSION and SOVERSION properties
- [x] Added namespace `iff::` for exported targets
- [x] Configured generated headers installation (export_iff.h, iff_config.h)

### 3. pkg-config Support
- [x] Created `libiff.pc.in` template
- [x] Added `configure_file()` to generate libiff.pc
- [x] Added installation of pkg-config file

### 4. CPack Configuration
- [x] Set CPACK_PACKAGE_NAME, VERSION, DESCRIPTION
- [x] Configured source package generation
- [x] Added platform-specific binary package generators
- [x] Configured DEB package settings
- [x] Configured RPM package settings
- [x] Added `include(CPack)` to enable packaging

### 5. Build Configuration Updates
- [x] Added project VERSION in `project()` command
- [x] Updated target include directories with generator expressions
- [x] Used `$<BUILD_INTERFACE:>` and `$<INSTALL_INTERFACE:>` properly

### 6. Documentation
- [x] Created LICENSE file
- [x] Created INSTALL.md with comprehensive installation instructions
- [x] Added usage examples for CMake find_package
- [x] Added usage examples for pkg-config
- [x] Added package creation instructions

### 7. Test Infrastructure
- [x] Created `test_install/test_installed.cpp` - Comprehensive test program
- [x] Created `test_install/CMakeLists.txt` - CMake test build
- [x] Created `test_install/Makefile` - Direct Makefile test
- [x] Created `test_installation.sh` - Automated installation test script

## Installation Paths

When installed, the library files will be placed in:

```
${CMAKE_INSTALL_PREFIX}/
├── include/
│   └── iff/
│       ├── parser.hh
│       ├── fourcc.hh
│       ├── chunk_iterator.hh
│       ├── chunk_reader.hh
│       ├── chunk_header.hh
│       ├── handler_registry.hh
│       ├── parse_options.hh
│       ├── exceptions.hh
│       ├── byte_order.hh
│       ├── endian.hh
│       ├── compiler.hh
│       ├── export_iff.h (generated)
│       └── iff_config.h (generated)
├── lib/
│   ├── libiff.so (or .dylib on macOS, .dll on Windows)
│   ├── libiff.so.1
│   ├── libiff.so.1.0.0
│   ├── cmake/
│   │   └── libiff/
│   │       ├── libiffConfig.cmake
│   │       ├── libiffConfigVersion.cmake
│   │       └── libiffTargets.cmake
│   └── pkgconfig/
│       └── libiff.pc
└── share/
    └── (documentation if built)
```

## Usage After Installation

### CMake (find_package)
```cmake
find_package(libiff REQUIRED)
target_link_libraries(myapp PRIVATE iff::iff)
```

### CMake (FetchContent)
```cmake
include(FetchContent)
FetchContent_Declare(libiff
    GIT_REPOSITORY https://github.com/user/libiff.git
    GIT_TAG main)
FetchContent_MakeAvailable(libiff)
target_link_libraries(myapp PRIVATE iff::iff)
```

### pkg-config
```bash
g++ myapp.cpp `pkg-config --cflags --libs libiff`
```

### Direct compilation
```bash
g++ -I/usr/local/include myapp.cpp -L/usr/local/lib -liff
```

## Building Packages

### Source package
```bash
make package_source
# Creates: libiff-1.0.0.tar.gz
```

### Binary packages
```bash
make package
# Creates platform-specific packages
```

### Debian package
```bash
cpack -G DEB
# Creates: libiff-1.0.0-Linux.deb
```

### RPM package
```bash
cpack -G RPM
# Creates: libiff-1.0.0-Linux.rpm
```

## Testing Installation

To test the installation:

```bash
# 1. Build and install
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/tmp/libiff_test ..
make
make install

# 2. Build test program
cd /path/to/test
export PKG_CONFIG_PATH=/tmp/libiff_test/lib/pkgconfig:$PKG_CONFIG_PATH
g++ test.cpp `pkg-config --cflags --libs libiff` -o test

# 3. Run test
export LD_LIBRARY_PATH=/tmp/libiff_test/lib:$LD_LIBRARY_PATH
./test
```

## Verification Complete

The CMake packaging for libiff is fully configured and ready for use. All necessary files have been created and the installation process has been documented.