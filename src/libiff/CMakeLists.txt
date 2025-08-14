if (LIBIFF_BUILD_FPIC)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if (LIBIFF_BUILD_SHARED)
    set(LIB_BUILD_TYPE SHARED)
else()
    set(LIB_BUILD_TYPE STATIC)
endif()


add_library(iff ${LIB_BUILD_TYPE}
        parser.cc
        handler_registry.cc
        chunk_reader.cc
        input.cc
        input.hh
        iff85_chunk_reader.cc
        iff85_chunk_reader.hh
        riff_chunk_reader.cc
        riff_chunk_reader.hh
        chunk_iterator.cc
        iff85_chunk_iterator.cc
        iff85_chunk_iterator.hh
        riff_chunk_iterator.cc
        riff_chunk_iterator.hh
        ${LIBIFF_PROJECT_ROOT}/include/iff/compiler.hh
        ${LIBIFF_PROJECT_ROOT}/include/iff/endian.hh
        ${LIBIFF_PROJECT_ROOT}/include/iff/fourcc.hh
        ${LIBIFF_PROJECT_ROOT}/include/iff/parser.hh
        ${LIBIFF_PROJECT_ROOT}/include/iff/byte_order.hh
        ${LIBIFF_PROJECT_ROOT}/include/iff/exceptions.hh
        ${LIBIFF_PROJECT_ROOT}/include/iff/chunk_iterator.hh
        ${LIBIFF_PROJECT_ROOT}/include/iff/chunk_reader.hh
        ${LIBIFF_PROJECT_ROOT}/include/iff/chunk_header.hh
        ${LIBIFF_PROJECT_ROOT}/include/iff/handler_registry.hh
        ${LIBIFF_PROJECT_ROOT}/include/iff/parse_options.hh

)

target_link_libraries(iff
        PUBLIC )
target_include_directories(iff
        PUBLIC
        ${LIBIFF_PROJECT_ROOT}/include/
        ${CMAKE_CURRENT_BINARY_DIR}
        PRIVATE
        ${CMAKE_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}

)
include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)

# Generate configuration header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/iff_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/iff/iff_config.h
)


set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

include(GenerateExportHeader)
set(export_file_name "export_iff.h")

generate_export_header(iff
    EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/iff/${export_file_name}
)
