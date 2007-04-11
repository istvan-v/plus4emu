;NSIS Modern User Interface
;Basic Example Script
;Written by Joost Verburg

  SetCompressor /SOLID /FINAL LZMA

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"

;--------------------------------
;General

  ;Name and file
  Name "plus4emu"
  OutFile "plus4emu-2.0-beta.exe"

  ;Default installation folder
  InstallDir "$PROGRAMFILES\plus4emu"

  ;Get installation folder from registry if available
  InstallDirRegKey HKCU "Software\plus4emu\InstallDirectory" ""

;--------------------------------
;Variables

  Var MUI_TEMP
  Var STARTMENU_FOLDER

;--------------------------------
;Interface Settings

  !define MUI_ABORTWARNING

;--------------------------------
;Pages

  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  ;Start Menu Folder Page Configuration
  !define MUI_STARTMENUPAGE_REGISTRY_ROOT "HKCU"
  !define MUI_STARTMENUPAGE_REGISTRY_KEY "Software\plus4emu"
  !define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Start Menu Folder"
  !insertmacro MUI_PAGE_STARTMENU Application $STARTMENU_FOLDER
  !insertmacro MUI_PAGE_INSTFILES

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Installer Sections

Section "plus4emu" SecMain

  SectionIn RO

  SetOutPath "$INSTDIR"

  File /nonfatal "..\LICENSE.FLTK"
  File /nonfatal "..\LICENSE.PortAudio"
  File /nonfatal "..\LICENSE.dotconf"
  File /nonfatal "..\LICENSE.libsndfile"
  File "/oname=readme.txt" "..\README"
  File "D:\MinGW\bin\dotconf.dll"
  File "D:\MinGW\bin\fltk.dll"
  File "..\plus4emu.exe"
  File "D:\MinGW\bin\libsndfile-1.dll"
  File "..\makecfg.exe"
  File "D:\MinGW\bin\mingwm10.dll"
  File "D:\MinGW\bin\portaudio.dll.0.0.19"
  File "..\tapconv.exe"

  SetOutPath "$INSTDIR\config"

  SetOutPath "$INSTDIR\demo"

  SetOutPath "$INSTDIR\disk"

  File "..\disk\disk.zip"

  SetOutPath "$INSTDIR\progs"

  SetOutPath "$INSTDIR\roms"

  File "..\roms\3plus1hi.rom"
  File "..\roms\3plus1lo.rom"
  File "..\roms\dos1541.rom"
  File "..\roms\dos1581.rom"
  File "..\roms\p4_basic.rom"
  File "..\roms\p4fileio.rom"
  File "..\roms\p4kernal.rom"
  File "..\roms\p4_ntsc.rom"

  SetOutPath "$INSTDIR\tape"

  ;Store installation folder
  WriteRegStr HKCU "Software\plus4emu\InstallDirectory" "" $INSTDIR

  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_BEGIN Application

    ;Create shortcuts
    CreateDirectory "$SMPROGRAMS\$STARTMENU_FOLDER"
    SetOutPath "$INSTDIR"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\plus4emu - OpenGL mode.lnk" "$INSTDIR\plus4emu.exe" '-opengl'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\plus4emu - software mode.lnk" "$INSTDIR\plus4emu.exe" '-no-opengl'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\README.lnk" "$INSTDIR\readme.txt"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Reinstall configuration files.lnk" "$INSTDIR\makecfg.exe" '"$INSTDIR"'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_END

  ExecWait '"$INSTDIR\makecfg.exe" "$INSTDIR"'

SectionEnd

Section "Associate .prg files with plus4emu" SecAssoc

  WriteRegStr HKCR ".prg" "" "Plus4Emu.PRGFile"
  WriteRegStr HKCR "Plus4Emu.PRGFile" "" "Plus/4 program"
  WriteRegStr HKCR "Plus4Emu.PRGFile\DefaultIcon" "" "$INSTDIR\plus4emu.exe,0"
  WriteRegStr HKCR "Plus4Emu.PRGFile\shell" "" "open"
  WriteRegStr HKCR "Plus4Emu.PRGFile\shell\open\command" "" '"$INSTDIR\plus4emu.exe" -prg "%1"'

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecMain ${LANG_ENGLISH} "plus4emu binaries"
  LangString DESC_SecAssoc ${LANG_ENGLISH} "Associate .prg files with plus4emu"

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} $(DESC_SecMain)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAssoc} $(DESC_SecAssoc)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  Delete "$INSTDIR\LICENSE.FLTK"
  Delete "$INSTDIR\LICENSE.PortAudio"
  Delete "$INSTDIR\LICENSE.dotconf"
  Delete "$INSTDIR\LICENSE.libsndfile"
  Delete "$INSTDIR\readme.txt"
  Delete "$INSTDIR\dotconf.dll"
  Delete "$INSTDIR\fltk.dll"
  Delete "$INSTDIR\plus4emu.exe"
  Delete "$INSTDIR\libsndfile-1.dll"
  Delete "$INSTDIR\makecfg.exe"
  Delete "$INSTDIR\mingwm10.dll"
  Delete "$INSTDIR\portaudio.dll.0.0.19"
  Delete "$INSTDIR\tapconv.exe"
  Delete "$INSTDIR\config\P4_Keyboard_US.cfg"
  Delete "$INSTDIR\config\P4_Keyboard_HU.cfg"
  Delete "$INSTDIR\config\P4_16k_PAL.cfg"
  Delete "$INSTDIR\config\P4_16k_NTSC.cfg"
  Delete "$INSTDIR\config\P4_64k_PAL.cfg"
  Delete "$INSTDIR\config\P4_64k_NTSC.cfg"
  Delete "$INSTDIR\config\P4_16k_PAL_3PLUS1.cfg"
  Delete "$INSTDIR\config\P4_16k_NTSC_3PLUS1.cfg"
  Delete "$INSTDIR\config\P4_64k_PAL_3PLUS1.cfg"
  Delete "$INSTDIR\config\P4_64k_NTSC_3PLUS1.cfg"
  Delete "$INSTDIR\config\P4_16k_PAL_FileIO.cfg"
  Delete "$INSTDIR\config\P4_16k_NTSC_FileIO.cfg"
  Delete "$INSTDIR\config\P4_64k_PAL_FileIO.cfg"
  Delete "$INSTDIR\config\P4_64k_NTSC_FileIO.cfg"
  Delete "$INSTDIR\config\P4_16k_PAL_3PLUS1_FileIO.cfg"
  Delete "$INSTDIR\config\P4_16k_NTSC_3PLUS1_FileIO.cfg"
  Delete "$INSTDIR\config\P4_64k_PAL_3PLUS1_FileIO.cfg"
  Delete "$INSTDIR\config\P4_64k_NTSC_3PLUS1_FileIO.cfg"
  RMDir "$INSTDIR\config"
  RMDir "$INSTDIR\demo"
  RMDir "$INSTDIR\disk"
  RMDir "$INSTDIR\progs"
  Delete "$INSTDIR\roms\3plus1hi.rom"
  Delete "$INSTDIR\roms\3plus1lo.rom"
  Delete "$INSTDIR\roms\dos1541.rom"
  Delete "$INSTDIR\roms\dos1581.rom"
  Delete "$INSTDIR\roms\p4_basic.rom"
  Delete "$INSTDIR\roms\p4fileio.rom"
  Delete "$INSTDIR\roms\p4kernal.rom"
  Delete "$INSTDIR\roms\p4_ntsc.rom"
  RMDir "$INSTDIR\roms"
  RMDir /r "$INSTDIR\src"
  RMDir "$INSTDIR\tape"

  !insertmacro MUI_STARTMENU_GETFOLDER Application $MUI_TEMP

  Delete "$SMPROGRAMS\$MUI_TEMP\plus4emu - OpenGL mode.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\plus4emu - software mode.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\README.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Reinstall configuration files.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\Uninstall.lnk"

  ;Delete empty start menu parent diretories
  StrCpy $MUI_TEMP "$SMPROGRAMS\$MUI_TEMP"

  startMenuDeleteLoop:
    ClearErrors
    RMDir $MUI_TEMP
    GetFullPathName $MUI_TEMP "$MUI_TEMP\.."

    IfErrors startMenuDeleteLoopDone

    StrCmp $MUI_TEMP $SMPROGRAMS startMenuDeleteLoopDone startMenuDeleteLoop
  startMenuDeleteLoopDone:

  DeleteRegKey /ifempty HKCU "Software\plus4emu\InstallDirectory"
  DeleteRegKey /ifempty HKCU "Software\plus4emu"

  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"

SectionEnd

