# benchmarks/CMakeLists.txt

add_executable(bmssp_benchmark
  ${CMAKE_CURRENT_SOURCE_DIR}/bmssp_benchmark.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
)

add_executable(bmssp_vs_dijkstra
  ${CMAKE_CURRENT_SOURCE_DIR}/bmssp_vs_dijkstra.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
)

# include paths
target_include_directories(bmssp_benchmark PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/lib
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
)
target_include_directories(bmssp_vs_dijkstra PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/lib
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
)

# לינקים
target_link_libraries(bmssp_benchmark PRIVATE
  rbconv
  benchmark::benchmark_main
  lemon
)
target_link_libraries(bmssp_vs_dijkstra PRIVATE
  rbconv
  benchmark::benchmark_main
  lemon
)

if (MSVC)
  target_link_libraries(bmssp_benchmark PRIVATE legacy_stdio_definitions)
  target_link_libraries(bmssp_vs_dijkstra PRIVATE legacy_stdio_definitions)
endif()
