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
OutFile "${INSTALLER_DIR}\${INSTALLER_NAME}"
Unicode True
InstallDir "$PROGRAMFILES64\Aulos Studio"
RequestExecutionLevel admin
SetCompressor /SOLID /FINAL lzma

VIProductVersion ${AULOS_RC_VERSION}
VIAddVersionKey /LANG=0 "CompanyName"  "blagodarin.me"
VIAddVersionKey /LANG=0 "FileDescription" "Aulos Studio Installer"
VIAddVersionKey /LANG=0 "FileVersion" "${AULOS_VERSION}"
VIAddVersionKey /LANG=0 "InternalName" "AulosStudioInstaller"
VIAddVersionKey /LANG=0 "LegalCopyright" "© 2020 Sergei Blagodarin"
VIAddVersionKey /LANG=0 "OriginalFilename" "${INSTALLER_NAME}"
VIAddVersionKey /LANG=0 "ProductName" "Aulos Studio"
VIAddVersionKey /LANG=0 "ProductVersion" "${AULOS_VERSION}"

!define MUI_ABORTWARNING
!define MUI_ICON "${SOURCE_DIR}\studio\res\aulos.ico"
!define MUI_FINISHPAGE_RUN "$INSTDIR\AulosStudio.exe"
!define MUI_UNICON "${SOURCE_DIR}\studio\res\aulos.ico"

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
	File "${BUILD_DIR}\AulosStudio.exe"
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

	SetOutPath "$INSTDIR"
	WriteUninstaller "$INSTDIR\Uninstall.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "DisplayIcon" "$INSTDIR\AulosStudio.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "DisplayName" "Aulos Studio"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "DisplayVersion" "${AULOS_VERSION}"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "InstallLocation" "$INSTDIR"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "NoRepair" 1
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "Publisher" "blagodarin.me"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "QuietUninstallString" "$\"$INSTDIR\Uninstall.exe$\" /S"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
SectionEnd

Section "Uninstall"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio"
	RMDir /r "$INSTDIR"
SectionEnd
