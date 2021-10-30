# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

primal_set_folder(aulosplay)
source_group("src" REGULAR_EXPRESSION "/src/")
add_executable(aulosplay_tests
	src/backend.cpp
	src/common.hpp
	src/player.cpp
	src/utils.cpp
	)
target_compile_options(aulosplay_tests PRIVATE ${MAX_WARNINGS})
target_include_directories(aulosplay_tests PRIVATE ${PRIMAL_DOWNLOAD_DIR})
target_link_libraries(aulosplay_tests PRIVATE aulosplay primal doctest::doctest_with_main)
set_target_properties(aulosplay_tests PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<BOOL:${AULOS_STATIC_RUNTIME}>>:DLL>")
add_test(NAME aulosplay COMMAND aulosplay_tests)
