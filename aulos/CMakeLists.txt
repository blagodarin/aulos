# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

set(HEADERS
	common.hpp
	composition.hpp
	data.hpp
	format.hpp
	renderer.hpp
	)

set(SOURCES
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
	src/utils/fixed.hpp
	src/utils/limited_vector.hpp
	src/utils/static_vector.hpp
	)

source_group("include" FILES ${HEADERS})
source_group("src" REGULAR_EXPRESSION "/src/")
source_group("src\\utils" REGULAR_EXPRESSION "/src/utils/")

add_library(aulos STATIC ${HEADERS} ${SOURCES})
target_compile_options(aulos PRIVATE ${MAX_WARNINGS})
target_include_directories(aulos PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

add_library(aulos_static STATIC ${HEADERS} ${SOURCES})
target_compile_options(aulos_static PRIVATE ${MAX_WARNINGS})
target_include_directories(aulos_static PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set_target_properties(aulos_static PROPERTIES
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
	POSITION_INDEPENDENT_CODE ON
	)

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aulos)
install_target(aulos EXPORT AulosTargets)
install_target(aulos_static EXPORT AulosTargets)
