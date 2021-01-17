# This file is part of the Primal library.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

source_group("include" REGULAR_EXPRESSION ".*\\.hpp")
set(HEADERS
	c_ptr.hpp
	fixed.hpp
	intrinsics.hpp
	rigid_vector.hpp
	static_vector.hpp
	)
add_library(primal INTERFACE)
target_include_directories(primal INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
set_property(TARGET primal PROPERTY FOLDER primal)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
	target_sources(primal PRIVATE ${HEADERS})
endif()
install_target(primal EXPORT AulosTargets)
