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

find_program(NSIS_EXECUTABLE makensis.exe PATHS [HKEY_LOCAL_MACHINE\\Software\\NSIS])
if(NSIS_EXECUTABLE MATCHES ".*-NOTFOUND$")
	message(FATAL_ERROR "Can't find NSIS executable")
endif()
set(STUDIO_INSTALLER_PATH ${PROJECT_BINARY_DIR}\\AulosStudio-${PROJECT_VERSION}.exe)
add_custom_command(OUTPUT ${STUDIO_INSTALLER_PATH}
	COMMAND ${NSIS_EXECUTABLE} /INPUTCHARSET UTF8 /NOCD /V2
		/DBUILD_DIR=$<TARGET_FILE_DIR:studio>
		/DCONFIG=$<CONFIG>
		/DINSTALLER_PATH=${STUDIO_INSTALLER_PATH}
		/DPRODUCT_VERSION=${PROJECT_VERSION}
		/DSOURCE_DIR=${PROJECT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}\\studio.nsi
	DEPENDS studio
	MAIN_DEPENDENCY studio.nsi
	VERBATIM
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
add_custom_target(studio_installer DEPENDS ${STUDIO_INSTALLER_PATH})
