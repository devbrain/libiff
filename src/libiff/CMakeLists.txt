# =============================================================================
# libiff library target
# =============================================================================

# Determine library type based on option
if(NEUTRINO_LIBIFF_BUILD_SHARED)
    set(_libiff_lib_type SHARED)
else()
    set(_libiff_lib_type STATIC)
endif()

# Source files
set(LIBIFF_SOURCES
    parser.cc
    handler_registry.cc
    chunk_reader.cc
    input.cc
    iff85_chunk_reader.cc
    riff_chunk_reader.cc
    chunk_iterator.cc
    iff85_chunk_iterator.cc
    riff_chunk_iterator.cc
)

# Private headers
set(LIBIFF_PRIVATE_HEADERS
    input.hh
    iff85_chunk_reader.hh
    riff_chunk_reader.hh
    iff85_chunk_iterator.hh
    riff_chunk_iterator.hh
)

# Public headers
set(LIBIFF_PUBLIC_HEADERS
    ${PROJECT_SOURCE_DIR}/include/iff/compiler.hh
    ${PROJECT_SOURCE_DIR}/include/iff/endian.hh
    ${PROJECT_SOURCE_DIR}/include/iff/fourcc.hh
    ${PROJECT_SOURCE_DIR}/include/iff/parser.hh
    ${PROJECT_SOURCE_DIR}/include/iff/byte_order.hh
    ${PROJECT_SOURCE_DIR}/include/iff/exceptions.hh
    ${PROJECT_SOURCE_DIR}/include/iff/chunk_iterator.hh
    ${PROJECT_SOURCE_DIR}/include/iff/chunk_reader.hh
    ${PROJECT_SOURCE_DIR}/include/iff/chunk_header.hh
    ${PROJECT_SOURCE_DIR}/include/iff/handler_registry.hh
    ${PROJECT_SOURCE_DIR}/include/iff/parse_options.hh
)

# Create library
add_library(iff ${_libiff_lib_type}
    ${LIBIFF_SOURCES}
    ${LIBIFF_PRIVATE_HEADERS}
    ${LIBIFF_PUBLIC_HEADERS}
)

# Create neutrino:: alias
add_library(neutrino::iff ALIAS iff)

# C++ standard
target_compile_features(iff PUBLIC cxx_std_17)

# Include directories
target_include_directories(iff
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# =============================================================================
# Configuration header
# =============================================================================

include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/iff_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/iff/iff_config.h
)

# =============================================================================
# Export header
# =============================================================================

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

include(GenerateExportHeader)
generate_export_header(iff
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/iff/export_iff.h
)

# =============================================================================
# Library properties
# =============================================================================

set_target_properties(iff PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
    FOLDER "Libraries"
)

# =============================================================================
# Compiler Warnings
# =============================================================================

neutrino_target_warnings(iff)
