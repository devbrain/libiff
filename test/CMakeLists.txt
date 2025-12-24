# =============================================================================
# libiff tests
# =============================================================================

# Test file generation
set(TEST_DEFINITIONS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test_files_generator/definitions)
set(PATH_TO_GENERATED_FILES ${CMAKE_CURRENT_BINARY_DIR}/generated)

file(GLOB TEST_DEFINITIONS ${TEST_DEFINITIONS_DIR}/*.json)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/.timestamp
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PATH_TO_GENERATED_FILES}
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/test_files_generator/generate_test_files.py
        ${TEST_DEFINITIONS}
        -o ${PATH_TO_GENERATED_FILES}
    COMMAND ${CMAKE_COMMAND} -E touch ${PATH_TO_GENERATED_FILES}/.timestamp
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/test_files_generator/generate_test_files.py
        ${TEST_DEFINITIONS}
    COMMENT "Generating test files"
    VERBATIM
)

add_custom_target(iff_generate_test_files ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/generated/.timestamp
)

# Test sources
set(TEST_SOURCES
    main.cc
    test_fourcc.cc
    test_parser.cc
    test_iterator.cc
    test_depth_first.cc
    test_factory.cc
    test_simple_parser.cc
    test_generated_files.cc
    test_edge_cases.cc
    test_riff_iterator.cc
    test_security.cc
    test_prop_support.cc
    test_bw64.cc
    test_streaming.cc
    test_warning_callbacks.cc
    test_reader_errors.cc
    test_rf64_edge_cases.cc
)

# Create test executable
add_executable(iff_unittest ${TEST_SOURCES})

target_link_libraries(iff_unittest
    PRIVATE
        iff
        doctest::doctest
)

target_include_directories(iff_unittest
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)

add_dependencies(iff_unittest iff_generate_test_files)

# Generate configuration header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/unittest_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/unittest_config.h
    @ONLY
)

# Register with CTest
include(CTest)
add_test(NAME iff_tests COMMAND iff_unittest)

# Set target properties
set_target_properties(iff_unittest PROPERTIES
    FOLDER "Tests"
)
