# src/CMakeLists.txt

add_executable(show_adj_from_lgf
  ${CMAKE_CURRENT_SOURCE_DIR}/show_adj_from_lgf.cpp
)

add_executable(SSSP
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

# ---- include dirs ( tools/rb2lemon) ----
target_include_directories(show_adj_from_lgf PRIVATE
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon  
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
)

target_include_directories(SSSP PRIVATE
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon   
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
)

target_link_libraries(show_adj_from_lgf PRIVATE lemon)
target_link_libraries(SSSP            PRIVATE lemon)

