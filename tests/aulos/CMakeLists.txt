# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

source_group("src" REGULAR_EXPRESSION "/src/")
add_executable(aulos_tests
	src/acoustics.cpp
	src/common.cpp
	src/format.cpp
	src/loop.cpp
	src/modulator.cpp
	src/oscillator.cpp
	src/period.cpp
	src/shaper.cpp
	src/tables.cpp
	src/voice.cpp
	src/wave.cpp
	)
target_compile_options(aulos_tests PRIVATE ${MAX_WARNINGS})
target_link_libraries(aulos_tests PRIVATE aulos Seir::base doctest::doctest_with_main)
seir_target(aulos_tests STATIC_RUNTIME ${AULOS_STATIC_RUNTIME})
add_test(NAME aulos COMMAND aulos_tests)
