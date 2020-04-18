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

download_file("https://raw.githubusercontent.com/onqtam/doctest/2.3.7/doctest/doctest.h" SHA1 "7b361ca6edac7cb1b9b894dcaaf925b2c9674fef")

set(_src
	src/main.cpp
	)
source_group("src" FILES ${_src})

add_executable(tests ${_src})
target_include_directories(tests PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(tests PRIVATE aulos)
