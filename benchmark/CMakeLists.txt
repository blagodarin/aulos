# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

source_group("src" REGULAR_EXPRESSION "/src/")
add_executable(benchmark
	src/main.cpp
	)
target_compile_options(benchmark PRIVATE ${MAX_WARNINGS})
target_link_libraries(benchmark PRIVATE aulos_static)
set_target_properties(benchmark PROPERTIES
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
	OUTPUT_NAME aulos_benchmark
	POSITION_INDEPENDENT_CODE ON
	)

install_target(benchmark)
