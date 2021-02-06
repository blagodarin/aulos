# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

primal_set_folder(aulos)
source_group("src" REGULAR_EXPRESSION "/src/")
add_executable(aulos_tests
	src/format.cpp
	src/loop.cpp
	src/main.cpp
	src/shaper.cpp
	src/voice.cpp
	)
target_compile_options(aulos_tests PRIVATE ${MAX_WARNINGS})
target_include_directories(aulos_tests PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(aulos_tests PRIVATE aulos primal)
set_target_properties(aulos_tests PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<BOOL:${AULOS_STATIC_RUNTIME}>>:DLL>")
add_test(NAME aulos COMMAND aulos_tests)
