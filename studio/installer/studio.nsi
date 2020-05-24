; This file is part of the Aulos toolkit.
; Copyright (C) Sergei Blagodarin.
; SPDX-License-Identifier: Apache-2.0

!include "FileFunc.nsh"
!include "LogicLib.nsh"
!include "MUI2.nsh"
!include "WordFunc.nsh"

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
VIAddVersionKey /LANG=0 "LegalCopyright" "© Sergei Blagodarin"
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
	ReadRegStr $0 HKLM64 "SOFTWARE\Microsoft\DevDiv\VC\Servicing\14.0\RuntimeMinimum" "Version"
	${VersionCompare} "$0" "14.24.28127" $1
	${If} $1 = 2
		SetOutPath "$INSTDIR"
		File "${INSTALLER_DIR}\vc_redist.x64.exe"
		ExecWait '"$INSTDIR\vc_redist.x64.exe" /install /passive /norestart'
		Delete "$INSTDIR\vc_redist.x64.exe"
	${EndIf}

	SetOutPath "$INSTDIR"
	WriteUninstaller "$INSTDIR\Uninstall.exe"
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
	File "${SOURCE_DIR}\examples\FurElise.aulos"
	File "${SOURCE_DIR}\examples\GrandeValseBrillante.aulos"
	File "${SOURCE_DIR}\examples\Popcorn.aulos"
	File "${SOURCE_DIR}\examples\PreludeInGMinor.aulos"

	SetShellVarContext all
	CreateShortcut "$DESKTOP\Aulos Studio.lnk" "$INSTDIR\AulosStudio.exe"
	CreateShortcut "$SMPROGRAMS\Aulos Studio.lnk" "$INSTDIR\AulosStudio.exe"

	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "DisplayIcon" "$INSTDIR\AulosStudio.exe"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "DisplayName" "Aulos Studio"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "DisplayVersion" "${AULOS_VERSION}"
	${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "EstimatedSize" $0
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "InstallLocation" "$INSTDIR"
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "NoModify" 1
	WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "NoRepair" 1
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "Publisher" "blagodarin.me"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "QuietUninstallString" "$\"$INSTDIR\Uninstall.exe$\" /S"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
SectionEnd

Section "Uninstall"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AulosStudio"

	SetShellVarContext all
	Delete "$DESKTOP\Aulos Studio.lnk"
	Delete "$SMPROGRAMS\Aulos Studio.lnk"

	Delete "$INSTDIR\examples\FurElise.aulos"
	Delete "$INSTDIR\examples\GrandeValseBrillante.aulos"
	Delete "$INSTDIR\examples\Popcorn.aulos"
	Delete "$INSTDIR\examples\PreludeInGMinor.aulos"
	RMDir "$INSTDIR\examples"

	Delete "$INSTDIR\plugins\styles\qwindowsvistastyle${SUFFIX}.dll"
	RMDir "$INSTDIR\plugins\styles"

	Delete "$INSTDIR\plugins\platforms\qwindows${SUFFIX}.dll"
	RMDir "$INSTDIR\plugins\platforms"

	Delete "$INSTDIR\plugins\audio\qtaudio_windows${SUFFIX}.dll"
	RMDir "$INSTDIR\plugins\audio"

	RMDir "$INSTDIR\plugins"

	Delete "$INSTDIR\AulosStudio.exe"
	Delete "$INSTDIR\Qt5Core${SUFFIX}.dll"
	Delete "$INSTDIR\Qt5Gui${SUFFIX}.dll"
	Delete "$INSTDIR\Qt5Multimedia${SUFFIX}.dll"
	Delete "$INSTDIR\Qt5Network${SUFFIX}.dll"
	Delete "$INSTDIR\Qt5Widgets${SUFFIX}.dll"
	Delete "$INSTDIR\Uninstall.exe"
	RMDir "$INSTDIR"
SectionEnd
