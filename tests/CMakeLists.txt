# tests/CMakeLists.txt

include(FetchContent)

# === LEMON ===
set(LEMON_ENABLE_GLPK  OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CPLEX OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CLP   OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CBC   OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_SOPLEX OFF CACHE BOOL "" FORCE)

FetchContent_Declare(lemon
  URL https://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.zip
)
FetchContent_MakeAvailable(lemon)

# === GoogleTest ===
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

# === Tests target ===
# include test_dijkstra.cpp and test_dijkstra_suite.cpp
add_executable(test_dijkstra
  test_dijkstra.cpp
  test_dijkstra_suite.cpp
)

# Include dirs for lemon
target_include_directories(test_dijkstra PRIVATE
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
)

# Link to gtest_main and lemon
target_link_libraries(test_dijkstra
  PRIVATE
    gtest_main
    lemon
)

# Discover tests (CTest)
include(GoogleTest)
gtest_discover_tests(test_dijkstra)