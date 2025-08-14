cmake_minimum_required(VERSION 3.20)
project(libiff)

set(CMAKE_CXX_STANDARD 17)

# Build options
option(LIBIFF_BUILD_SHARED "Build LIBIFF as a shared library" ON)
option(LIBIFF_BUILD_FPIC "Build LIBIFF with position independent code (for static builds)" OFF)
option(LIBIFF_BUILD_DOCUMENTATION "Build documentation using Doxygen" OFF)
option(LIBIFF_BUILD_EXAMPLES "Build example programs" OFF)


# Output binary to predictable location.
set(BINARY_OUT_DIR ${CMAKE_BINARY_DIR}/bin)
set(LIB_OUT_DIR ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIB_OUT_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BINARY_OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BINARY_OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${LIB_OUT_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BINARY_OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BINARY_OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${LIB_OUT_DIR})
# ===========================================================================
set (LIBIFF_PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(ext)
add_subdirectory(src/libiff)
add_subdirectory(unittest)

# Examples
if(LIBIFF_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Documentation
if(LIBIFF_BUILD_DOCUMENTATION)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        # Set input and output files
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        # Configure the file
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        message("Doxygen build started")

        # Add custom target for documentation
        add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM
        )

        # Optionally make documentation build with 'make all'
        # add_custom_target(doc ALL ...)
    else()
        message("Doxygen need to be installed to generate the documentation")
    endif()
endif()

