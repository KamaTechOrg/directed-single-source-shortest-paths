# tests/CMakeLists.txt

# ----- Fetch GoogleTest once -----
include(FetchContent)

FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

# נדרש כדי להשתמש ב-gtest_discover_tests לכל היעדים
include(GoogleTest)

# ----- אופציה לבנות/לא לבנות טסטים שתלויים ב-LEMON -----
option(ENABLE_LEMON "Build tests that depend on LEMON" ON)

# ----- Include paths שמשותפים לכל הטסטים -----
set(PROJ_INCLUDE_DIRS
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/lib
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
)

# אם lemon נבנה בפרויקט – נוסיף גם את התיקיות שלו ל-include
if (TARGET lemon)
  list(APPEND PROJ_INCLUDE_DIRS
    ${lemon_SOURCE_DIR}
    ${lemon_BINARY_DIR}
  )
endif()

# ----- פונקציות עזר לאחידות הגדרה של טסטים -----

# טסט שאינו תלוי ב-LEMON
function(add_simple_gtest target)
  add_executable(${target} ${ARGN})
  target_include_directories(${target} PRIVATE ${PROJ_INCLUDE_DIRS})
  target_compile_features(${target} PRIVATE cxx_std_17)
  target_link_libraries(${target} PRIVATE gtest_main)
  gtest_discover_tests(${target})
endfunction()

# טסט שתלוי ב-LEMON
function(add_lemon_gtest target)
  add_executable(${target} ${ARGN})
  target_include_directories(${target} PRIVATE ${PROJ_INCLUDE_DIRS})
  target_compile_features(${target} PRIVATE cxx_std_17)
  target_link_libraries(${target} PRIVATE gtest_main lemon)
  gtest_discover_tests(${target})
endfunction()

# =========================
# טסטים ללא LEMON
# =========================
add_simple_gtest(test_d0
  ${CMAKE_SOURCE_DIR}/tests/test_d0.cpp
)

add_simple_gtest(test_hash
  ${CMAKE_SOURCE_DIR}/tests/test_hash.cpp
)

add_simple_gtest(test_d1
  ${CMAKE_SOURCE_DIR}/tests/test_d1.cpp
)

add_simple_gtest(test_relax
  ${CMAKE_SOURCE_DIR}/tests/test_relax.cpp
)

add_simple_gtest(test_find_pivots
  ${CMAKE_SOURCE_DIR}/tests/test_find_pivots.cpp
)

# רק אם קיים הקובץ – שלא יפיל קונפיגורציה אצל מי שאין לו אותו
if (EXISTS "${CMAKE_SOURCE_DIR}/tests/test_base_case.cpp")
  add_simple_gtest(test_base_case
    ${CMAKE_SOURCE_DIR}/tests/test_base_case.cpp
  )
endif()

# =========================
# טסטים שתלויים ב-LEMON בלבד
# =========================
if (ENABLE_LEMON AND TARGET lemon)

  # Dijkstra (+lemon)
  add_lemon_gtest(test_dijkstra
    ${CMAKE_SOURCE_DIR}/tests/test_dijkstra.cpp
    ${CMAKE_SOURCE_DIR}/tests/test_dijkstra_suite.cpp
  )

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

  # show_adj_from_lgf (רק הרצה, ללא GTest)
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
  add_lemon_gtest(test_dual_block_flow
    ${CMAKE_SOURCE_DIR}/tests/test_dual_block_flow.cpp
    ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
  )

  # bmssp vs dijkstra
  add_lemon_gtest(test_bmssp_vs_dijkstra
    ${CMAKE_SOURCE_DIR}/tests/test_bmssp_vs_dijkstra.cpp
    # אם יש cpp נפרד ל-BMSSP הוסיפי כאן (השאירי כמוכנס רק אם קיים אצלך):
    # ${CMAKE_SOURCE_DIR}/src/lib/algorithms/bmssp.cpp
    ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
  )

  # correctness: BMSSP == Dijkstra
  add_lemon_gtest(test_correctness_bmssp
    ${CMAKE_SOURCE_DIR}/tests/test_correctness_bmssp.cpp
    ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
  )

endif()

