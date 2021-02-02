# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

primal_set_folder(aulos)
source_group("src" REGULAR_EXPRESSION "/src/")
add_executable(aulos_benchmark
	src/main.cpp
	)
target_compile_options(aulos_benchmark PRIVATE ${MAX_WARNINGS})
target_link_libraries(aulos_benchmark PRIVATE aulos_standalone)
primal_set_properties(aulos_benchmark STANDALONE)
primal_install(aulos_benchmark)
