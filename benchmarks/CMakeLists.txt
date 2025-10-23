cmake_minimum_required(VERSION 3.20)

# ---- BMSSP ----
add_executable(bench_bmssp
  bench_bmssp.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp   # חשוב! כדי לסגור unresolved externals
)

# ---- Dijkstra ----
add_executable(bench_dijkstra
  bench_dijkstra.cpp
  # אם Dijkstra לא משתמש ב-dual_block_structure.cpp אפשר להסיר את השורה הבאה.
  ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
)

# C++ standard
target_compile_features(bench_bmssp PRIVATE cxx_std_17)
target_compile_features(bench_dijkstra PRIVATE cxx_std_17)

# include paths (התאימי אם יש הבדלים אצלך)
target_include_directories(bench_bmssp PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/lib
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
)
target_include_directories(bench_dijkstra PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/lib
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
)

add_executable(bench_compare
  ${CMAKE_CURRENT_SOURCE_DIR}/bench_compare.cpp
)
target_include_directories(bench_compare PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/lib
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
)
target_link_libraries(bench_compare PRIVATE
  sssp_lib
  rbconv
  lemon
)

# קישורים לספריות שכבר יש לך בפרויקט
target_link_libraries(bench_bmssp PRIVATE
  rbconv
  lemon
)
target_link_libraries(bench_dijkstra PRIVATE
  rbconv
  lemon
)

# לרוב אין צורך, אבל אם MSVC מתבכיין על printf/stdio במצבים מסוימים:
if (MSVC)
  target_link_libraries(bench_bmssp PRIVATE legacy_stdio_definitions)
  target_link_libraries(bench_dijkstra PRIVATE legacy_stdio_definitions)
endif()
