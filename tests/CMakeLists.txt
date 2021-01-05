# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

source_group("src" REGULAR_EXPRESSION "/src/")
add_executable(tests
	src/format.cpp
	src/loop.cpp
	src/main.cpp
	src/shaper.cpp
	src/utils.cpp
	src/voice.cpp
	)
target_compile_options(tests PRIVATE ${MAX_WARNINGS})
target_include_directories(tests PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(tests PRIVATE aulos_standalone)
set_target_properties(tests PROPERTIES
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
	POSITION_INDEPENDENT_CODE OFF
	)

add_test(NAME aulos COMMAND tests)
