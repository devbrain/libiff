cmake_minimum_required(VERSION 3.20)
project(test_libiff)

set(CMAKE_CXX_STANDARD 17)

# Find installed libiff
find_package(libiff REQUIRED)

# Create test executable
add_executable(test_installed test_installed.cpp)

# Link against libiff
target_link_libraries(test_installed PRIVATE iff::iff)

# Print information
message(STATUS "Found libiff version: ${libiff_VERSION}")
message(STATUS "libiff include dirs: ${libiff_INCLUDE_DIRS}")
message(STATUS "libiff libraries: ${libiff_LIBRARIES}")