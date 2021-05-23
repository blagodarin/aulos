# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.19)

project(Aulos VERSION 0.0.5.0 HOMEPAGE_URL https://github.com/blagodarin/aulos LANGUAGES CXX)

include(CMakeDependentOption)
include(FetchContent)

FetchContent_Declare(Primal GIT_REPOSITORY https://github.com/blagodarin/primal.git GIT_TAG 2944209a0a37d36e54b5ddc1b2f5b403e5b0f2f4)
FetchContent_MakeAvailable(Primal)

primal_set_defaults()

option(AULOS_AULOSPLAY "Build AulosPlay library" ON)
option(AULOS_STATIC_RUNTIME "Build Aulos with static runtime" OFF)
if(NOT AULOS_SUBPROJECT)
	option(AULOS_BENCHMARKS "Build Aulos benchmarks" OFF)
	option(AULOS_TESTS "Build Aulos tests" OFF)
	cmake_dependent_option(AULOS_STUDIO "Build Aulos Studio (requires Qt)" OFF "AULOS_AULOSPLAY;NOT AULOS_STATIC_RUNTIME" OFF)
	cmake_dependent_option(AULOS_STUDIO_INSTALLER "Build Aulos Studio installer (requires NSIS)" OFF "AULOS_STUDIO" OFF)
	cmake_dependent_option(AULOS_STUDIO_QT6 "Build Aulos Studio with Qt 6" OFF "AULOS_STUDIO" OFF)
	cmake_dependent_option(AULOS_STUDIO_RELEASE "Produce release version of Aulos Studio" OFF "AULOS_STUDIO" OFF)
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
	add_compile_options(/std:c++latest /WX)
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

if(WIN32)
	set(CMAKE_DEBUG_POSTFIX "d")
endif()

set(AULOS_RC_VERSION "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},${PROJECT_VERSION_TWEAK}")
set(AULOS_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
configure_file(config.h.in ${PROJECT_BINARY_DIR}/aulos_config.h)

add_subdirectory(aulos)

if(AULOS_AULOSPLAY)
	find_package(Threads REQUIRED)
	if(NOT WIN32)
		find_package(ALSA REQUIRED)
	endif()
	add_subdirectory(aulosplay)
endif()

if(AULOS_STUDIO)
	if(AULOS_STUDIO_QT6)
		set(AULOS_QT Qt6)
	else()
		set(AULOS_QT Qt5)
	endif()
	find_package(${AULOS_QT} COMPONENTS Widgets REQUIRED)
	message(STATUS "Using Qt ${${AULOS_QT}_VERSION}")
	add_subdirectory(studio)
	if(AULOS_STUDIO_INSTALLER)
		add_subdirectory(studio/installer)
	endif()
endif()

if(AULOS_TESTS)
	enable_testing()
	primal_download("https://raw.githubusercontent.com/onqtam/doctest/2.4.4/doctest/doctest.h" SHA1 "7c25b870f3e0cde6d6c8596c60f3592fe557989a")
	add_subdirectory(tests/aulos)
	if(AULOS_AULOSPLAY)
		add_subdirectory(tests/aulosplay)
	endif()
endif()

if(AULOS_BENCHMARKS)
	add_subdirectory(benchmark)
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(AulosConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/AulosConfig.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Aulos)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/AulosConfigVersion.cmake COMPATIBILITY SameMinorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/AulosConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/AulosConfigVersion.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Aulos)
install(EXPORT AulosTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Aulos NAMESPACE Aulos::)
