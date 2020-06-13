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
	src/composition.cpp
	src/composition.hpp
	src/data.cpp
	src/modulator.hpp
	src/note_table.hpp
	src/oscillator.hpp
	src/renderer.cpp
	src/shaper.hpp
	src/utils.hpp
	src/voice.hpp
	src/wave.hpp
	)
source_group("src" FILES ${_src})

add_library(aulos STATIC ${_include} ${_src})
target_compile_options(aulos PRIVATE ${MAX_WARNINGS})
target_include_directories(aulos PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

install(FILES ${_include} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aulos)
install(TARGETS aulos EXPORT AulosTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
