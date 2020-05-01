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

set(_include
	data.hpp
	playback.hpp
	)
source_group("include" FILES ${_include})

set(_src
	src/composition.cpp
	src/composition.hpp
	src/modulator.hpp
	src/note_table.hpp
	src/oscillator.hpp
	src/renderer.cpp
	src/shaper.hpp
	src/voice.hpp
	src/wave.hpp
	)
source_group("src" FILES ${_src})

add_library(aulos STATIC ${_include} ${_src})
target_compile_definitions(aulos PRIVATE _USE_MATH_DEFINES)
target_compile_options(aulos PRIVATE ${MAX_WARNINGS})
target_include_directories(aulos PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>)

install(FILES ${_include} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/aulos)
install(TARGETS aulos EXPORT AulosTargets
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
