# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

primal_set_folder(aulos)

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
	)

source_group("include" FILES ${HEADERS})
source_group("src" REGULAR_EXPRESSION "/src/")

add_library(aulos STATIC ${HEADERS} ${SOURCES})
target_compile_options(aulos PRIVATE ${MAX_WARNINGS})
target_include_directories(aulos PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(aulos PRIVATE primal)
primal_set_properties(aulos)
primal_install(aulos EXPORT AulosTargets)

add_library(aulos_standalone STATIC ${HEADERS} ${SOURCES})
target_compile_options(aulos_standalone PRIVATE ${MAX_WARNINGS})
target_include_directories(aulos_standalone PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(aulos_standalone PRIVATE primal)
primal_set_properties(aulos_standalone STANDALONE)
primal_install(aulos_standalone EXPORT AulosTargets)

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aulos)
