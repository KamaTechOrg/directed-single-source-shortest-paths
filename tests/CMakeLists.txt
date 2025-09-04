# tests/CMakeLists.txt

include(FetchContent)



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


add_executable(test_lgf_to_adj
  "test_lgf_to_adj.cpp"
)

target_include_directories(test_lgf_to_adj PRIVATE
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
)

target_compile_features(test_lgf_to_adj PRIVATE cxx_std_17)

file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/tools/rb2lemon/tmp_lgf")

target_compile_definitions(test_lgf_to_adj PRIVATE
  RB2LEMON_TMP_DIR=\"${CMAKE_SOURCE_DIR}/tools/rb2lemon/tmp_lgf\"
)

target_link_libraries(test_lgf_to_adj PRIVATE
  gtest_main
  gmock
  lemon
)




# Discover tests (CTest)
include(GoogleTest)
gtest_discover_tests(test_dijkstra)
gtest_discover_tests(test_lgf_to_adj)

