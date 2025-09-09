cmake_minimum_required(VERSION 3.15)
project(SSSP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


include(FetchContent)

# === LEMON ===
set(LEMON_ENABLE_GLPK   OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CPLEX  OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CLP    OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CBC    OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_SOPLEX OFF CACHE BOOL "" FORCE)

FetchContent_Declare(lemon
  URL https://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.zip
)
FetchContent_MakeAvailable(lemon)

add_subdirectory(src)

add_subdirectory(tools/rb2lemon)

enable_testing()
set(CTEST_OUTPUT_ON_FAILURE ON)

add_subdirectory(tests)


# UTF-8 on MSVC and GCC/Clang
if (MSVC)
  add_compile_options(/utf-8)
else()
  add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()
