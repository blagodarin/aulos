#
# This file is part of the Aulos toolkit.
# Copyright (C) 2020 Sergei Blagodarin.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.15)

project(aulos VERSION 0.0.2.0 HOMEPAGE_URL https://github.com/blagodarin/aulos LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

set_property(GLOBAL PROPERTY AUTOGEN_SOURCE_GROUP "qt")
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ".cmake")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(PUBLIC_BUILD "Build for public distribution" OFF)
option(USE_SANITIZERS "Use sanitizers" OFF)
option(WITH_BENCHMARK "Build benchmark" OFF)
option(WITH_STUDIO "Build Aulos Studio (requires Qt)" OFF)
option(WITH_STUDIO_INSTALLER "Build Aulos Studio installer (requires NSIS)" OFF)
option(WITH_TESTS "Build tests" OFF)

# Qt headers don't compile with high warning levels,
# so a separate set of warning settings is needed.

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	add_compile_options(-Werror -Wall -Wextra)
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

set(AULOS_RC_VERSION "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},${PROJECT_VERSION_TWEAK}")
set(AULOS_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
configure_file(config.h.in ${PROJECT_BINARY_DIR}/aulos_config.h)

function(download_file _url)
	cmake_parse_arguments(_arg "" "SHA1" "" ${ARGN})
	string(REGEX REPLACE "^.*/([^/]+)$" "\\1" _name ${_url})
	set(_path ${PROJECT_BINARY_DIR}/${_name})
	if(EXISTS ${_path})
		return()
	endif()
	message(STATUS "Downloading ${_name}")
	if(_arg_SHA1)
		file(DOWNLOAD ${_url} ${_path} TLS_VERIFY ON EXPECTED_HASH SHA1=${_arg_SHA1})
	else()
		file(DOWNLOAD ${_url} ${_path} TLS_VERIFY ON)
	endif()
endfunction()

add_subdirectory(aulos)

if(WITH_TESTS)
	enable_testing()
	download_file("https://raw.githubusercontent.com/onqtam/doctest/2.3.7/doctest/doctest.h" SHA1 "7b361ca6edac7cb1b9b894dcaaf925b2c9674fef")
	add_subdirectory(tests)
endif()

if(WITH_BENCHMARK)
	add_subdirectory(benchmark)
endif()

if(WITH_STUDIO)
	find_package(Qt5 COMPONENTS Multimedia Widgets REQUIRED)
	add_subdirectory(studio)
endif()

if(WIN32)
	set(_cmake_config_dir cmake)
else()
	set(_cmake_config_dir ${CMAKE_INSTALL_LIBDIR}/cmake/Aulos)
endif()

configure_package_config_file(AulosConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/AulosConfig.cmake
	INSTALL_DESTINATION ${_cmake_config_dir})

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/AulosConfigVersion.cmake
	COMPATIBILITY SameMinorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/AulosConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/AulosConfigVersion.cmake DESTINATION ${_cmake_config_dir})
install(EXPORT AulosTargets NAMESPACE Aulos:: DESTINATION ${_cmake_config_dir})
