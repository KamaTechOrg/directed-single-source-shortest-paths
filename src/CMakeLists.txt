# src/CMakeLists.txt



add_executable(SSSP
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/dual_block_structure.cpp
)
target_compile_features(SSSP PUBLIC cxx_std_17)

# ---- include dirs ( tools/rb2lemon) ----


target_include_directories(SSSP PRIVATE
  ${CMAKE_SOURCE_DIR}/include 
  ${CMAKE_CURRENT_SOURCE_DIR}/lib
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/internal
  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
)

target_link_libraries(SSSP PRIVATE rbconv)


if (TARGET lemon)
  target_include_directories(SSSP PRIVATE
    ${lemon_SOURCE_DIR}
    ${lemon_BINARY_DIR}
  )
  target_link_libraries(SSSP PRIVATE lemon)
endif()

