cmake_minimum_required(VERSION 3.15)
project(SSSP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === add subdirectory for actual code ===
add_subdirectory(src)

# === tests ===
enable_testing()

set(CTEST_OUTPUT_ON_FAILURE ON)

add_subdirectory(tests)
