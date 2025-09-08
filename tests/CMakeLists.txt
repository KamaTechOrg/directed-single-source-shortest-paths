# tests/CMakeLists.txt
include(FetchContent)

# === GoogleTest ===
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)



# =========================================
# D0 tests (target נפרד)
# =========================================
add_executable(test_d0
  ${CMAKE_SOURCE_DIR}/tests/test_d0.cpp
)
target_include_directories(test_d0 PRIVATE
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/src/lib
)

target_compile_features(test_d0 PRIVATE cxx_std_17)   # <<< חשוב!

target_link_libraries(test_d0 PRIVATE gtest_main)
include(GoogleTest)
gtest_discover_tests(test_d0)






# =========================================
# Tests target: Dijkstra
# =========================================
add_executable(test_dijkstra
  ${CMAKE_SOURCE_DIR}/tests/test_dijkstra.cpp
  ${CMAKE_SOURCE_DIR}/tests/test_dijkstra_suite.cpp
)
target_include_directories(test_dijkstra PRIVATE
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src/lib/internal  # אם הטסטים כוללים את d0/ds_common
  ${CMAKE_SOURCE_DIR}/src/lib
)
target_link_libraries(test_dijkstra PRIVATE gtest_main lemon)




# =========================================
# Tests target: lgf_to_adj (עם GTest)
# =========================================
add_executable(test_lgf_to_adj
  ${CMAKE_SOURCE_DIR}/tests/test_lgf_to_adj.cpp
)
target_include_directories(test_lgf_to_adj PRIVATE
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src/lib/internal  # אם צריך
  ${CMAKE_SOURCE_DIR}/src/lib
)
target_compile_features(test_lgf_to_adj PRIVATE cxx_std_17)
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/tools/rb2lemon/tmp_lgf")
target_compile_definitions(test_lgf_to_adj PRIVATE
  RB2LEMON_TMP_DIR=\"${CMAKE_SOURCE_DIR}/tools/rb2lemon/tmp_lgf\"
)
target_link_libraries(test_lgf_to_adj PRIVATE gtest_main gmock lemon)



# =========================================
# Tests target: show_adj_from_lgf (דוגמה להרצה ידנית)
# =========================================
add_executable(test_show_adj_from_lgf
  ${CMAKE_SOURCE_DIR}/tests/test_show_adj_from_lgf.cpp
)
target_include_directories(test_show_adj_from_lgf PRIVATE
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
)
target_link_libraries(test_show_adj_from_lgf PRIVATE lemon)

add_test(
  NAME show_adj_from_lgf_runs
  COMMAND test_show_adj_from_lgf
          "${CMAKE_SOURCE_DIR}/data/EAT_RS_graph.lgf"
)

# === Discover tests (GTest) ===
include(GoogleTest)
gtest_discover_tests(test_dijkstra)
gtest_discover_tests(test_lgf_to_adj)