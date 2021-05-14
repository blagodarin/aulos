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
	src/oscillator.hpp
	src/period.hpp
	src/renderer.cpp
	src/shaper.hpp
	src/tables.hpp
	src/voice.hpp
	src/wave.hpp
	)
source_group("include" FILES ${HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
add_library(aulos STATIC ${HEADERS} ${SOURCES})
target_compile_options(aulos PRIVATE ${MAX_WARNINGS})
target_include_directories(aulos PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(aulos PRIVATE primal)
set_target_properties(aulos PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<BOOL:${AULOS_STATIC_RUNTIME}>>:DLL>")
primal_install(aulos EXPORT AulosTargets)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aulos)
