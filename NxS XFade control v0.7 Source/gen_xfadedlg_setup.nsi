; Example installer script for plugin
; Written by Saivert

!include "MUI.nsh"

!define PLUGIN_NAME "NxS XFade control v0.7"
!define PLUGIN_INSTREGKEY "Software\Saivert\NSIS\NxSXFadeControl"

; Define the following to make the installer intercept multiple instances of itself.
; If a previous instance is already running it will bring that instance to top instead
; of starting a new instance.
; Note: This will include the System NSIS plugin in the installer.
!define INTERCEPT_MULTIPLE_INSTANCES 1


Name "${PLUGIN_NAME}"
SetCompressor /solid lzma
OutFile "gen_xfadedlg_setup.exe"
XPStyle on
InstallDir "$PROGRAMFILES\Winamp"
InstallDirRegKey HKLM \
  "Software\Microsoft\Windows\CurrentVersion\Uninstall\winamp" "UninstallString"
DirText "Please specify the path to your Winamp 5 installation.$\r$\n\
  You will be able to continue when $\"winamp.exe$\" is found."

!insertmacro MUI_PAGE_WELCOME

; ReadMe page
!define MUI_PAGE_HEADER_TEXT "What's new?"
!define MUI_PAGE_HEADER_SUBTEXT "The version history"
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
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\Plugins\gen_xfadedlg.html"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!define MUI_CUSTOMFUNCTION_GUIINIT .onMyGUIInit

!insertmacro MUI_LANGUAGE "English"

InstType "Plugin only"
InstType "Plugin with source"

Section "${PLUGIN_NAME}" SecPlugin
  SectionIn 1 2 RO
  SetOutPath "$INSTDIR\Plugins"
  File "Release\gen_xfadedlg.dll"
  File "gen_xfadedlg.html"
SectionEnd

Section "Source code files" SecSource
  SectionIn 2

  SetOutPath "$INSTDIR\Plugins\${PLUGIN_NAME} Source"
    File "readme.txt"

   ; Source file
    File "xfadedlg.c"
    File "ctrlskin.c"
    File "ctrlskin.h"

   ; Resource related
    File "dialog.rc"
    File "resource.h"
    File "xfade.bmp"
    File "xfade_h.bmp"

   ; "NxSWebLink" control class
    File "nxsweblink.c"
    File "nxsweblink.h"

  ; Standard project files
;    File "gen_xfadedlg.dsp"
;    File "gen_xfadedlg.dsw"

  ; Visual C++ 2005 Express Edition project files
    File "gen_xfadedlg.sln"
    File "gen_xfadedlg.vcproj"

  ; Winamp SDK files
    File "gen.h"
;    File "wa_hotkeys.h"
    File "wa_ipc.h"
    File "wa_msgids.h"
    File "wa_dlg.h"
    
  ; Other API files
    File "NxSThingerAPI.h"

  ; Include this file as well
    File "${__FILE__}"

SectionEnd

Section -post
  WriteUninstaller "$INSTDIR\gen_xfadedlg_uninstall.exe"
  
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NxSXFadeControlForWinamp" "UninstallString" "$INSTDIR\gen_xfadedlg_uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NxSXFadeControlForWinamp" "DisplayName" "${PLUGIN_NAME}"

SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecPlugin} "Required files"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSource} "MS Visual C++ .NET 2003 project files"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Section "Uninstall"

  ; Remove uninstaller itself
  Delete "$INSTDIR\gen_xfadedlg_uninstall.exe"
  Delete "$INSTDIR\gen_xfadedlg.html"

  Delete "$INSTDIR\Plugins\gen_xfadedlg.dll"
  RMDir /r "$INSTDIR\Plugins\${PLUGIN_NAME} Source"
  
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\NxSXFadeControlForWinamp"

SectionEnd

!define WINAMP_FILE_EXIT 40001
Function .onInit

!ifdef INTERCEPT_MULTIPLE_INSTANCES
  System::Call 'kernel32::CreateMutexA(i 0, i 0, t "NxSXFadeControlSetup") i .r1 ?e'
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
    MessageBox MB_YESNO|MB_ICONEXCLAMATION "Please shutdown all instances of Winamp before installing this plugin!$\r$\nDo you want the installer to do this?" IDYES yes IDNO no
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

