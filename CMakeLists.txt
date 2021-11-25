# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

project(Aulos VERSION 0.0.5.0 HOMEPAGE_URL https://github.com/blagodarin/aulos LANGUAGES CXX)

include(CMakeDependentOption)

option(AULOS_STATIC_RUNTIME "Build Aulos with static runtime" OFF)
if(PROJECT_IS_TOP_LEVEL)
	option(AULOS_BENCHMARKS "Build Aulos benchmarks" OFF)
	option(AULOS_TESTS "Build Aulos tests" OFF)
	cmake_dependent_option(AULOS_STUDIO "Build Aulos Studio (requires Qt)" OFF "NOT AULOS_STATIC_RUNTIME" OFF)
	cmake_dependent_option(AULOS_STUDIO_INSTALLER "Build Aulos Studio installer (requires NSIS)" OFF "AULOS_STUDIO" OFF)
	cmake_dependent_option(AULOS_STUDIO_QT6 "Build Aulos Studio with Qt 6" OFF "AULOS_STUDIO" OFF)
	cmake_dependent_option(AULOS_STUDIO_RELEASE "Produce release version of Aulos Studio" OFF "AULOS_STUDIO" OFF)
	set(SEIR_AUDIO ${AULOS_STUDIO})
	set(SEIR_STATIC_RUNTIME ${AULOS_STATIC_RUNTIME})
	include(FetchContent)
	FetchContent_Declare(Seir GIT_REPOSITORY https://github.com/blagodarin/seir.git GIT_TAG ebf14e8ba74aadf1dc195167601290b3564f6d7c)
	FetchContent_MakeAvailable(Seir)
	seir_defaults()
endif()

# Qt headers don't compile with high warning levels,
# so a separate set of warning settings is needed.

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	add_compile_options(-Werror -Wall -Wextra)
	set(STUDIO_WARNINGS
		-Wno-deprecated-enum-enum-conversion
		)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	add_compile_options(-Werror -Wall -Wextra)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	add_compile_options(/WX)
	set(MAX_WARNINGS /Wall
		/wd4061 # enumerator '___' in switch of enum '___' is not explicitly handled by a case label
		/wd4514 # unreferenced inline function has been removed
		/wd4571 # Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
		/wd4623 # default constructor was implicitly defined as deleted
		/wd4625 # copy constructor was implicitly defined as deleted
		/wd4626 # assignment operator was implicitly defined as deleted
		/wd4710 # function not inlined
		/wd4711 # function '___' selected for automatic inline expansion
		/wd4738 # storing 32-bit float result in memory, possible loss of performance
		/wd4820 # '___' bytes padding added after data member '___'
		/wd5026 # move constructor was implicitly defined as deleted
		/wd5027 # move assignment operator was implicitly defined as deleted
		/wd5045 # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
		)
	set(STUDIO_WARNINGS /W4
		/wd5054 # operator '___': deprecated between enumerations of different types
		/wd5240 # attribute is ignored in this syntactic position
		)
endif()

if(NOT CMAKE_GENERATOR MATCHES "^Visual Studio")
	set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /nologo")
endif()

set(AULOS_RC_VERSION "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},${PROJECT_VERSION_TWEAK}")
set(AULOS_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
configure_file(config.h.in ${PROJECT_BINARY_DIR}/aulos_config.h)

add_subdirectory(aulos)

if(AULOS_STUDIO)
	if(AULOS_STUDIO_QT6)
		set(AULOS_QT Qt6)
	else()
		set(AULOS_QT Qt5)
	endif()
	find_package(${AULOS_QT} COMPONENTS Widgets REQUIRED)
	message(STATUS "Using Qt ${${AULOS_QT}_VERSION}")
	add_subdirectory(studio)
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT studio)
	if(AULOS_STUDIO_INSTALLER)
		add_subdirectory(studio/installer)
	endif()
endif()

if(AULOS_TESTS)
	seir_provide_doctest(doctest_ROOT STATIC_RUNTIME ${AULOS_STATIC_RUNTIME})
	find_package(doctest REQUIRED)
	enable_testing()
	add_subdirectory(tests/aulos)
endif()

if(AULOS_BENCHMARKS)
	add_subdirectory(benchmark)
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(AulosConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/AulosConfig.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Aulos)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/AulosConfigVersion.cmake COMPATIBILITY SameMinorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/AulosConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/AulosConfigVersion.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Aulos)
install(EXPORT AulosTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Aulos NAMESPACE Aulos::)
