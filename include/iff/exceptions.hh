/**
 * @file exceptions.hh
 * @brief Exception classes and throwing macros for IFF library
 * @author Igor
 * @date 14/08/2025
 * 
 * This file defines the exception hierarchy and convenience macros for
 * error handling throughout the IFF library.
 */

#pragma once

#include <stdexcept>
#include <string>
#include <sstream>

namespace iff {
    
    /**
     * @class iff_error
     * @brief Base exception class for all IFF-related errors
     * 
     * All IFF library exceptions derive from this class, making it easy
     * to catch all IFF-specific errors with a single catch block.
     */
    class iff_error : public std::runtime_error {
    public:
        explicit iff_error(const std::string& msg) 
            : std::runtime_error(msg) {}
    };
    
    /**
     * @class io_error
     * @brief Exception for I/O related errors
     * 
     * Thrown when file access, reading, writing, or seeking operations fail.
     */
    class io_error : public iff_error {
    public:
        explicit io_error(const std::string& msg) 
            : iff_error(msg) {}
    };
    
    /**
     * @class parse_error
     * @brief Exception for parsing errors
     * 
     * Thrown when the file format is invalid, data is corrupted,
     * or the file structure violates IFF/RIFF specifications.
     */
    class parse_error : public iff_error {
    public:
        explicit parse_error(const std::string& msg) 
            : iff_error(msg) {}
    };
    
    /**
     * @brief Build error message from variadic arguments
     * @tparam Args Variadic template arguments
     * @param args Arguments to concatenate into error message
     * @return Concatenated error message string
     * 
     * Uses C++17 fold expressions to concatenate all arguments into
     * a single error message string.
     */
    template<typename... Args>
    std::string build_error_msg(Args&&... args) {
        std::ostringstream oss;
        ((oss << args), ...);
        return oss.str();
    }
    
    /**
     * @defgroup ExceptionMacros Exception Throwing Macros
     * @{
     */
    
    /**
     * @def THROW_IO
     * @brief Throw an io_error with formatted message
     * @param ... Variable arguments to format into error message
     */
    #define THROW_IO(...) \
        throw ::iff::io_error(::iff::build_error_msg(__VA_ARGS__))
    
    /**
     * @def THROW_PARSE
     * @brief Throw a parse_error with formatted message
     * @param ... Variable arguments to format into error message
     */
    #define THROW_PARSE(...) \
        throw ::iff::parse_error(::iff::build_error_msg(__VA_ARGS__))
    
    /**
     * @def THROW_IO_IF
     * @brief Conditionally throw an io_error
     * @param condition Condition to check
     * @param ... Variable arguments for error message if condition is true
     */
    #define THROW_IO_IF(condition, ...) \
        do { if (condition) THROW_IO(__VA_ARGS__); } while(0)
    
    /**
     * @def THROW_PARSE_IF
     * @brief Conditionally throw a parse_error
     * @param condition Condition to check
     * @param ... Variable arguments for error message if condition is true
     */
    #define THROW_PARSE_IF(condition, ...) \
        do { if (condition) THROW_PARSE(__VA_ARGS__); } while(0)
    
    /**
     * @def THROW_IO_UNLESS
     * @brief Throw an io_error unless condition is true
     * @param condition Condition that must be true to avoid throwing
     * @param ... Variable arguments for error message if condition is false
     */
    #define THROW_IO_UNLESS(condition, ...) \
        do { if (!(condition)) THROW_IO(__VA_ARGS__); } while(0)
    
    /**
     * @def THROW_PARSE_UNLESS
     * @brief Throw a parse_error unless condition is true
     * @param condition Condition that must be true to avoid throwing
     * @param ... Variable arguments for error message if condition is false
     */
    #define THROW_PARSE_UNLESS(condition, ...) \
        do { if (!(condition)) THROW_PARSE(__VA_ARGS__); } while(0)
    
    /** @} */ // end of ExceptionMacros group
    
} // namespace iff