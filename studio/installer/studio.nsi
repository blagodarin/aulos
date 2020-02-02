;
; This file is part of the Aulos toolkit.
; Copyright (C) 2020 Sergei Blagodarin.
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.
;

!include "MUI2.nsh"

Name "Aulos Studio"
OutFile "${INSTALLER_PATH}"
Unicode True
InstallDir "$PROGRAMFILES64\Aulos Studio"
RequestExecutionLevel admin
SetCompressor /SOLID /FINAL lzma

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\AulosStudio.exe"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${SOURCE_DIR}\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Russian"

!if ${CONFIG} == Debug
	!define SUFFIX d
!else
	!define SUFFIX
!endif

Section
	SetOutPath "$INSTDIR"
	WriteUninstaller "$INSTDIR\Uninstall.exe"

	File /nonfatal "${BUILD_DIR}\bin\AulosStudio.exe"
	File /nonfatal "${BUILD_DIR}\bin\${CONFIG}\AulosStudio.exe"
	File "$%QTDIR%\bin\Qt5Core${SUFFIX}.dll"
	File "$%QTDIR%\bin\Qt5Gui${SUFFIX}.dll"
	File "$%QTDIR%\bin\Qt5Multimedia${SUFFIX}.dll"
	File "$%QTDIR%\bin\Qt5Network${SUFFIX}.dll"
	File "$%QTDIR%\bin\Qt5Widgets${SUFFIX}.dll"

	SetOutPath "$INSTDIR\plugins\audio"
	File "$%QTDIR%\plugins\audio\qtaudio_windows${SUFFIX}.dll"

	SetOutPath "$INSTDIR\plugins\platforms"
	File "$%QTDIR%\plugins\platforms\qwindows${SUFFIX}.dll"

	SetOutPath "$INSTDIR\plugins\styles"
	File "$%QTDIR%\plugins\styles\qwindowsvistastyle${SUFFIX}.dll"

	SetOutPath "$INSTDIR\examples"
	File "${SOURCE_DIR}\examples\*.aulos"
SectionEnd

Section "Uninstall"
	RMDir /r "$INSTDIR"
SectionEnd
