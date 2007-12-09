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
  OutFile "plus4emu-1.2.5.exe"

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

  !insertmacro MUI_PAGE_LICENSE "..\COPYING"
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

  File "..\COPYING"
  File /nonfatal "..\LICENSE.FLTK"
  File /nonfatal "..\LICENSE.Lua"
  File /nonfatal "..\LICENSE.PortAudio"
  File /nonfatal "..\LICENSE.SDL"
  File /nonfatal "..\LICENSE.dotconf"
  File /nonfatal "..\LICENSE.libsndfile"
  File "/oname=news.txt" "..\NEWS"
  File "/oname=readme.txt" "..\README"
  File "..\plus4emu.exe"
  File "C:\MinGW\bin\libsndfile-1.dll"
  File "C:\MinGW\bin\lua51.dll"
  File "..\makecfg.exe"
  File "C:\MinGW\bin\mingwm10.dll"
  File "C:\MinGW\bin\portaudio.dll.0.0.19"
  File "C:\MinGW\bin\SDL.dll"
  File "..\tapconv.exe"

  SetOutPath "$INSTDIR\config"

  SetOutPath "$INSTDIR\demo"

  SetOutPath "$INSTDIR\disk"

  File "..\disk\disk.zip"

  SetOutPath "$INSTDIR\progs"

  SetOutPath "$INSTDIR\roms"

  File "..\roms\p4fileio.rom"

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
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\plus4emu - GL - Win2000 theme.lnk" "$INSTDIR\plus4emu.exe" '-opengl' '-colorscheme 1'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\plus4emu - GL - plastic theme.lnk" "$INSTDIR\plus4emu.exe" '-opengl' '-colorscheme 2'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\plus4emu - software mode.lnk" "$INSTDIR\plus4emu.exe" '-no-opengl'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\README.lnk" "$INSTDIR\readme.txt"
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Reinstall configuration files.lnk" "$INSTDIR\makecfg.exe" '"$INSTDIR"'
    CreateShortCut "$SMPROGRAMS\$STARTMENU_FOLDER\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

  !insertmacro MUI_STARTMENU_WRITE_END

  ExecWait '"$INSTDIR\makecfg.exe" "$INSTDIR"'

SectionEnd

Section "Source code" SecSrc

  SetOutPath "$INSTDIR\src"

  File "..\COPYING"
  File "..\NEWS"
  File "..\README"
  File "..\SConstruct"
  File "..\*.patch"
  File "..\*.sh"

  SetOutPath "$INSTDIR\src\config"

  File "..\config\*.cfg"

  SetOutPath "$INSTDIR\src\disk"

  File "..\disk\*.zip"

  SetOutPath "$INSTDIR\src\gui"

  File "..\gui\*.fl"
  File "..\gui\gui.cpp"
  File "..\gui\gui.hpp"
  File "..\gui\main.cpp"
  File "..\gui\monitor.cpp"
  File "..\gui\monitor.hpp"

  SetOutPath "$INSTDIR\src\installer"

  File "..\installer\*.nsi"
  File "..\installer\*.fl"
  File "..\installer\makecfg.cpp"

  SetOutPath "$INSTDIR\src\msvc"

  SetOutPath "$INSTDIR\src\msvc\include"

  File "..\msvc\include\*.h"

  SetOutPath "$INSTDIR\src\plus4emu.app"

  SetOutPath "$INSTDIR\src\plus4emu.app\Contents"

  File "..\plus4emu.app\Contents\Info.plist"
  File "..\plus4emu.app\Contents\PkgInfo"

  SetOutPath "$INSTDIR\src\plus4emu.app\Contents\MacOS"

  SetOutPath "$INSTDIR\src\plus4emu.app\Contents\Resources"

  File "..\plus4emu.app\Contents\Resources\plus4emu.icns"

  SetOutPath "$INSTDIR\src\resid"

  File "..\resid\AUTHORS"
  File "..\resid\COPYING"
  File "..\resid\ChangeLog"
  File "..\resid\NEWS"
  File "..\resid\README"
  File "..\resid\THANKS"
  File "..\resid\TODO"
  File "..\resid\*.cpp"
  File "..\resid\*.hpp"

  SetOutPath "$INSTDIR\src\roms"

  File "..\roms\*.rom"

  SetOutPath "$INSTDIR\src\src"

  File "..\src\*.cpp"
  File "..\src\*.hpp"
  File "..\src\*.py"

  SetOutPath "$INSTDIR\src\util"

  File "..\util\*.c"
  File "..\util\*.cpp"

SectionEnd

Section "Associate .prg, .p00, .d64, and .d81 files with plus4emu" SecAssoc

  WriteRegStr HKCR ".prg" "" "Plus4Emu.PRGFile"
  WriteRegStr HKCR ".p00" "" "Plus4Emu.PRGFile"
  WriteRegStr HKCR "Plus4Emu.PRGFile" "" "Plus/4 program"
  WriteRegStr HKCR "Plus4Emu.PRGFile\DefaultIcon" "" "$INSTDIR\plus4emu.exe,0"
  WriteRegStr HKCR "Plus4Emu.PRGFile\shell" "" "open"
  WriteRegStr HKCR "Plus4Emu.PRGFile\shell\open\command" "" '"$INSTDIR\plus4emu.exe" -prg "%1"'

  WriteRegStr HKCR ".d64" "" "Plus4Emu.DiskFile"
  WriteRegStr HKCR ".d81" "" "Plus4Emu.DiskFile"
  WriteRegStr HKCR "Plus4Emu.DiskFile" "" "Plus/4 disk image"
  WriteRegStr HKCR "Plus4Emu.DiskFile\DefaultIcon" "" "$INSTDIR\plus4emu.exe,0"
  WriteRegStr HKCR "Plus4Emu.DiskFile\shell" "" "open"
  WriteRegStr HKCR "Plus4Emu.DiskFile\shell\open\command" "" '"$INSTDIR\plus4emu.exe" -disk "%1"'

SectionEnd

Section "Download ROM images" SecDLRoms

  Var /GLOBAL useZimmersNet
  Var /GLOBAL romFileName
  Var /GLOBAL altDLPath
  Var /GLOBAL romDLPath
  StrCpy $useZimmersNet "no"
  StrCpy $romFileName ""
  StrCpy $altDLPath ""
  StrCpy $romDLPath ""

  SetOutPath "$INSTDIR\roms"

  Push ""
  Push ""
  Push "p4_ntsc.rom"
  Push "firmware/computers/plus4/kernal.318005-05.bin"
  Push "p4kernal.rom"
  Push "firmware/computers/plus4/kernal.318004-05.bin"
  Push "p4_basic.rom"
  Push "firmware/computers/plus4/basic.318006-01.bin"
  Push "dos1581.rom"
  Push "firmware/drives/new/1581/1581-rom.318045-02.bin"
  Push "dos1551.rom"
  Push "firmware/computers/plus4/1551.318008-01.bin"
  Push "dos15412.rom"
  Push "firmware/drives/new/1541/1541-II.251968-03.bin"
  Push "dos1541.rom"
  Push "firmware/drives/new/1541/1541-II.251968-03.bin"
  Push "3plus1lo.rom"
  Push "firmware/computers/plus4/3-plus-1.317053-01.bin"
  Push "3plus1hi.rom"
  Push "firmware/computers/plus4/3-plus-1.317054-01.bin"
  Push "1526_mod.rom"
  Push "firmware/printers/1526/1526-07c.bin"
  Push "1526_07c.rom"
  Push "firmware/printers/1526/1526-07c.bin"

  downloadLoop:

    Pop $altDLPath
    Pop $romFileName
    StrCmp $romFileName "" downloadLoopDone 0

  setROMFileDLPath:

    StrCmp $useZimmersNet "yes" setDLPath2 0
    StrCpy $romDLPath "http://www.sharemation.com/IstvanV/roms/$romFileName"
    Goto dlROMFile

  setDLPath2:

    StrCpy $romDLPath "http://www.zimmers.net/anonftp/pub/cbm/$altDLPath"

  dlROMFile:

    NSISdl::download "$romDLPath" "$INSTDIR\roms\$romFileName"
    Pop $R0
    StrCmp $R0 "success" downloadLoop 0
    StrCmp $R0 "cancel" downloadLoop 0
    StrCmp $useZimmersNet "no" tryZimmersNet 0
    MessageBox MB_OK "Download failed: $R0"
    Goto downloadLoop

  tryZimmersNet:

    MessageBox MB_OK "WARNING: download from www.sharemation.com failed ($R0), using www.zimmers.net instead"
    StrCpy $useZimmersNet "yes"
    Goto setROMFileDLPath

  downloadLoopDone:

SectionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecMain ${LANG_ENGLISH} "plus4emu binaries"
  LangString DESC_SecSrc ${LANG_ENGLISH} "plus4emu source code"
  LangString DESC_SecAssoc ${LANG_ENGLISH} "Associate .prg, .p00, .d64, and .d81 files with plus4emu"
  LangString DESC_SecDLRoms ${LANG_ENGLISH} "Download and install ROM images"

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMain} $(DESC_SecMain)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecSrc} $(DESC_SecSrc)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAssoc} $(DESC_SecAssoc)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecDLRoms} $(DESC_SecDLRoms)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  Delete "$INSTDIR\COPYING"
  Delete "$INSTDIR\LICENSE.FLTK"
  Delete "$INSTDIR\LICENSE.Lua"
  Delete "$INSTDIR\LICENSE.PortAudio"
  Delete "$INSTDIR\LICENSE.SDL"
  Delete "$INSTDIR\LICENSE.dotconf"
  Delete "$INSTDIR\LICENSE.libsndfile"
  Delete "$INSTDIR\news.txt"
  Delete "$INSTDIR\readme.txt"
  Delete "$INSTDIR\plus4emu.exe"
  Delete "$INSTDIR\libsndfile-1.dll"
  Delete "$INSTDIR\lua51.dll"
  Delete "$INSTDIR\makecfg.exe"
  Delete "$INSTDIR\mingwm10.dll"
  Delete "$INSTDIR\portaudio.dll.0.0.19"
  Delete "$INSTDIR\SDL.dll"
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
  Delete "$INSTDIR\disk\disk.zip"
  RMDir "$INSTDIR\disk"
  RMDir "$INSTDIR\progs"
  Delete "$INSTDIR\roms\1526_07c.rom"
  Delete "$INSTDIR\roms\1526_mod.rom"
  Delete "$INSTDIR\roms\3plus1hi.rom"
  Delete "$INSTDIR\roms\3plus1lo.rom"
  Delete "$INSTDIR\roms\dos1541.rom"
  Delete "$INSTDIR\roms\dos15412.rom"
  Delete "$INSTDIR\roms\dos1551.rom"
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
  Delete "$SMPROGRAMS\$MUI_TEMP\plus4emu - GL - Win2000 theme.lnk"
  Delete "$SMPROGRAMS\$MUI_TEMP\plus4emu - GL - plastic theme.lnk"
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

