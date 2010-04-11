
;!include "MUI2.nsh"
;!define MUI_ICON "..\images\gc.ico" ;Value

  
Name "Golden Cheetah"

Icon "..\images\gc.ico"

SetCompressor /SOLID lzma

OutFile "GoldenCheetahInstall.exe"

InstallDir $PROGRAMFILES\GoldenCheetah

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\GoldenCheetah" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------

; Pages

  ;!insertmacro MUI_PAGE_WELCOME
  ;!insertmacro MUI_PAGE_LICENSE "${NSISDIR}\Docs\Modern UI\License.txt"
  ;!insertmacro MUI_PAGE_COMPONENTS
  ;!insertmacro MUI_PAGE_DIRECTORY
  ;!insertmacro MUI_PAGE_INSTFILES
  
  ;!insertmacro MUI_UNPAGE_CONFIRM
  ;!insertmacro MUI_UNPAGE_INSTFILES
  
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------
;Languages
 
  ;!insertmacro MUI_LANGUAGE "English"
;--------------------------------

; The stuff to install
Section "Golden Cheetah (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "..\release\GoldenCheetah.exe"
  ;File "..\release\qwt5.dll"
  File "..\release\mingwm10.dll"
  File "..\release\QtCore4.dll"
  File "..\release\QtGui4.dll"
  File "..\release\QtSql4.dll"
  File "..\release\QtXml4.dll"
  File "..\release\QtWebKit4.dll"
  File "..\release\QtXmlPatterns4.dll"
  File "..\release\phonon4.dll"
  File "..\release\QtNetwork4.dll"
  File "..\release\QtOpenGL4.dll"
  File "..\release\libgcc_s_dw2-1.dll"
  File "..\release\qwtplot3d.dll"

  SetOutPath $INSTDIR\sqldrivers
  File "..\release\qsqlite4.dll"

  SetOutPath $INSTDIR\imageformats
  File "..\release\qjpeg4.dll"

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\GoldenCheetah "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GoldenCheetah" "DisplayName" "Golden Cheetah"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GoldenCheetah" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GoldenCheetah" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GoldenCheetah" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\GoldenCheetah"
  CreateShortCut "$SMPROGRAMS\GoldenCheetah\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\GoldenCheetah\Golden Cheetah.lnk" "$INSTDIR\GoldenCheetah.exe" "" "$INSTDIR\GoldenCheetah.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\GoldenCheetah"
  DeleteRegKey HKLM SOFTWARE\GoldenCheetah

  ; Remove files and uninstaller
  Delete $INSTDIR\GoldenCheetah.exe

  ; we continue to delete qwt5.dll becvause people don't always uninstall before they install
  ;; so a subsequent uninstall still should remove it if it is there
  Delete $INSTDIR\qwt5.dll
  Delete $INSTDIR\mingwm10.dll
  Delete $INSTDIR\QtCore4.dll
  Delete $INSTDIR\QtGui4.dll
  Delete $INSTDIR\QtSql4.dll
  Delete $INSTDIR\QtXml4.dll
  Delete $INSTDIR\QtWebKit4.dll
  Delete $INSTDIR\QtXmlPatterns4.dll
  Delete $INSTDIR\phonon4.dll
  Delete $INSTDIR\QtNetwork4.dll
  Delete $INSTDIR\QtOpenGL4.dll
  Delete $INSTDIR\libgcc_s_dw2-1.dll
  Delete $INSTDIR\qwtplot3d.dll
  Delete $INSTDIR\uninstall.exe
  Delete $INSTDIR\sqldrivers\qsqlite4.dll
  Delete $INSTDIR\imageformats\qjpeg4.dll

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\GoldenCheetah\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\GoldenCheetah"
  RMDir "$INSTDIR"

SectionEnd
