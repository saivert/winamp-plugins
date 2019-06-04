; NxS Balloon Tip Notification Plugin
; NSIS Installer script
; Written by Saivert

; Define this to the file to generate
!define OUTPUT_FILE "nxsballoontip.exe"

; Product defines
!define PRODUCT_NAME                "NxS Balloon Tip Notification Plugin"
!define PRODUCT_VERSION             "2.8"
!define PRODUCT_PUBLISHER           "Saivert"
!define PRODUCT_WEB_SITE            "http://inthegray.com/saivert/"
!define PRODUCT_UNINST_KEY          "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY     "HKLM"

; Winamp Plug-in defines
!define WAPLUGIN_SRCFILE            ".\Release\gen_nxsballoontip.dll"
!define WAPLUGIN_FILENAME           "gen_nxsballoontip.dll"
!define WAPLUGIN_DOCS               "gen_nxsballoontip.html"
!define WAPLUGIN_SOURCEDIR          "NxS Balloon Tip Source"
!define WAPLUGIN_UNINSTALLER        "$INSTDIR\uninst_nxsballoontip.exe"

!define PLUGIN_INSTREGKEY "Software\Saivert\NSIS\NxSBalloonTipPlugin"

; Define the following to make the installer intercept multiple instances of itself.
; If a previous instance is already running it will bring that instance to top instead
; of starting a new instance.
; Note: This will include the System NSIS plugin in the installer.
!define INTERCEPT_MULTIPLE_INSTANCES 1


SetCompressor lzma

!include "MUI.nsh"

!define MUI_CUSTOMFUNCTION_GUIINIT .onMyGUIInit

!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

!define MUI_WELCOMEPAGE_TITLE_3LINES
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

!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\plugins\${WAPLUGIN_DOCS}"
!define MUI_FINISHPAGE_LINK "${PRODUCT_WEB_SITE}"
!define MUI_FINISHPAGE_LINK_LOCATION "${PRODUCT_WEB_SITE}"
!define MUI_FINISHPAGE_RUN "$INSTDIR\winamp.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Start Winamp"
!define MUI_FINISHPAGE_TITLE_3LINES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile ${OUTPUT_FILE}
DirText "Please select your Winamp path below \
  (you will be able to proceed when Winamp is detected):"
InstallDir "$PROGRAMFILES\Winamp\"
; detect winamp path from uninstall string if available
InstallDirRegKey HKLM \
                 "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \
                 "UninstallString"
ShowInstDetails show
ShowUnInstDetails show

InstType "Plugin only"
InstType "Plugin with source"

Section "Plug-in" SecPlugin
  SectionIn 1 2 RO

  SetOutPath "$INSTDIR\plugins"
  SetOverwrite ifnewer
  File "${WAPLUGIN_SRCFILE}"
  File "${WAPLUGIN_DOCS}"

  ; Delete old version (new version has different filename)
  IfFileExists "$INSTDIR\plugins\gen_nxsballoon.dll" +1 noprevfile
    DetailPrint "Removing old version (new version has a different filename)"
    Delete "$INSTDIR\plugins\gen_nxsballoon.dll"
  noprevfile:

  ; Transfer old settings to new INI keys

  ; Enable plug-in when it's installed
  DeleteINIStr "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "enabled"
  WriteINIStr "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "config_enabled" "1"

  ReadINIStr $R0 "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "usewinampstrayicon"
  StrCmp $R0 "" +2 ; Do not overwrite settings if key does not exists (or is blank).
    WriteINIStr "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "config_usewatray" $R0
  DeleteINIStr "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "usewinampstrayicon"

  ReadINIStr $R0 "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "balloontimeout"
  StrCmp $R0 "" +2
    WriteINIStr "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "config_timeout" $R0
  DeleteINIStr "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "balloontimeout"

  ReadINIStr $R0 "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "iconindex"
  StrCmp $R0 "" +2
    WriteINIStr "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "config_iconidx" $R0
  DeleteINIStr "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "iconindex"

  ReadINIStr $R0 "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "advformatstring"
  StrCmp $R0 "" +2
    WriteINIStr "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "szAdvFormatStr" $R0
  DeleteINIStr "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "advformatstring"

  ReadINIStr $R0 "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "balloontitle"
  StrCmp $R0 "" +2
    WriteINIStr "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "szBalloonTitle" $R0
  DeleteINIStr "$INSTDIR\plugins\plugin.ini" "NxS Balloon Tip" "balloontitle"

SectionEnd

Section "Source" SecSource
  SectionIn 2
  
  SetOutPath "$INSTDIR\plugins\${WAPLUGIN_SOURCEDIR}"
  SetOverwrite off

  ; Include all source files
  File "*.dsw"
  File "*.dsp"
  File "*.cpp"
  File "*.h"
  File "*.rc"
  File "readme.txt"
  
  ; Include new VS .NET 2003 files
  File "gen_nxsballoontip.sln"
  File "gen_nxsballoontip.vcproj"
  
  ; Include this file as well
  File ${__FILE__}
SectionEnd

Section -Post
  WriteUninstaller "${WAPLUGIN_UNINSTALLER}"
  
  ; Add entry to Add/Remove programs list
  WriteRegStr   ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName"     "$(^Name)"
  WriteRegStr   ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "${WAPLUGIN_UNINSTALLER}"
  WriteRegStr   ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion"  "${PRODUCT_VERSION}"
  WriteRegStr   ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon"     "$INSTDIR\winamp.exe"
  WriteRegStr   ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout"    "${PRODUCT_WEB_SITE}"
  WriteRegStr   ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher"       "${PRODUCT_PUBLISHER}"
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify"        "1"
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair"        "1"
SectionEnd

Section Uninstall
  Delete "${WAPLUGIN_UNINSTALLER}"
  RMDir /r "$INSTDIR\plugins\${WAPLUGIN_SOURCEDIR}"
  Delete "$INSTDIR\plugins\${WAPLUGIN_FILENAME}"
  Delete "$INSTDIR\plugins\${WAPLUGIN_DOCS}"
  RMDir "$INSTDIR\plugins\${WAPLUGIN_SOURCEDIR}"
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  SetAutoClose false
SectionEnd

; -= Functions =-

; Section descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecPlugin} "Required files"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSource} "MS Visual C++ 6.0 project files"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

!define WINAMP_FILE_EXIT 40001
Function .onInit

!ifdef INTERCEPT_MULTIPLE_INSTANCES
  System::Call 'kernel32::CreateMutexA(i 0, i 0, t "NxSBalloonTipPluginSetup") i .r1 ?e'
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

