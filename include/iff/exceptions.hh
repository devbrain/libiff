#pragma once

#include <stdexcept>
#include <string>
#include <cstdint>
#include <failsafe/exception.hh>

namespace iff {
    
    // Base exception for all IFF errors
    class iff_error : public std::runtime_error {
    public:
        explicit iff_error(const std::string& msg) 
            : std::runtime_error(msg) {}
    };
    
    // IO-related errors (file access, read/write failures)
    class io_error : public iff_error {
    public:
        explicit io_error(const std::string& msg) 
            : iff_error(msg) {}
    };
    
    // Parse-related errors (invalid format, corrupt data)
    class parse_error : public iff_error {
    public:
        explicit parse_error(const std::string& msg) 
            : iff_error(msg) {}
    };
    
    // Convenience throwing macros using failsafe
    
    // Throw IO error with message formatting
    #define THROW_IO(...) \
        THROW(::iff::io_error, __VA_ARGS__)
    
    // Throw parse error with message formatting
    #define THROW_PARSE(...) \
        THROW(::iff::parse_error, __VA_ARGS__)
    
    // Conditional throwing
    #define THROW_IO_IF(condition, ...) \
        THROW_IF(condition, ::iff::io_error, __VA_ARGS__)
    
    #define THROW_PARSE_IF(condition, ...) \
        THROW_IF(condition, ::iff::parse_error, __VA_ARGS__)
    
    #define THROW_IO_UNLESS(condition, ...) \
        THROW_UNLESS(condition, ::iff::io_error, __VA_ARGS__)
    
    #define THROW_PARSE_UNLESS(condition, ...) \
        THROW_UNLESS(condition, ::iff::parse_error, __VA_ARGS__)
    
} // namespace iff