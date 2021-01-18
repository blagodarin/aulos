# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

set(HEADERS
	player.hpp
	)

set(SOURCES
	src/backend.hpp
	src/player.cpp
	src/utils.hpp
	)

if(WIN32)
	list(APPEND SOURCES src/backend_wasapi.cpp)
	set_property(SOURCE src/player.cpp APPEND PROPERTY COMPILE_OPTIONS
		/wd4355 # 'this': used in base member initializer list
		)
else()
	list(APPEND SOURCES src/backend_alsa.cpp)
	set(AULOSPLAY_LIBRARIES ALSA::ALSA)
endif()

source_group("include" FILES ${HEADERS})
source_group("src" REGULAR_EXPRESSION "/src/")

add_library(aulosplay STATIC ${HEADERS} ${SOURCES})
target_compile_options(aulosplay PRIVATE ${MAX_WARNINGS})
target_include_directories(aulosplay PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(aulosplay PRIVATE primal Threads::Threads ${AULOSPLAY_LIBRARIES})
set_target_properties(aulosplay PROPERTIES
	FOLDER aulosplay
	POSITION_INDEPENDENT_CODE ON
	)
if(WITH_AULOSPLAY)
	primal_install(aulosplay EXPORT AulosTargets)
endif()

if(WITH_STANDALONE OR WITH_TESTS)
	add_library(aulosplay_standalone STATIC ${HEADERS} ${SOURCES})
	target_compile_options(aulosplay_standalone PRIVATE ${MAX_WARNINGS})
	target_include_directories(aulosplay_standalone PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
	target_link_libraries(aulosplay_standalone PRIVATE primal Threads::Threads ${AULOSPLAY_LIBRARIES})
	set_target_properties(aulosplay_standalone PROPERTIES
		FOLDER aulosplay
		MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
		POSITION_INDEPENDENT_CODE OFF
		)
	if(WITH_AULOSPLAY AND WITH_STANDALONE)
		primal_install(aulosplay_standalone EXPORT AulosTargets)
	endif()
endif()

if(WITH_AULOSPLAY)
	install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aulosplay)
endif()
