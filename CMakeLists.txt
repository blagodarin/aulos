# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.21)

project(Aulos VERSION 0.0.5.0 HOMEPAGE_URL https://github.com/blagodarin/aulos LANGUAGES CXX)

include(CMakeDependentOption)
include(FetchContent)

option(AULOS_STUDIO_INSTALLER "Build Aulos Studio installer (requires NSIS)")
option(AULOS_STUDIO_QT6 "Build Aulos Studio with Qt 6")
option(AULOS_STUDIO_RELEASE "Produce release version of Aulos Studio")

set(SEIR_AUDIO ON)
set(SEIR_STATIC_RUNTIME OFF)
set(SEIR_SYNTH ON)
FetchContent_Declare(Seir GIT_REPOSITORY https://github.com/blagodarin/seir.git GIT_TAG 9ec775fc343c5a0363e65c316d9e67802d66df4f)
FetchContent_MakeAvailable(Seir)
seir_defaults()

set_property(GLOBAL PROPERTY AUTOGEN_SOURCE_GROUP "gen")

# Qt headers don't compile with high warning levels. :(
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	add_compile_options(-Werror -Wall -Wextra
		-Wno-deprecated-enum-enum-conversion
		)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	add_compile_options(-Werror -Wall -Wextra)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	add_compile_options(/WX /W4
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
