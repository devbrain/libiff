file(GLOB TEST_DEFINITIONS ${CMAKE_CURRENT_SOURCE_DIR}/test_files_generator/definitions/*.json)
set(PATH_TO_GENERATED_FILES ${CMAKE_CURRENT_BINARY_DIR}/generated)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated/.timestamp
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PATH_TO_GENERATED_FILES}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_files_generator/generate_test_files.py
            ${TEST_DEFINITIONS}
            -o ${PATH_TO_GENERATED_FILES}
    COMMAND ${CMAKE_COMMAND} -E touch ${PATH_TO_GENERATED_FILES}/.timestamp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test_files_generator/generate_test_files.py
            ${TEST_DEFINITIONS}
    COMMENT "Generating test files"
    VERBATIM
)

add_custom_target(generate_test_files ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/generated/.timestamp
)

add_executable(iff_unittest
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
)
target_link_libraries(iff_unittest iff doctest::doctest failsafe)

target_include_directories(iff_unittest
        PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)

add_dependencies(iff_unittest generate_test_files)

# Generate configuration header
configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/unittest_config.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/unittest_config.h
        @ONLY
)
