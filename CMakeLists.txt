cmake_minimum_required(VERSION 3.20)
project(libiff 
    VERSION 1.0.0
    DESCRIPTION "Modern C++ library for parsing IFF and RIFF files"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Include GNUInstallDirs for standard install directories
include(GNUInstallDirs)

# Build options
option(LIBIFF_BUILD_SHARED "Build LIBIFF as a shared library" ON)
option(LIBIFF_BUILD_FPIC "Build LIBIFF with position independent code (for static builds)" OFF)
option(LIBIFF_BUILD_DOCUMENTATION "Build documentation using Doxygen" OFF)
option(LIBIFF_BUILD_EXAMPLES "Build example programs" OFF)
option(LIBIFF_BUILD_TESTS "Build unittest" OFF)


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

if (LIBIFF_BUILD_TESTS)
    add_subdirectory(unittest)
endif ()

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

# ===========================================================================
# Installation and Packaging
# ===========================================================================

# Install headers
install(DIRECTORY include/iff
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hh" PATTERN "*.h"
)

# Create and install CMake package configuration
include(CMakePackageConfigHelpers)

# Generate the config file
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/libiffConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/libiffConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libiff
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

# Generate the version file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/libiffConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the config and version files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/libiffConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/libiffConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libiff
)

# Export targets
install(EXPORT libiffTargets
    FILE libiffTargets.cmake
    NAMESPACE iff::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libiff
)

# Generate and install pkg-config file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/libiff.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/libiff.pc
    @ONLY
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/libiff.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# ===========================================================================
# CPack Configuration
# ===========================================================================

set(CPACK_PACKAGE_NAME "libiff")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VENDOR "libiff")
set(CPACK_PACKAGE_CONTACT "your.email@example.com")

# Source package
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_SOURCE_IGNORE_FILES
    "/build/"
    "/.git/"
    "/.idea/"
    "/.vscode/"
    "/cmake-build-.*/"
    ".*~$"
)

# Binary packages
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
    set(CPACK_GENERATOR "TGZ")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    
    # DEB specific
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})
    set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.27), libstdc++6 (>= 7)")
    
    # RPM specific
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
    set(CPACK_RPM_PACKAGE_REQUIRES "glibc >= 2.27")
endif()

include(CPack)

# ===========================================================================
# Build Configuration Summary
# ===========================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "libiff ${PROJECT_VERSION} Build Configuration:")
message(STATUS "========================================")
message(STATUS "  C++ Standard:           ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:             ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install Prefix:         ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Build Options:")
message(STATUS "  Shared Library:         ${LIBIFF_BUILD_SHARED}")
message(STATUS "  Position Independent:   ${LIBIFF_BUILD_FPIC}")
message(STATUS "  Documentation:          ${LIBIFF_BUILD_DOCUMENTATION}")
message(STATUS "  Examples:               ${LIBIFF_BUILD_EXAMPLES}")
message(STATUS "  Tests:                  ${LIBIFF_BUILD_TESTS}")
message(STATUS "")
message(STATUS "Output Directories:")
message(STATUS "  Runtime:                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "  Library:                ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "  Archive:                ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
message(STATUS "========================================")
message(STATUS "")

