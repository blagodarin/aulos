# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

primal_set_folder(aulosplay)

set(HEADERS
	player.hpp
	)
source_group("include" FILES ${HEADERS})
source_group("src" REGULAR_EXPRESSION "/src/")
add_library(aulosplay STATIC
	src/backend.hpp
	src/player.cpp
	src/utils.hpp
	)
target_compile_options(aulosplay PRIVATE ${MAX_WARNINGS})
target_include_directories(aulosplay PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(aulosplay PRIVATE primal Threads::Threads)
set_target_properties(aulosplay PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<BOOL:${AULOS_STATIC_RUNTIME}>>:DLL>")
if(WIN32)
	target_sources(aulosplay PRIVATE src/backend_wasapi.cpp)
	set_property(SOURCE src/player.cpp APPEND PROPERTY COMPILE_OPTIONS
		/wd4355 # 'this': used in base member initializer list
		)
else()
	target_sources(aulosplay PRIVATE src/backend_alsa.cpp)
	target_link_libraries(aulosplay PRIVATE ALSA::ALSA)
endif()
primal_install(aulosplay EXPORT AulosTargets)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aulosplay)
