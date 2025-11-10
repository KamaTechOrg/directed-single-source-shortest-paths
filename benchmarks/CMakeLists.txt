cmake_minimum_required(VERSION 3.20)

# ---- BMSSP ----
add_executable(bench_bmssp
  ${CMAKE_CURRENT_SOURCE_DIR}/bench_bmssp.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
)

# ---- Dijkstra ----
add_executable(bench_dijkstra
  ${CMAKE_CURRENT_SOURCE_DIR}/bench_dijkstra.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
)

# ---- Compare ----
add_executable(bench_compare
  ${CMAKE_CURRENT_SOURCE_DIR}/bench_compare.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
)

# ---- Compare with threads ----
add_executable(bench_compare_threads
  ${CMAKE_CURRENT_SOURCE_DIR}/bench_compare_threads.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
)

set(BENCH_COMMON_INCLUDES
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/lib
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
)

set(BENCH_COMMON_LIBS
  rbconv
  lemon
)

# C++17
target_compile_features(bench_bmssp            PRIVATE cxx_std_17)
target_compile_features(bench_dijkstra         PRIVATE cxx_std_17)
target_compile_features(bench_compare          PRIVATE cxx_std_17)
target_compile_features(bench_compare_threads  PRIVATE cxx_std_17)

# include paths
target_include_directories(bench_bmssp           PRIVATE ${BENCH_COMMON_INCLUDES})
target_include_directories(bench_dijkstra        PRIVATE ${BENCH_COMMON_INCLUDES})
target_include_directories(bench_compare         PRIVATE ${BENCH_COMMON_INCLUDES})
target_include_directories(bench_compare_threads PRIVATE ${BENCH_COMMON_INCLUDES})

# link libraries
target_link_libraries(bench_bmssp           PRIVATE ${BENCH_COMMON_LIBS})
target_link_libraries(bench_dijkstra        PRIVATE ${BENCH_COMMON_LIBS})
target_link_libraries(bench_compare         PRIVATE ${BENCH_COMMON_LIBS})
target_link_libraries(bench_compare_threads PRIVATE ${BENCH_COMMON_LIBS})

# Windows legacy_stdio_definitions 
if (MSVC)
  target_link_libraries(bench_bmssp           PRIVATE legacy_stdio_definitions)
  target_link_libraries(bench_dijkstra        PRIVATE legacy_stdio_definitions)
  target_link_libraries(bench_compare         PRIVATE legacy_stdio_definitions)
  target_link_libraries(bench_compare_threads PRIVATE legacy_stdio_definitions)
endif()

# === OpenMP (אופציונלי) ===
option(SSSP_ENABLE_OPENMP "Enable OpenMP parallel relax" ON)

if (SSSP_ENABLE_OPENMP)
  find_package(OpenMP)
  if (OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found: enabling parallel relax for benchmarks")
    foreach(tgt bench_bmssp bench_dijkstra bench_compare bench_compare_threads)
      target_link_libraries(${tgt} PRIVATE OpenMP::OpenMP_CXX)
      target_compile_definitions(${tgt} PRIVATE HAS_OMP=1)
    endforeach()
  else()
    message(STATUS "OpenMP NOT found: building benchmarks without parallel relax")
    foreach(tgt bench_bmssp bench_dijkstra bench_compare bench_compare_threads)
      target_compile_definitions(${tgt} PRIVATE HAS_OMP=0)
    endforeach()
  endif()
else()
  message(STATUS "OpenMP disabled by option")
  foreach(tgt bench_bmssp bench_dijkstra bench_compare bench_compare_threads)
    target_compile_definitions(${tgt} PRIVATE HAS_OMP=0)
  endforeach()
endif()

# === ITT (Intel Instrumentation) ===
option(ENABLE_ITT "Enable Intel ITT (pause/resume) if available" ON)

if (ENABLE_ITT)
  set(_VTUNE_HINT "$ENV{VTUNE_PROFILER_2025_DIR}")

  set(_ITT_INCLUDE_HINTS
      "${_VTUNE_HINT}/include"
      "C:/Program Files (x86)/Intel/oneAPI/vtune/latest/include"
      "/opt/intel/oneapi/vtune/latest/include"
  )
  set(_ITT_LIBRARY_HINTS
      "${_VTUNE_HINT}/lib64"
      "C:/Program Files (x86)/Intel/oneAPI/vtune/latest/lib64"
      "/opt/intel/oneapi/vtune/latest/lib64"
      "C:/Program Files (x86)/Intel/oneAPI/vtune/latest/lib"
  )

  find_path(ITT_INCLUDE_DIR ittnotify.h HINTS ${_ITT_INCLUDE_HINTS})
  find_library(ITT_LIBRARY NAMES ittnotify libittnotify HINTS ${_ITT_LIBRARY_HINTS})

  if (ITT_INCLUDE_DIR AND ITT_LIBRARY)
    message(STATUS "ITT found: ${ITT_INCLUDE_DIR}, ${ITT_LIBRARY}")

    if (NOT TARGET ittnotify_iface)
      add_library(ittnotify_iface INTERFACE)
      target_include_directories(ittnotify_iface INTERFACE "${ITT_INCLUDE_DIR}")
      target_link_libraries(ittnotify_iface INTERFACE "${ITT_LIBRARY}")
    endif()

    foreach(tgt bench_bmssp bench_dijkstra bench_compare bench_compare_threads)
      target_compile_definitions(${tgt} PRIVATE HAS_ITT=1)
      target_link_libraries(${tgt} PRIVATE ittnotify_iface)
    endforeach()
  else()
    message(WARNING "ITT not found. Building without ITT.")
    foreach(tgt bench_bmssp bench_dijkstra bench_compare bench_compare_threads)
      target_compile_definitions(${tgt} PRIVATE HAS_ITT=0)
    endforeach()
  endif()
endif()
