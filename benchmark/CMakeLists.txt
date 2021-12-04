# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

source_group("src" REGULAR_EXPRESSION "/src/")
add_executable(aulos_benchmark
	src/main.cpp
	)
target_compile_options(aulos_benchmark PRIVATE ${MAX_WARNINGS})
target_link_libraries(aulos_benchmark PRIVATE Aulos::aulos)
seir_target(aulos_benchmark STATIC_RUNTIME ${AULOS_STATIC_RUNTIME})
