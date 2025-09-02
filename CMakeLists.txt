cmake_minimum_required(VERSION 3.15)
project(SSSP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# === LEMON פעם אחת ===
set(LEMON_ENABLE_GLPK   OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CPLEX  OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CLP    OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CBC    OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_SOPLEX OFF CACHE BOOL "" FORCE)

FetchContent_Declare(lemon
  URL https://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.zip
)
FetchContent_MakeAvailable(lemon)

# קוד הפרויקט
add_subdirectory(src)

# כלי ההמרה
add_subdirectory(tools/rb2lemon)

# טסטים
enable_testing()
set(CTEST_OUTPUT_ON_FAILURE ON)

add_subdirectory(tests)
