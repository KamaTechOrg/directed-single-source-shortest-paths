# tests/CMakeLists.txt
include(FetchContent)

# === GoogleTest ===

FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

# =========================================
# D0 tests 
# =========================================
add_executable(test_d0
  ${CMAKE_SOURCE_DIR}/tests/test_d0.cpp
)
target_include_directories(test_d0 PRIVATE
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/src/lib
)

target_compile_features(test_d0 PRIVATE cxx_std_17)   

target_link_libraries(test_d0 PRIVATE gtest_main)
include(GoogleTest)
gtest_discover_tests(test_d0)

# =========================================
# hash tests
# =========================================
add_executable(test_hash
  ${CMAKE_SOURCE_DIR}/tests/test_hash.cpp
)
target_include_directories(test_hash PRIVATE
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/src/lib
)
target_compile_features(test_hash PRIVATE cxx_std_17)
target_link_libraries(test_hash PRIVATE gtest_main)
gtest_discover_tests(test_hash)

# =========================================
# dijkstra tests
# =========================================
add_executable(test_dijkstra
  ${CMAKE_SOURCE_DIR}/tests/test_dijkstra.cpp
  ${CMAKE_SOURCE_DIR}/tests/test_dijkstra_suite.cpp
)
target_compile_features(test_dijkstra PRIVATE cxx_std_17)
target_include_directories(test_dijkstra PRIVATE
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/src/lib
)
target_link_libraries(test_dijkstra PRIVATE gtest_main lemon)
gtest_discover_tests(test_dijkstra)

# =========================================
# Tests target: lgf_to_adj 
# =========================================
add_executable(test_lgf_to_adj
  ${CMAKE_SOURCE_DIR}/tests/test_lgf_to_adj.cpp
)
target_include_directories(test_lgf_to_adj PRIVATE
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src/lib/internal 
  ${CMAKE_SOURCE_DIR}/src/lib
)
target_compile_features(test_lgf_to_adj PRIVATE cxx_std_17)
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/tools/rb2lemon/tmp_lgf")
target_compile_definitions(test_lgf_to_adj PRIVATE
  RB2LEMON_TMP_DIR=\"${CMAKE_SOURCE_DIR}/tools/rb2lemon/tmp_lgf\"
)
target_link_libraries(test_lgf_to_adj PRIVATE gtest_main gmock lemon)
gtest_discover_tests(test_lgf_to_adj)

# =========================================
# Tests target: show_adj_from_lgf 
# =========================================
add_executable(test_show_adj_from_lgf
  ${CMAKE_SOURCE_DIR}/tests/test_show_adj_from_lgf.cpp
)
target_compile_features(test_show_adj_from_lgf PRIVATE cxx_std_17)
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

# =========================================
# relax tests
# =========================================
add_executable(test_relax
  ${CMAKE_SOURCE_DIR}/tests/test_relax.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/algorithms/relax.cpp
)
target_include_directories(test_relax PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(test_relax PRIVATE cxx_std_17)
target_link_libraries(test_relax PRIVATE gtest_main)
gtest_discover_tests(test_relax)

# =========================================
# find_pivots tests
# =========================================
add_executable(test_find_pivots
  ${CMAKE_CURRENT_SOURCE_DIR}/test_find_pivots.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/algorithms/find_pivots.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/algorithms/relax.cpp
)
target_include_directories(test_find_pivots PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_compile_features(test_find_pivots PRIVATE cxx_std_17)
target_link_libraries(test_find_pivots PRIVATE gtest_main)
gtest_discover_tests(test_find_pivots)


add_executable(test_d1
  ${CMAKE_SOURCE_DIR}/tests/test_d1.cpp
)
target_include_directories(test_d1 PRIVATE
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/src/lib
)
target_compile_features(test_d1 PRIVATE cxx_std_17)

set_property(TARGET test_d1 PROPERTY CXX_STANDARD 17)
set_property(TARGET test_d1 PROPERTY CXX_STANDARD_REQUIRED YES)
set_property(TARGET test_d1 PROPERTY CXX_EXTENSIONS NO)
target_link_libraries(test_d1 PRIVATE gtest_main)


add_test(NAME test_d1_all COMMAND test_d1)


# include(GoogleTest)
# gtest_discover_tests(test_d1
#   DISCOVERY_TIMEOUT 60
#   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
# )
