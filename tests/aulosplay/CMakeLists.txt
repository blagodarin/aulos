# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

source_group("src" REGULAR_EXPRESSION "/src/")
add_executable(aulosplay_tests
	src/backend.cpp
	src/common.hpp
	src/main.cpp
	src/player.cpp
	src/utils.cpp
	)
target_compile_options(aulosplay_tests PRIVATE ${MAX_WARNINGS})
target_include_directories(aulosplay_tests PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(aulosplay_tests PRIVATE aulosplay_standalone primal)
set_target_properties(aulosplay_tests PROPERTIES
	FOLDER aulosplay
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
	POSITION_INDEPENDENT_CODE OFF
	)
add_test(NAME aulosplay COMMAND aulosplay_tests)
