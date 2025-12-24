#pragma once

#include <memory>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include "unittest_config.h"

// Utility function to load test files from the generated directory
inline std::unique_ptr<std::istream> load_test(const std::string& name) {
    static std::filesystem::path root(UNITTEST_PATH_TO_GENERATED_FILES);
    auto path = root / name;
    return std::make_unique<std::ifstream>(path, std::ios::binary);
}

// Load test file as byte vector for tests that need raw data
inline std::vector<std::byte> load_test_data(const std::string& name) {
    auto stream = load_test(name);
    if (!stream || !stream->good()) {
        throw std::runtime_error("Cannot open test file: " + name);
    }
    
    // Get file size
    stream->seekg(0, std::ios::end);
    auto size = stream->tellg();
    stream->seekg(0, std::ios::beg);
    
    // Read file
    std::vector<std::byte> data(size);
    stream->read(reinterpret_cast<char*>(data.data()), size);
    
    return data;
}