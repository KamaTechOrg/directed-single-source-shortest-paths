cmake_minimum_required(VERSION 3.15)
project(SSSP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


include(FetchContent)


set(LEMON_ENABLE_GLPK   OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CPLEX  OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CLP    OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CBC    OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_SOPLEX OFF CACHE BOOL "" FORCE)



FetchContent_Declare(lemon
  GIT_REPOSITORY https://github.com/alex-176/lemon.git
  GIT_TAG        main
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(lemon)




include(FetchContent)
set(BENCHMARK_ENABLE_TESTING        OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS    OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_EXAMPLES       OFF CACHE BOOL "" FORCE)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR         OFF CACHE BOOL "" FORCE)

FetchContent_Declare(benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.8.3
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(benchmark)


add_subdirectory(tools/rb2lemon)

add_subdirectory(src)

add_subdirectory(benchmarks)



enable_testing()
set(CTEST_OUTPUT_ON_FAILURE ON)

add_subdirectory(tests)


# UTF-8 on MSVC and GCC/Clang
if (MSVC)
  add_compile_options(/utf-8)
else()
  add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()
