#!/bin/bash

# Test installation script for libiff

set -e  # Exit on error

echo "========================================="
echo "libiff Installation Test"
echo "========================================="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
INSTALL_DIR="$SCRIPT_DIR/install_test"
TEST_DIR="$SCRIPT_DIR/test_install"

echo "Directories:"
echo "  Script dir: $SCRIPT_DIR"
echo "  Build dir:  $BUILD_DIR"
echo "  Install dir: $INSTALL_DIR"
echo "  Test dir:   $TEST_DIR"
echo

# Step 1: Clean previous builds
echo -e "${YELLOW}Step 1: Cleaning previous builds...${NC}"
rm -rf "$BUILD_DIR" "$INSTALL_DIR/include" "$INSTALL_DIR/lib" "$INSTALL_DIR/share"
mkdir -p "$BUILD_DIR" "$INSTALL_DIR"
echo -e "${GREEN}✓ Cleaned${NC}"
echo

# Step 2: Configure with CMake
echo -e "${YELLOW}Step 2: Configuring with CMake...${NC}"
cd "$BUILD_DIR"
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DLIBIFF_BUILD_SHARED=ON \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
    ..
echo -e "${GREEN}✓ Configured${NC}"
echo

# Step 3: Build
echo -e "${YELLOW}Step 3: Building libiff...${NC}"
make -j$(nproc)
echo -e "${GREEN}✓ Built${NC}"
echo

# Step 4: Install
echo -e "${YELLOW}Step 4: Installing libiff...${NC}"
make install
echo -e "${GREEN}✓ Installed${NC}"
echo

# Step 5: Check installed files
echo -e "${YELLOW}Step 5: Checking installed files...${NC}"
echo "Headers:"
find "$INSTALL_DIR/include" -type f -name "*.h*" | head -10

echo
echo "Libraries:"
find "$INSTALL_DIR/lib" -type f -name "*.so*" -o -name "*.a" | head -10

echo
echo "CMake files:"
find "$INSTALL_DIR/lib/cmake" -type f 2>/dev/null | head -10

echo
echo "pkg-config files:"
find "$INSTALL_DIR/lib/pkgconfig" -type f 2>/dev/null | head -10

# Check for required headers
REQUIRED_HEADERS=(
    "parser.hh"
    "fourcc.hh"
    "chunk_iterator.hh"
    "chunk_reader.hh"
    "exceptions.hh"
    "parse_options.hh"
    "handler_registry.hh"
    "chunk_header.hh"
    "byte_order.hh"
    "export_iff.h"
    "iff_config.h"
)

echo
echo "Checking required headers:"
MISSING_HEADERS=0
for header in "${REQUIRED_HEADERS[@]}"; do
    if [ -f "$INSTALL_DIR/include/iff/$header" ]; then
        echo -e "  ${GREEN}✓${NC} $header"
    else
        echo -e "  ${RED}✗${NC} $header MISSING!"
        MISSING_HEADERS=$((MISSING_HEADERS + 1))
    fi
done

if [ $MISSING_HEADERS -gt 0 ]; then
    echo -e "${RED}ERROR: $MISSING_HEADERS headers are missing!${NC}"
    exit 1
fi

echo -e "${GREEN}✓ All headers present${NC}"
echo

# Step 6: Build test program with Makefile
echo -e "${YELLOW}Step 6: Building test program with Makefile...${NC}"
cd "$TEST_DIR"
make clean
make
echo -e "${GREEN}✓ Test program built with Makefile${NC}"
echo

# Step 7: Run test program
echo -e "${YELLOW}Step 7: Running test program...${NC}"
./test_installed_make
echo -e "${GREEN}✓ Test program executed successfully${NC}"
echo

# Step 8: Build with CMake find_package
echo -e "${YELLOW}Step 8: Building test with CMake find_package...${NC}"
mkdir -p build_cmake
cd build_cmake
cmake -Dlibiff_DIR="$INSTALL_DIR/lib/cmake/libiff" ..
make
echo -e "${GREEN}✓ Built with CMake find_package${NC}"
echo

# Step 9: Run CMake-built test
echo -e "${YELLOW}Step 9: Running CMake-built test...${NC}"
./test_installed
echo -e "${GREEN}✓ CMake test executed successfully${NC}"
echo

# Step 10: Test pkg-config
echo -e "${YELLOW}Step 10: Testing pkg-config...${NC}"
cd "$TEST_DIR"
export PKG_CONFIG_PATH="$INSTALL_DIR/lib/pkgconfig:$PKG_CONFIG_PATH"

echo "pkg-config version:"
pkg-config --modversion libiff

echo "pkg-config cflags:"
pkg-config --cflags libiff

echo "pkg-config libs:"
pkg-config --libs libiff

make test_pkgconfig
./test_pkgconfig
echo -e "${GREEN}✓ pkg-config test passed${NC}"
echo

# Summary
echo "========================================="
echo -e "${GREEN}✓ ALL TESTS PASSED!${NC}"
echo "========================================="
echo
echo "libiff has been successfully:"
echo "  • Built as a shared library"
echo "  • Installed to $INSTALL_DIR"
echo "  • All headers are present"
echo "  • Library can be linked with Makefile"
echo "  • Library can be found with CMake find_package"
echo "  • Library can be found with pkg-config"
echo "  • Test programs execute correctly"
echo
echo "Installation is verified and working!"