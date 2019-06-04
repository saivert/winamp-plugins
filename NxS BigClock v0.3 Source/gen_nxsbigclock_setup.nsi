; Example installer script for plugin
; Written by Saivert

!include "MUI.nsh"

!define PLUGIN_NAME "NxS BigClock v0.3"
!define PLUGIN_INSTREGKEY "Software\Saivert\NSIS\NxSBigClock"

; Define the following to make the installer intercept multiple instances of itself.
; If a previous instance is already running it will bring that instance to top instead
; of starting a new instance.
; Note: This will include the System NSIS plugin in the installer.
!define INTERCEPT_MULTIPLE_INSTANCES 1

Name "${PLUGIN_NAME}"
SetCompressor lzma
OutFile "gen_nxsbigclock_setup.exe"
XPStyle on
InstallDir "$PROGRAMFILES\Winamp"
InstallDirRegKey HKLM \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\winamp" "UninstallString"
DirText "Please specify the path to your Winamp 5 installation.$\r$\n\
  You will be able to continue when $\"winamp.exe$\" is found."

!insertmacro MUI_PAGE_WELCOME

; ReadMe page
!define MUI_PAGE_HEADER_TEXT "ReadMe"
!define MUI_PAGE_HEADER_SUBTEXT "Information and version history"
!define MUI_LICENSEPAGE_BUTTON $(^NextBtn)
!define MUI_LICENSEPAGE_TEXT_TOP $(^)
!define MUI_LICENSEPAGE_TEXT_BOTTOM "Please read this document before installing. \
    Contains useful information..."
!insertmacro MUI_PAGE_LICENSE "readme.txt"


!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\winamp.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Run Winamp"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\plugins\gen_nxsbigclock.html"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!define MUI_CUSTOMFUNCTION_GUIINIT .onMyGUIInit

!insertmacro MUI_LANGUAGE "English"

InstType "Plugin only"
InstType "Plugin with source"
InstType "Everything :-)"

Section "${PLUGIN_NAME}" SecPlugin
  SectionIn 1 2 3 RO
  SetOutPath "$INSTDIR\Plugins"
  File "Release\gen_nxsbigclock.dll"
  File "gen_nxsbigclock.html"
  
  ; Fix the configuration data (format changed since last version)
  ; If config_showvis was set to 1, set config_vismode to 2 (the new config
  ; item) and delete the now obsolete configuration item.
  ReadIniStr $R0 "$INSTDIR\Plugins\plugin.ini" "NxS BigClock" "config_showvis"
  StrCmp $R0 1 +1 noshowvis
    DeleteIniStr "$INSTDIR\Plugins\plugin.ini" "NxS BigClock" "config_showvis"
    WriteIniStr "$INSTDIR\Plugins\plugin.ini" "NxS BigClock" "config_vismode" "2"
  noshowvis:
SectionEnd

Section "Source code files" SecSource
  SectionIn 2 3

  SetOutPath "$INSTDIR\Plugins\${PLUGIN_NAME} Source"
    File "readme.txt"

   ; Source file
    File "gen_nxsbigclock.c"
    File "ctrlskin.c"
    File "ctrlskin.h"
    File "iconlist.c"
    File "iconlist.h"
    File "NxSThingerAPI.h"

   ; Resource related
    File "dialog.rc"
    File "resource.h"

   ; "NxSWebLink" control class
    File "nxsweblink.c"
    File "nxsweblink.h"

  ; Visual C++ .NET 2003 project files
    File "gen_nxsbigclock.vcproj"
    File "gen_nxsbigclock.sln"

  ; Winamp SDK files
    File "gen.h"
    File "wa_hotkeys.h"
    File "wa_ipc.h"
    File "wa_msgids.h"
    File "wa_dlg.h"

  ; Include this file as well
    File "${__FILE__}"

SectionEnd

Section -post
  WriteUninstaller "$INSTDIR\gen_nxsbigclock_uninstall.exe"
SectionEnd

LangString SecPlugin ${LANG_ENGLISH} "Required files"
LangString SecSource ${LANG_ENGLISH} "MS Visual C++ .NET 2003 project files"

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecPlugin} "$(SecPlugin)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSource} "$(SecSource)"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "Uninstall"

  ; Remove uninstaller itself
  Delete "$INSTDIR\gen_nxsbigclock_uninstall.exe"

  ; Remove plugin and readme
  Delete "$INSTDIR\Plugins\gen_nxsbigclock.dll"
  Delete "$INSTDIR\Plugins\gen_nxsbigclock.html"

  RMDir /r "$INSTDIR\Plugins\${PLUGIN_NAME} Source"

SectionEnd

!define WINAMP_FILE_EXIT 40001
Function .onInit

!ifdef INTERCEPT_MULTIPLE_INSTANCES
  System::Call 'kernel32::CreateMutexA(i 0, i 0, t "NxSBigClockSetup") i .r1 ?e'
  Pop $R0

  StrCmp $R0 0 noprevinst
    ReadRegStr $R0 HKCU "${PLUGIN_INSTREGKEY}" "WindowHandle"
    System::Call 'user32::SetForegroundWindow(i $R0) i ?e'
    Abort

  noprevinst:
!endif

  checkagain:
  FindWindow $R0 "Winamp v1.x"
  IntCmp $R0 0 ok
    MessageBox MB_YESNO|MB_ICONEXCLAMATION "Please shutdown all instances of Winamp before installing this plugin!$\r$\n\
    Do you want the installer to do this?" IDYES yes IDNO no
    yes:
      SendMessage $R0 ${WM_COMMAND} ${WINAMP_FILE_EXIT} 0
      Goto checkagain
    no:
      Abort ; quit installer
  ok:
FunctionEnd

Function .onVerifyInstDir
  IfFileExists "$INSTDIR\winamp.exe" ok
    Abort
  ok:
FunctionEnd

Function .onMyGUIInit
!ifdef INTERCEPT_MULTIPLE_INSTANCES
  WriteRegStr HKCU "${PLUGIN_INSTREGKEY}" "WindowHandle" "$HWNDPARENT"
!endif
FunctionEnd

Function .onGUIEnd
!ifdef INTERCEPT_MULTIPLE_INSTANCES
  DeleteRegValue HKCU "${PLUGIN_INSTREGKEY}" "WindowHandle"
  DeleteRegKey /ifempty HKCU "${PLUGIN_INSTREGKEY}"
!endif
FunctionEnd

