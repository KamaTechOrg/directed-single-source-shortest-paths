# src/CMakeLists.txt



add_executable(SSSP
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
 ${CMAKE_CURRENT_SOURCE_DIR}/lib/algorithms/find_pivots.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/algorithms/relax.cpp   
)
target_compile_features(SSSP PUBLIC cxx_std_17)

# ---- include dirs ( tools/rb2lemon) ----


target_include_directories(SSSP PRIVATE
  ${CMAKE_SOURCE_DIR}/include 
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon   
  ${lemon_SOURCE_DIR}
  ${lemon_BINARY_DIR}
)

target_link_libraries(SSSP PRIVATE lemon)

