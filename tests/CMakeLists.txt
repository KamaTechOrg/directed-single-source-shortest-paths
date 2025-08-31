# tests/CMakeLists.txt

include(FetchContent)

# === GoogleTest בלבד (לא להוריד פה LEMON שוב) ===
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

# === Tests target ===
add_executable(test_dijkstra
  test_dijkstra.cpp
  test_dijkstra_suite.cpp
 "test_dijkstra_from_graphml.cpp")

# אם את כוללת ישירות headers של lemon בטסטים – נשאיר include dirs
target_include_directories(test_dijkstra PRIVATE
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
)

# קישור ל-GTest ול-LEMON
target_link_libraries(test_dijkstra
  PRIVATE
    gtest_main
    lemon
)

include(GoogleTest)
gtest_discover_tests(test_dijkstra)
