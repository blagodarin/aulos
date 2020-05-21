# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

set(_src
	src/main.cpp
	)
source_group("src" FILES ${_src})

add_executable(benchmark ${_src})
target_compile_options(benchmark PRIVATE ${MAX_WARNINGS})
target_link_libraries(benchmark PRIVATE aulos)
