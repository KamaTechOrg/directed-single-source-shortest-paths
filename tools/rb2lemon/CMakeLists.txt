# tools/rb2lemon/CMakeLists.txt

# ספריית ליבה – לוגיקת ההמרה
add_library(rbconv STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/rb_to_lemon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/rb_to_lemon.hpp
)
target_link_libraries(rbconv PUBLIC lemon)
target_include_directories(rbconv
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${lemon_SOURCE_DIR}      # כדי שהכולל <lemon/...> יעבוד
    ${lemon_BINARY_DIR}
)

# CLI
add_executable(rb2lemon_cli
  ${CMAKE_CURRENT_SOURCE_DIR}/rb_to_lemon_cli.cpp
)
target_link_libraries(rb2lemon_cli PRIVATE rbconv)

# GUI
if (WIN32)
  add_executable(rb2lemon_gui WIN32 ${CMAKE_CURRENT_SOURCE_DIR}/rb_to_lemon_gui.cpp)
  target_compile_definitions(rb2lemon_gui PRIVATE UNICODE _UNICODE NOMINMAX)
  target_link_libraries(rb2lemon_gui PRIVATE rbconv Comdlg32 User32)
endif()
