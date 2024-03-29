# This file is part of the Aulos toolkit.
# Copyright (C) Sergei Blagodarin.
# SPDX-License-Identifier: Apache-2.0

find_program(NSIS_EXECUTABLE makensis.exe PATHS [HKEY_LOCAL_MACHINE\\Software\\NSIS])
if(NSIS_EXECUTABLE MATCHES ".*-NOTFOUND$")
	message(FATAL_ERROR "Can't find NSIS executable")
endif()
seir_download("https://aka.ms/vs/16/release/vc_redist.x64.exe")
if(AULOS_STUDIO_RELEASE)
	set(STUDIO_INSTALLER_NAME AulosStudio-${AULOS_VERSION}.exe)
else()
	set(STUDIO_INSTALLER_NAME AulosStudio-${AULOS_VERSION}-unstable.exe)
endif()
get_filename_component(QTDIR "${${AULOS_QT}_DIR}" PATH)
get_filename_component(QTDIR "${QTDIR}" PATH)
get_filename_component(QTDIR "${QTDIR}" PATH)
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/${STUDIO_INSTALLER_NAME}
	COMMAND ${NSIS_EXECUTABLE} /INPUTCHARSET UTF8 /NOCD /V2
		/DAULOS_RC_VERSION=${PROJECT_VERSION}
		/DAULOS_VERSION=${AULOS_VERSION}
		/DBUILD_DIR=$<TARGET_FILE_DIR:studio>
		/DCONFIG=$<CONFIG>
		/DDOWNLOAD_DIR=${SEIR_DOWNLOAD_DIR}
		/DINSTALLER_DIR=${PROJECT_BINARY_DIR}
		/DINSTALLER_NAME=${STUDIO_INSTALLER_NAME}
		/DQT_PREFIX=${AULOS_QT}
		/DQTDIR=${QTDIR}
		/DSOURCE_DIR=${PROJECT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/studio.nsi
	DEPENDS studio
	MAIN_DEPENDENCY studio.nsi
	VERBATIM
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
add_custom_target(studio_installer DEPENDS ${PROJECT_BINARY_DIR}/${STUDIO_INSTALLER_NAME})
