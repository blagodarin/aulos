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

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(_res
	res/studio.qrc
	)
if(WIN32)
	list(APPEND _res res/studio.rc)
endif()
source_group("res" FILES ${_res})

set(_src
	src/add_time_item.cpp
	src/add_time_item.hpp
	src/add_voice_item.cpp
	src/add_voice_item.hpp
	src/button_item.cpp
	src/button_item.hpp
	src/colors.cpp
	src/colors.hpp
	src/composition_scene.cpp
	src/composition_scene.hpp
	src/cursor_item.cpp
	src/cursor_item.hpp
	src/fragment_item.cpp
	src/fragment_item.hpp
	src/info_editor.cpp
	src/info_editor.hpp
	src/key_item.cpp
	src/key_item.hpp
	src/main.cpp
	src/pianoroll_item.cpp
	src/pianoroll_item.hpp
	src/player.cpp
	src/player.hpp
	src/sequence_editor.cpp
	src/sequence_editor.hpp
	src/sequence_scene.cpp
	src/sequence_scene.hpp
	src/sound_item.cpp
	src/sound_item.hpp
	src/studio.cpp
	src/studio.hpp
	src/timeline_item.cpp
	src/timeline_item.hpp
	src/track_editor.cpp
	src/track_editor.hpp
	src/track_item.cpp
	src/track_item.hpp
	src/utils.cpp
	src/utils.hpp
	src/voice_editor.cpp
	src/voice_editor.hpp
	src/voice_item.cpp
	src/voice_item.hpp
	)
source_group("src" FILES ${_src})

add_executable(studio WIN32 ${_res} ${_src})
target_compile_options(studio PRIVATE /wd5054)
target_link_libraries(studio PRIVATE aulos Qt5::Multimedia Qt5::Widgets)
if(WIN32)
	set_property(TARGET studio PROPERTY OUTPUT_NAME AulosStudio)
else()
	set_property(TARGET studio PROPERTY OUTPUT_NAME aulos_studio)
endif()

install(TARGETS studio RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(WITH_STUDIO_INSTALLER)
	add_subdirectory(installer)
endif()
