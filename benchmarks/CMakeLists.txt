cmake_minimum_required(VERSION 3.20)

# ---- BMSSP ----
add_executable(bench_bmssp
  bench_bmssp.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
)

# ---- Dijkstra ----
add_executable(bench_dijkstra
  bench_dijkstra.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
)

# ---- Compare ----
add_executable(bench_compare
  ${CMAKE_CURRENT_SOURCE_DIR}/bench_compare.cpp
  ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp
)

# נתיבי include משותפים
set(BENCH_COMMON_INCLUDES
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src/lib
  ${CMAKE_SOURCE_DIR}/src/lib/internal
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
)

# ספריות קישור משותפות
set(BENCH_COMMON_LIBS
  rbconv
  lemon
)

# ---- הגדרות פר-טארגט (ללא foreach) ----
# C++17
target_compile_features(bench_bmssp    PRIVATE cxx_std_17)
target_compile_features(bench_dijkstra PRIVATE cxx_std_17)
target_compile_features(bench_compare  PRIVATE cxx_std_17)

# include paths
target_include_directories(bench_bmssp    PRIVATE ${BENCH_COMMON_INCLUDES})
target_include_directories(bench_dijkstra PRIVATE ${BENCH_COMMON_INCLUDES})
target_include_directories(bench_compare  PRIVATE ${BENCH_COMMON_INCLUDES})

# link libraries
target_link_libraries(bench_bmssp    PRIVATE ${BENCH_COMMON_LIBS})
target_link_libraries(bench_dijkstra PRIVATE ${BENCH_COMMON_LIBS})
target_link_libraries(bench_compare  PRIVATE ${BENCH_COMMON_LIBS})

# Windows legacy_stdio_definitions (אם צריך)
if (MSVC)
  target_link_libraries(bench_bmssp    PRIVATE legacy_stdio_definitions)
  target_link_libraries(bench_dijkstra PRIVATE legacy_stdio_definitions)
  target_link_libraries(bench_compare  PRIVATE legacy_stdio_definitions)
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

    target_compile_definitions(bench_bmssp    PRIVATE HAS_ITT=1)
    target_compile_definitions(bench_dijkstra PRIVATE HAS_ITT=1)
    target_compile_definitions(bench_compare  PRIVATE HAS_ITT=1)

    target_link_libraries(bench_bmssp    PRIVATE ittnotify_iface)
    target_link_libraries(bench_dijkstra PRIVATE ittnotify_iface)
    target_link_libraries(bench_compare  PRIVATE ittnotify_iface)
  else()
    message(WARNING "ITT not found. Building without ITT.")
    target_compile_definitions(bench_bmssp    PRIVATE HAS_ITT=0)
    target_compile_definitions(bench_dijkstra PRIVATE HAS_ITT=0)
    target_compile_definitions(bench_compare  PRIVATE HAS_ITT=0)
  endif()
endif()
