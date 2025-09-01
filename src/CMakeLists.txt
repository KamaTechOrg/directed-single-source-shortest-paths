

# src/CMakeLists.txt

# Executable להצגת רשימת שכנויות מקובץ LGF
add_executable(show_adj_from_lgf
  ${CMAKE_CURRENT_SOURCE_DIR}/show_adj_from_lgf.cpp
)
add_executable(SSSP main.cpp)


target_include_directories(show_adj_from_lgf PRIVATE
  ${CMAKE_SOURCE_DIR}/src         # כדי לכלול "io/sssp_lgf_io.hpp"
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
)

target_link_libraries(show_adj_from_lgf PRIVATE lemon)
