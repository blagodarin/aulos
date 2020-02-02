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

project(aulos VERSION 0.0.0 HOMEPAGE_URL https://github.com/blagodarin/aulos LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

set_property(GLOBAL PROPERTY AUTOGEN_SOURCE_GROUP "qt")
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ".cmake")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(WITH_STUDIO "Build Aulos Studio (requires Qt)" OFF)
option(WITH_STUDIO_INSTALLER "Build Aulos Studio installer (requires NSIS)" OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	add_compile_options(/std:c++latest /WX /W4)
endif()

add_subdirectory(aulos)

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
