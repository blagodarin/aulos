# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15)

project(Aulos VERSION 0.0.5.0 HOMEPAGE_URL https://github.com/blagodarin/aulos LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(Primal GIT_REPOSITORY https://github.com/blagodarin/primal.git GIT_TAG 6940f5f3a204b855b525cc7bb1e4b41936aac855)
FetchContent_MakeAvailable(Primal)

primal_defaults()

set(CMAKE_CXX_STANDARD 20)

option(PUBLIC_BUILD "Build for public distribution" OFF)
option(QT_VERSION "Qt version to use (5 or 6)" "5")
option(USE_SANITIZERS "Use sanitizers" OFF)
option(WITH_AULOSPLAY "Build aulosplay library" OFF)
option(WITH_BENCHMARK "Build benchmark" OFF)
option(WITH_STANDALONE "Build standalone library variants" OFF)
option(WITH_STUDIO "Build Aulos Studio (requires Qt)" OFF)
option(WITH_STUDIO_INSTALLER "Build Aulos Studio installer (requires NSIS)" OFF)
option(WITH_TESTS "Build tests" OFF)

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
		)
endif()

if(USE_SANITIZERS)
	if(CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|GNU)$")
		add_compile_options(-fsanitize=address -fsanitize=undefined)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")
	endif()
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

if(WITH_AULOSPLAY OR WITH_STUDIO)
	find_package(Threads REQUIRED)
	if(NOT WIN32)
		find_package(ALSA REQUIRED)
	endif()
	add_subdirectory(aulosplay)
endif()

if(WITH_TESTS)
	enable_testing()
	primal_download("https://raw.githubusercontent.com/onqtam/doctest/2.4.4/doctest/doctest.h" SHA1 "7c25b870f3e0cde6d6c8596c60f3592fe557989a")
	add_subdirectory(tests/aulos)
	add_subdirectory(tests/aulosplay)
endif()

if(WITH_BENCHMARK)
	add_subdirectory(benchmark)
endif()

if(WITH_STUDIO)
	find_package(Qt${QT_VERSION} COMPONENTS Widgets REQUIRED)
	message(STATUS "Using Qt ${Qt${QT_VERSION}_VERSION}")
	add_subdirectory(studio)
	if(WITH_STUDIO_INSTALLER)
		add_subdirectory(studio/installer)
	endif()
endif()

include(CMakePackageConfigHelpers)
configure_package_config_file(AulosConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/AulosConfig.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Aulos)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/AulosConfigVersion.cmake COMPATIBILITY SameMinorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/AulosConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/AulosConfigVersion.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Aulos)
install(EXPORT AulosTargets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Aulos NAMESPACE Aulos::)
