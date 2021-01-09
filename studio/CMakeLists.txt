# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

source_group("qt" REGULAR_EXPRESSION "qrc_.+\\.cpp\\.rule")
source_group("res" REGULAR_EXPRESSION "/res/")
source_group("src" REGULAR_EXPRESSION "/src/")
source_group("src\\composition" REGULAR_EXPRESSION "/src/composition/")
source_group("src\\sequence" REGULAR_EXPRESSION "/src/sequence/")
add_executable(studio WIN32
	res/studio.qrc
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
	src/voice_widget.cpp
	src/voice_widget.hpp
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
	src/composition/loop_item.cpp
	src/composition/loop_item.hpp
	src/composition/timeline_item.cpp
	src/composition/timeline_item.hpp
	src/composition/track_item.cpp
	src/composition/track_item.hpp
	src/composition/voice_editor.cpp
	src/composition/voice_editor.hpp
	src/composition/voice_item.cpp
	src/composition/voice_item.hpp
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
target_compile_options(studio PRIVATE ${STUDIO_WARNINGS})
target_include_directories(studio PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(studio PRIVATE aulos)
if(QT_VERSION EQUAL 6 OR WIN32)
	target_link_libraries(studio PRIVATE Qt${QT_VERSION}::Widgets)
else()
	target_link_libraries(studio PRIVATE Qt${QT_VERSION}::Multimedia Qt${QT_VERSION}::Widgets)
endif()
set_target_properties(studio PROPERTIES AUTOMOC ON AUTORCC ON AUTOUIC ON)
if(WIN32)
	target_sources(studio PRIVATE
		res/studio.rc
		src/player_wasapi.cpp
		src/player_wasapi.hpp
		)
	set_property(TARGET studio PROPERTY OUTPUT_NAME AulosStudio)
else()
	set_property(TARGET studio PROPERTY OUTPUT_NAME aulos_studio)
endif()

install_target(studio)
