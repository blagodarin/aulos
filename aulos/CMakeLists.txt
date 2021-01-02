# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

set(_include
	common.hpp
	composition.hpp
	data.hpp
	)
source_group("include" FILES ${_include})

set(_src
	src/acoustics.hpp
	src/composition.cpp
	src/composition.hpp
	src/data.cpp
	src/modulator.hpp
	src/period.hpp
	src/renderer.cpp
	src/shaper.hpp
	src/tables.hpp
	src/voice.hpp
	src/wave.hpp
	)
source_group("src" FILES ${_src})

set(_src_utils
	src/utils/fixed.hpp
	src/utils/limited_vector.hpp
	src/utils/static_vector.hpp
	)
source_group("src\\utils" FILES ${_src_utils})

add_library(aulos STATIC ${_include} ${_src} ${_src_utils})
target_compile_options(aulos PRIVATE ${MAX_WARNINGS})
target_include_directories(aulos PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

install(FILES ${_include} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aulos)
install_target(aulos EXPORT AulosTargets)
