# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

primal_set_folder(aulos)
source_group("src" REGULAR_EXPRESSION "/src/")
add_executable(aulos_tests
	src/acoustics.cpp
	src/common.cpp
	src/format.cpp
	src/loop.cpp
	src/main.cpp
	src/modulator.cpp
	src/oscillator.cpp
	src/period.cpp
	src/shaper.cpp
	src/tables.cpp
	src/voice.cpp
	src/wave.cpp
	)
target_compile_options(aulos_tests PRIVATE ${MAX_WARNINGS})
target_include_directories(aulos_tests PRIVATE ${PRIMAL_DOWNLOAD_DIR})
target_link_libraries(aulos_tests PRIVATE aulos primal)
set_target_properties(aulos_tests PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<BOOL:${AULOS_STATIC_RUNTIME}>>:DLL>")
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(aulos_tests PRIVATE
		/wd5245 # unreferenced function with internal linkage has been removed
		)
endif()
add_test(NAME aulos COMMAND aulos_tests)
