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

# C++17 + includes + קישור בסיסי
foreach(tgt IN ITEMS bench_bmssp bench_dijkstra bench_compare)
  target_compile_features(${tgt} PRIVATE cxx_std_17)
  target_include_directories(${tgt} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/lib
    ${CMAKE_SOURCE_DIR}/src/lib/internal
    ${CMAKE_SOURCE_DIR}/tools/rb2lemon
  )
  target_link_libraries(${tgt} PRIVATE rbconv lemon)
endforeach()

# Windows legacy_stdio_definitions (אם צריך)
if (MSVC)
  target_link_libraries(bench_bmssp   PRIVATE legacy_stdio_definitions)
  target_link_libraries(bench_dijkstra PRIVATE legacy_stdio_definitions)
  target_link_libraries(bench_compare  PRIVATE legacy_stdio_definitions)
endif()

# === ITT (Intel Instrumentation) ===
option(ENABLE_ITT "Enable Intel ITT (pause/resume) if available" ON)

if (ENABLE_ITT)
  # דוגמה למשתנה סביבה נפוץ: VTUNE_PROFILER_2025_DIR=C:\Program Files (x86)\Intel\oneAPI\vtune\2025.6
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
      "C:/Program Files (x86)/Intel/oneAPI/vtune/latest/lib"  # לעיתים ב-Windows
  )

  find_path(ITT_INCLUDE_DIR ittnotify.h HINTS ${_ITT_INCLUDE_HINTS})
  find_library(ITT_LIBRARY NAMES ittnotify libittnotify HINTS ${_ITT_LIBRARY_HINTS})

  if (ITT_INCLUDE_DIR AND ITT_LIBRARY)
    message(STATUS "ITT found: ${ITT_INCLUDE_DIR}, ${ITT_LIBRARY}")

    # הגנה מפני הגדרה כפולה אם ה-CMake הזה נטען יותר מפעם אחת/מקובץ-אב
    if (NOT TARGET ittnotify_iface)
      add_library(ittnotify_iface INTERFACE)
      target_include_directories(ittnotify_iface INTERFACE "${ITT_INCLUDE_DIR}")
      target_link_libraries(ittnotify_iface INTERFACE "${ITT_LIBRARY}")
    endif()

    foreach(tgt IN ITEMS bench_bmssp bench_dijkstra bench_compare)
      target_compile_definitions(${tgt} PRIVATE HAS_ITT=1)
      target_link_libraries(${tgt} PRIVATE ittnotify_iface)
    endforeach()
  else()
    message(WARNING "ITT not found. Building without ITT.")
    foreach(tgt IN ITEMS bench_bmssp bench_dijkstra bench_compare)
      target_compile_definitions(${tgt} PRIVATE HAS_ITT=0)
    endforeach()
  endif()
endif()
