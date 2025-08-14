# Examples for libiff

# Helper function to add an example
function(add_example name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE iff)
    
    # Add filesystem library if needed (for chunk_extractor)
    if(${name} STREQUAL "chunk_extractor")
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")
            target_link_libraries(${name} PRIVATE stdc++fs)
        endif()
    endif()
endfunction()

# Add examples
add_example(simple_example)
add_example(wav_parser)
add_example(aiff_parser)
add_example(structure_analyzer)
add_example(chunk_extractor)
add_example(secure_parser)