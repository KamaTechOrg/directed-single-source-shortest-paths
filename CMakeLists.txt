cmake_minimum_required(VERSION 3.15)
project(SSSP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# =========================
# LEMON 
# =========================
set(LEMON_ENABLE_GLPK   OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CPLEX  OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CLP    OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_CBC    OFF CACHE BOOL "" FORCE)
set(LEMON_ENABLE_SOPLEX OFF CACHE BOOL "" FORCE)

FetchContent_Declare(lemon
  URL https://lemon.cs.elte.hu/pub/sources/lemon-1.3.1.zip
)
FetchContent_MakeAvailable(lemon)

# =========================
# tinyxml2 - דרוש לטעינת GraphML
# =========================
# אם tinyxml2 כבר מגיע ממקום אחר בפרויקט, אפשר להסיר את הבלוק הזה.
FetchContent_Declare(
  tinyxml2
  GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
  GIT_TAG        10.0.0
)
FetchContent_MakeAvailable(tinyxml2)

# =========================
# GoogleTest - אם לא מוגדר במקום אחר
# =========================
# אם יש לך כבר FetchContent ל-gtest בתיקיית tests/, אפשר להשמיט את הבלוק הזה.
if (NOT TARGET gtest)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

# =========================
# ספריית לוגיקה משותפת (המימוש בלבד)
# =========================
add_library(rbconv STATIC tools/rb_to_lemon_impl.cpp "include/loaders/graphml_loader.cpp")
target_include_directories(rbconv PUBLIC
  ${lemon_SOURCE_DIR} ${lemon_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/tools            # כדי שה-headers שלך בכלים יימצאו
)
target_link_libraries(rbconv PUBLIC lemon)

# =========================
# CLI (אופציונלי) — רץ משורת פקודה עם ארגומנט קובץ
# =========================
add_executable(rb_to_lemon tools/rb_to_lemon_cli.cpp "include/loaders/graphml_loader.cpp")
target_link_libraries(rb_to_lemon PRIVATE rbconv)

# =========================
# GUI (Win32) — פותח חלון בחירת קובץ
# =========================
if (WIN32)
  add_executable(rb_to_lemon_gui WIN32 tools/rb_to_lemon_gui.cpp "include/loaders/graphml_loader.cpp")
  target_link_libraries(rb_to_lemon_gui PRIVATE rbconv Comdlg32)
endif()

# =========================
# קוד הפרויקט שלך
# =========================
add_subdirectory(src)

# =========================
# טסטים
# =========================
enable_testing()
set(CTEST_OUTPUT_ON_FAILURE ON)

# -- אפשר להשאיר את תת-התיקייה tests שלך אם כבר קיימת ובה טסטים נוספים:
add_subdirectory(tests)

# --- טסט דייקסטרה מ-GraphML (חדש) ---
# קובץ המקור של הטסט: tests/test_dijkstra_from_graphml.cpp
# הקפידי לכלול בו: #include "loaders/graphml_loader.hpp"
add_executable(test_dijkstra_from_graphml
  tests/test_dijkstra_from_graphml.cpp
 "include/loaders/graphml_loader.cpp")

# נתיבי include — lemon + include/ (שם שמנו את graphml_loader.hpp)
target_include_directories(test_dijkstra_from_graphml PRIVATE
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}/include
)

# קישוריות: lemon + gtest + tinyxml2
target_link_libraries(test_dijkstra_from_graphml PRIVATE
  lemon
  gtest
  gtest_main
  tinyxml2
)

# רישום הטסט ל-CTest, כדי שיופיע יפה ב-UI (CTest/CLion/VSCode Test Explorer)
# עדכני את הנתיב לקובץ ה-GraphML המומר שלך אם שונה.
add_test(NAME dijkstra_graphml
  COMMAND test_dijkstra_from_graphml
          ${PROJECT_SOURCE_DIR}/data/converted.graphml
)

# אפשרי: לקבוע משתני סביבה לטסט (למשל לבחור מקור אחר לגרף)
# set_tests_properties(dijkstra_graphml PROPERTIES
#   ENVIRONMENT "GRAPH_SRC=0"
#   TIMEOUT 30
# )
