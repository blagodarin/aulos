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
	src/button_item.cpp
	src/button_item.hpp

	src/elusive_item.cpp
	src/elusive_item.hpp
	src/info_editor.cpp
	src/info_editor.hpp
	src/main.cpp
	src/player.cpp
	src/player.hpp
	src/studio.cpp
	src/studio.hpp
	src/theme.cpp
	src/theme.hpp
	src/track_editor.cpp
	src/track_editor.hpp
	src/voice_editor.cpp
	src/voice_editor.hpp
	src/voice_widget.cpp
	src/voice_widget.hpp
	)
source_group("src" FILES ${_src})

set(_src_composition
	src/composition/add_voice_item.cpp
	src/composition/add_voice_item.hpp
	src/composition/composition_scene.cpp
	src/composition/composition_scene.hpp
	src/composition/composition_widget.cpp
	src/composition/composition_widget.hpp
	src/composition/cursor_item.cpp
	src/composition/cursor_item.hpp
	src/composition/fragment_item.cpp
	src/composition/fragment_item.hpp
	src/composition/timeline_item.cpp
	src/composition/timeline_item.hpp
	src/composition/track_item.cpp
	src/composition/track_item.hpp
	src/composition/voice_item.cpp
	src/composition/voice_item.hpp
	)
source_group("src\\composition" FILES ${_src_composition})

set(_src_sequence
	src/sequence/key_item.cpp
	src/sequence/key_item.hpp
	src/sequence/pianoroll_item.cpp
	src/sequence/pianoroll_item.hpp
	src/sequence/sequence_scene.cpp
	src/sequence/sequence_scene.hpp
	src/sequence/sequence_widget.cpp
	src/sequence/sequence_widget.hpp
	src/sequence/sound_item.cpp
	src/sequence/sound_item.hpp
	)
source_group("src\\sequence" FILES ${_src_sequence})

add_executable(studio WIN32 ${_res} ${_src} ${_src_composition} ${_src_sequence})
target_compile_options(studio PRIVATE /wd5054)
target_include_directories(studio PRIVATE ${PROJECT_BINARY_DIR})
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
