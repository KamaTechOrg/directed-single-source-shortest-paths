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

		# C++ standard
		target_compile_features(bench_bmssp PRIVATE cxx_std_17)
		target_compile_features(bench_dijkstra PRIVATE cxx_std_17)

		# include paths 
		target_include_directories(bench_bmssp PRIVATE
		  ${CMAKE_SOURCE_DIR}/include
		  ${CMAKE_SOURCE_DIR}/src/lib
		  ${CMAKE_SOURCE_DIR}/src/lib/internal
		  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
		)
		target_include_directories(bench_dijkstra PRIVATE
		  ${CMAKE_SOURCE_DIR}/include
		  ${CMAKE_SOURCE_DIR}/src/lib
		  ${CMAKE_SOURCE_DIR}/src/lib/internal
		  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
		)

		# ---- Compare ----
	add_executable(bench_compare
	  ${CMAKE_CURRENT_SOURCE_DIR}/bench_compare.cpp
	  ${CMAKE_SOURCE_DIR}/src/lib/dual_block_structure.cpp   # כדי למנוע unresolved externals
	)

	target_include_directories(bench_compare PRIVATE
	  ${CMAKE_SOURCE_DIR}/include
	  ${CMAKE_SOURCE_DIR}/src/lib
	  ${CMAKE_SOURCE_DIR}/src/lib/internal
	  ${CMAKE_SOURCE_DIR}/tools/rb2lemon
	)

	# לא לקשר ל-sssp_lib (לא קיים) – רק למה שבאמת צריך
	target_link_libraries(bench_compare PRIVATE
	  rbconv
	  lemon
	)

	# אם אין לך ittnotify.h/lib – לבטל ITT רק ל-target הזה
	target_compile_definitions(bench_compare PRIVATE HAS_ITT=0)



		target_link_libraries(bench_bmssp PRIVATE
		  rbconv
		  lemon
		)
		target_link_libraries(bench_dijkstra PRIVATE
		  rbconv
		  lemon
		)

		if (MSVC)
		  target_link_libraries(bench_bmssp PRIVATE legacy_stdio_definitions)
		  target_link_libraries(bench_dijkstra PRIVATE legacy_stdio_definitions)
		  target_link_libraries(bench_compare PRIVATE legacy_stdio_definitions)

		endif()

		if (TARGET ittnotify_iface)
		  target_link_libraries(bench_compare PRIVATE ittnotify_iface)
		  target_link_libraries(bench_bmssp  PRIVATE ittnotify_iface)
		endif()