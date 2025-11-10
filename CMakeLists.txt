cmake_minimum_required(VERSION 3.15)
project(SSSP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# --- OpenMP (optional) ---
option(SSSP_ENABLE_OPENMP "Enable OpenMP parallel relax" ON)

if (SSSP_ENABLE_OPENMP)
  find_package(OpenMP)
endif()

if (OpenMP_CXX_FOUND)
  set(SSSP_OPENMP_AVAILABLE TRUE)
  message(STATUS "OpenMP found: enabling parallel relax")
else()
  set(SSSP_OPENMP_AVAILABLE FALSE)
  message(STATUS "OpenMP NOT found: building without parallel relax")
endif()

# helper to attach OpenMP (and define HAS_OMP) to targets
function(sssp_enable_openmp target)
  if (SSSP_OPENMP_AVAILABLE)
    target_link_libraries(${target} PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(${target} PRIVATE HAS_OMP=1)
  else()
    target_compile_definitions(${target} PRIVATE HAS_OMP=0)
  endif()
endfunction()


# --- ITT (VTune) ---
option(USE_ITT "Enable VTune ITT API markers" ON)

if (USE_ITT)
  # include
  find_path(ITT_INCLUDE_DIR ittnotify.h
    HINTS
      "$ENV{VTUNE_PROFILER_2025_DIR}/include"
      "$ENV{VTUNE_PROFILER_DIR}/include"
      "$ENV{ONEAPI_ROOT}/vtune/latest/include"
      "C:/Program Files (x86)/Intel/oneAPI/vtune/latest/include"
  )
  # lib (ב־Windows בדרך כלל lib64)
  find_library(ITT_LIB ittnotify
    HINTS
      "$ENV{VTUNE_PROFILER_2025_DIR}/lib64"
      "$ENV{VTUNE_PROFILER_DIR}/lib64"
      "$ENV{ONEAPI_ROOT}/vtune/latest/lib64"
      "C:/Program Files (x86)/Intel/oneAPI/vtune/latest/lib64"
  )

  if (ITT_INCLUDE_DIR AND ITT_LIB)
    add_library(ittnotify_iface INTERFACE)
    target_include_directories(ittnotify_iface INTERFACE ${ITT_INCLUDE_DIR})
    target_link_libraries(ittnotify_iface INTERFACE ${ITT_LIB})
    target_compile_definitions(ittnotify_iface INTERFACE HAS_ITT=1)
    message(STATUS "ITT enabled: inc=${ITT_INCLUDE_DIR}, lib=${ITT_LIB}")
  else()
    message(WARNING "ITT not found; building without ITT markers")
  endif()
endif()


include(FetchContent)


set(LEMON_ENABLE_GLPK   OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CPLEX  OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CLP    OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CBC    OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_SOPLEX OFF CACHE BOOL "" FORCE)



FetchContent_Declare(lemon
  GIT_REPOSITORY https://github.com/alex-176/lemon.git
  GIT_TAG        main
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(lemon)




include(FetchContent)
set(BENCHMARK_ENABLE_TESTING        OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS    OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_EXAMPLES       OFF CACHE BOOL "" FORCE)
set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_WERROR         OFF CACHE BOOL "" FORCE)

FetchContent_Declare(benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG        v1.8.3
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(benchmark)


add_subdirectory(tools/rb2lemon)

add_subdirectory(src)

add_subdirectory(benchmarks)



enable_testing()
set(CTEST_OUTPUT_ON_FAILURE ON)

add_subdirectory(tests)


# UTF-8 on MSVC and GCC/Clang
if (MSVC)
  add_compile_options(/utf-8)
else()
  add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()
