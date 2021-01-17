# This file is part of the Primal library.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

source_group("src" REGULAR_EXPRESSION "/src/")
add_executable(primal_tests
	src/c_ptr.cpp
	src/fixed.cpp
	src/intrinsics.cpp
	src/main.cpp
	src/rigid_vector.cpp
	src/static_vector.cpp
	)
target_compile_options(primal_tests PRIVATE ${MAX_WARNINGS})
target_include_directories(primal_tests PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(primal_tests PRIVATE primal)
set_target_properties(primal_tests PROPERTIES
	FOLDER primal
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
	POSITION_INDEPENDENT_CODE OFF
	)
add_test(NAME primal COMMAND primal_tests)
