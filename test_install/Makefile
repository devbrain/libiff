# Makefile for testing libiff installation

# Installation directory
INSTALL_PREFIX = ../install_test

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -I$(INSTALL_PREFIX)/include
LDFLAGS = -L$(INSTALL_PREFIX)/lib -Wl,-rpath,$(INSTALL_PREFIX)/lib
LIBS = -liff

# Target
TARGET = test_installed_make

# Source
SOURCES = test_installed.cpp

# Build rule
$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS) $(LIBS)

# Test with pkg-config
test_pkgconfig: $(SOURCES)
	PKG_CONFIG_PATH=$(INSTALL_PREFIX)/lib/pkgconfig:$$PKG_CONFIG_PATH \
	$(CXX) $(SOURCES) -o test_pkgconfig `pkg-config --cflags --libs libiff`

# Run test
run: $(TARGET)
	./$(TARGET)

# Clean
clean:
	rm -f $(TARGET) test_pkgconfig test_cmake

.PHONY: run clean test_pkgconfig