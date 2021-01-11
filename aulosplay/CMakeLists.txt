# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

set(HEADERS
	player.hpp
	)

set(SOURCES
	src/backend.hpp
	src/player.cpp
	src/utils.cpp
	src/utils.hpp
	)

if(WIN32)
	list(APPEND SOURCES src/backend_wasapi.cpp)
	set_property(SOURCE src/player.cpp APPEND PROPERTY COMPILE_OPTIONS
		/wd4355 # 'this': used in base member initializer list
		)
endif()

source_group("include" FILES ${HEADERS})
source_group("src" REGULAR_EXPRESSION "/src/")

add_library(aulosplay STATIC ${HEADERS} ${SOURCES})
target_compile_options(aulosplay PRIVATE ${MAX_WARNINGS})
target_include_directories(aulosplay PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set_property(TARGET aulosplay PROPERTY POSITION_INDEPENDENT_CODE ON)
install_target(aulosplay EXPORT AulosTargets)

if(WITH_STANDALONE)
	add_library(aulosplay_standalone STATIC ${HEADERS} ${SOURCES})
	target_compile_options(aulosplay_standalone PRIVATE ${MAX_WARNINGS})
	target_include_directories(aulosplay_standalone PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
	set_target_properties(aulosplay_standalone PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" POSITION_INDEPENDENT_CODE OFF)
	install_target(aulosplay_standalone EXPORT AulosTargets)
endif()

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aulosplay)
