# tests/CMakeLists.txt

include(FetchContent)

# ===== GoogleTest =====
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)


set(PROJ_INCLUDE_DIRS
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/lib
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
)


if (TARGET lemon)
  list(APPEND PROJ_INCLUDE_DIRS
    ${lemon_SOURCE_DIR}
    ${lemon_BINARY_DIR}
  )
endif()


function(add_simple_gtest target)
  add_executable(${target} ${ARGN})
  target_include_directories(${target} PRIVATE ${PROJ_INCLUDE_DIRS})
  target_compile_features(${target} PRIVATE cxx_std_17)
  target_link_libraries(${target} PRIVATE gtest_main)
  gtest_discover_tests(${target})
endfunction()

# =========================
# D0 (ללא LEMON)
# =========================
add_simple_gtest(test_d0
  ${CMAKE_SOURCE_DIR}/tests/test_d0.cpp
)

# =========================
# Hash (ללא LEMON)
# =========================
add_simple_gtest(test_hash
  ${CMAKE_SOURCE_DIR}/tests/test_hash.cpp
)

# =========================
# D1 (ללא LEMON)
# =========================
add_executable(test_d1
  ${CMAKE_SOURCE_DIR}/tests/test_d1.cpp
)
target_include_directories(test_d1 PRIVATE ${PROJ_INCLUDE_DIRS})
target_compile_features(test_d1 PRIVATE cxx_std_17)
target_link_libraries(test_d1 PRIVATE gtest_main)
gtest_discover_tests(test_d1)

# =========================
# Relax (ללא LEMON)
# =========================
add_simple_gtest(test_relax
  ${CMAKE_SOURCE_DIR}/tests/test_relax.cpp
)

# =========================
# Find pivots (ללא LEMON)
# =========================
add_simple_gtest(test_find_pivots
  ${CMAKE_SOURCE_DIR}/tests/test_find_pivots.cpp
)

# =========================
# BaseCase (ללא LEMON)
# =========================
if (EXISTS "${CMAKE_SOURCE_DIR}/tests/test_base_case.cpp")
  add_simple_gtest(test_base_case
    ${CMAKE_SOURCE_DIR}/tests/test_base_case.cpp
  )
endif()

# =========================
# מטרות שתלויות ב-LEMON בלבד
# =========================
if (TARGET lemon)
  # Dijkstra (+lemon)
  add_executable(test_dijkstra
    ${CMAKE_SOURCE_DIR}/tests/test_dijkstra.cpp
    ${CMAKE_SOURCE_DIR}/tests/test_dijkstra_suite.cpp
  )
  target_include_directories(test_dijkstra PRIVATE ${PROJ_INCLUDE_DIRS})
  target_compile_features(test_dijkstra PRIVATE cxx_std_17)
  target_link_libraries(test_dijkstra PRIVATE gtest_main lemon)
  gtest_discover_tests(test_dijkstra)

  # lgf_to_adj (+lemon, +tools path)
  add_executable(test_lgf_to_adj
    ${CMAKE_SOURCE_DIR}/tests/test_lgf_to_adj.cpp
  )
  target_include_directories(test_lgf_to_adj PRIVATE ${PROJ_INCLUDE_DIRS})
  target_compile_features(test_lgf_to_adj PRIVATE cxx_std_17)
  file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/tools/rb2lemon/tmp_lgf")
  target_compile_definitions(test_lgf_to_adj PRIVATE
    RB2LEMON_TMP_DIR=\"${CMAKE_SOURCE_DIR}/tools/rb2lemon/tmp_lgf\"
  )
  target_link_libraries(test_lgf_to_adj PRIVATE gtest_main gmock lemon)
  gtest_discover_tests(test_lgf_to_adj)

  # show_adj_from_lgf (רק הרצה)
  add_executable(test_show_adj_from_lgf
    ${CMAKE_SOURCE_DIR}/tests/test_show_adj_from_lgf.cpp
  )
  target_include_directories(test_show_adj_from_lgf PRIVATE ${PROJ_INCLUDE_DIRS})
  target_compile_features(test_show_adj_from_lgf PRIVATE cxx_std_17)
  target_link_libraries(test_show_adj_from_lgf PRIVATE lemon)
  add_test(NAME show_adj_from_lgf_runs
    COMMAND test_show_adj_from_lgf "${CMAKE_SOURCE_DIR}/data/EAT_RS_graph.lgf"
  )

  # DualBlockStructure flow  (כולל קישור ל-CPP של המבנה)
  add_executable(test_dual_block_flow
    ${CMAKE_SOURCE_DIR}/tests/test_dual_block_flow.cpp
    ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
  )
  target_include_directories(test_dual_block_flow PRIVATE ${PROJ_INCLUDE_DIRS})
  target_compile_features(test_dual_block_flow PRIVATE cxx_std_17)
  target_link_libraries(test_dual_block_flow PRIVATE gtest_main lemon)
  gtest_discover_tests(test_dual_block_flow)



# =========================================
# bmssp vs dijkstra tests
# =========================================
add_executable(test_bmssp_vs_dijkstra
  ${CMAKE_SOURCE_DIR}/tests/test_bmssp_vs_dijkstra.cpp
  # אם המימוש של BMSSP נמצא בקובץ cpp נפרד – צרפי אותו כאן:
  # ${CMAKE_SOURCE_DIR}/src/lib/algorithms/bmssp.cpp
)

target_include_directories(test_bmssp_vs_dijkstra PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/src/lib
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
)

target_sources(test_bmssp_vs_dijkstra PRIVATE
  ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
)

target_compile_features(test_bmssp_vs_dijkstra PRIVATE cxx_std_17)

target_link_libraries(test_bmssp_vs_dijkstra PRIVATE
  gtest_main
  lemon
)

gtest_discover_tests(test_bmssp_vs_dijkstra)


# =========================================
# correctness tests: BMSSP == Dijkstra
# =========================================
add_executable(test_correctness_bmssp
  ${CMAKE_SOURCE_DIR}/tests/test_correctness_bmssp.cpp
  # אם BMSSP/ה-D-structure לא header-only אצלך, צרפי גם את ה־cpp הרלוונטיים:
  ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
)

target_include_directories(test_correctness_bmssp PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/src/lib
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
)

target_compile_features(test_correctness_bmssp PRIVATE cxx_std_17)

target_link_libraries(test_correctness_bmssp PRIVATE
  gtest_main
  lemon
)

include(GoogleTest)
gtest_discover_tests(test_correctness_bmssp)



# include(GoogleTest)
# gtest_discover_tests(test_d1
#   DISCOVERY_TIMEOUT 60
#   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
# )
