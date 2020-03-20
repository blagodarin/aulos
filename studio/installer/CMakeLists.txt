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

function(download_file _url)
	string(REGEX REPLACE "^.*/([^/]+)$" "\\1" _name ${_url})
	set(_path ${PROJECT_BINARY_DIR}/${_name})
	if(NOT EXISTS ${_path})
		message(STATUS "Downloading ${_name}")
		file(DOWNLOAD ${_url} ${_path} TLS_VERIFY ON)
	endif()
endfunction()

find_program(NSIS_EXECUTABLE makensis.exe PATHS [HKEY_LOCAL_MACHINE\\Software\\NSIS])
if(NSIS_EXECUTABLE MATCHES ".*-NOTFOUND$")
	message(FATAL_ERROR "Can't find NSIS executable")
endif()
download_file("https://aka.ms/vs/16/release/vc_redist.x64.exe")
if(PUBLIC_BUILD)
	set(STUDIO_INSTALLER_NAME AulosStudio-${AULOS_VERSION}.exe)
else()
	set(STUDIO_INSTALLER_NAME AulosStudio-${AULOS_VERSION}-unstable.exe)
endif()
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/${STUDIO_INSTALLER_NAME}
	COMMAND ${NSIS_EXECUTABLE} /INPUTCHARSET UTF8 /NOCD /V2
		/DAULOS_RC_VERSION=${PROJECT_VERSION}
		/DAULOS_VERSION=${AULOS_VERSION}
		/DBUILD_DIR=$<TARGET_FILE_DIR:studio>
		/DCONFIG=$<CONFIG>
		/DINSTALLER_DIR=${PROJECT_BINARY_DIR}
		/DINSTALLER_NAME=${STUDIO_INSTALLER_NAME}
		/DSOURCE_DIR=${PROJECT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/studio.nsi
	DEPENDS studio
	MAIN_DEPENDENCY studio.nsi
	VERBATIM
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
add_custom_target(studio_installer DEPENDS ${PROJECT_BINARY_DIR}/${STUDIO_INSTALLER_NAME})
