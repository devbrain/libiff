cmake_minimum_required(VERSION 3.20)

project(libiff
    VERSION 1.0.0
    DESCRIPTION "Modern C++ library for parsing IFF and RIFF files"
    LANGUAGES CXX
)

# ============================================================================
# Neutrino CMake Integration
# ============================================================================

include(FetchContent)

# Fetch neutrino-cmake if not already available
if(NOT DEFINED NEUTRINO_CMAKE_DIR)
    FetchContent_Declare(neutrino_cmake
        GIT_REPOSITORY https://github.com/devbrain/neutrino-cmake.git
        GIT_TAG master
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(neutrino_cmake)
    set(NEUTRINO_CMAKE_DIR "${neutrino_cmake_SOURCE_DIR}/cmake")
endif()

list(APPEND CMAKE_MODULE_PATH "${NEUTRINO_CMAKE_DIR}")
include(NeutrinoInit)

# ============================================================================
# Build Options (using standardized naming)
# ============================================================================

neutrino_define_library_options(libiff)

# ============================================================================
# Dependencies
# ============================================================================

# No external dependencies for core library
# Tests use doctest (fetched when building tests)

# ============================================================================
# Library Target
# ============================================================================

add_subdirectory(src/libiff)

# ============================================================================
# Testing
# ============================================================================

if(NEUTRINO_LIBIFF_BUILD_TESTS)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    include(${NEUTRINO_CMAKE_DIR}/deps/doctest.cmake)
    neutrino_fetch_doctest()

    enable_testing()
    add_subdirectory(test)
endif()

# ============================================================================
# Examples
# ============================================================================

if(NEUTRINO_LIBIFF_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ============================================================================
# Documentation
# ============================================================================

if(NEUTRINO_LIBIFF_BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

        add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )
    else()
        message(WARNING "Doxygen not found, documentation will not be built")
    endif()
endif()

# ============================================================================
# Installation
# ============================================================================

if(NEUTRINO_LIBIFF_INSTALL)
    include(NeutrinoInstall)

    # Install generated headers alongside public headers
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/iff/export_iff.h
        ${CMAKE_CURRENT_BINARY_DIR}/iff/iff_config.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/iff
    )

    neutrino_install_library(iff
        NAMESPACE neutrino::
        COMPATIBILITY SameMajorVersion
    )
endif()

# ============================================================================
# Summary
# ============================================================================

neutrino_is_top_level(_libiff_is_top_level)
if(_libiff_is_top_level)
    neutrino_print_options(libiff)
    message(STATUS "  Shared library: ${NEUTRINO_LIBIFF_BUILD_SHARED}")
    message(STATUS "")
endif()
