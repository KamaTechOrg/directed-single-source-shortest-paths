cmake_minimum_required(VERSION 3.15)
project(SSSP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# === toggle: build targets that require LEMON ===
option(USE_LEMON "Build parts that require the LEMON graph library" OFF)

include(FetchContent)

# --- LEMON (optional) ---
if (USE_LEMON)
  # lighter build: disable external solvers
  set(LEMON_ENABLE_GLPK   OFF CACHE BOOL "" FORCE)
  set(LEMON_ENABLE_CPLEX  OFF CACHE BOOL "" FORCE)
  set(LEMON_ENABLE_CLP    OFF CACHE BOOL "" FORCE)
  set(LEMON_ENABLE_CBC    OFF CACHE BOOL "" FORCE)
  set(LEMON_ENABLE_SOPLEX OFF CACHE BOOL "" FORCE)

  # A) online:
  # FetchContent_Declare(lemon
  #   URL https://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.zip
  # )

  # B) offline (recommended behind NetFree): place the zip locally
  # set(LEMON_LOCAL_ZIP "${CMAKE_SOURCE_DIR}/third_party/lemon-1.3.1.zip")
  # file(TO_CMAKE_PATH "${LEMON_LOCAL_ZIP}" LEMON_LOCAL_ZIP_CMAKE)
  # FetchContent_Declare(lemon URL "file://${LEMON_LOCAL_ZIP_CMAKE}")

  # choose one of A/B above; defaulting to A for now:
  FetchContent_Declare(lemon
    URL https://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.zip
  )
  FetchContent_MakeAvailable(lemon)

  # tools that rely on LEMON
  add_subdirectory(tools/rb2lemon)
endif()

add_subdirectory(src)

enable_testing()
set(CTEST_OUTPUT_ON_FAILURE ON)
add_subdirectory(tests)

# UTF-8 on MSVC and GCC/Clang
if (MSVC)
  add_compile_options(/utf-8)
else()
  add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()


