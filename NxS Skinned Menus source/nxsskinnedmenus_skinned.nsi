; nxsskinnedmenus
; NSIS Installer script
; Written by Saivert

!define INCLUDESKIN_PATH "D:\appz\nsis\my nsis shit\wansis\contrib\wansis\zman3_forum_theme"
!include "d:\appz\nsis\my nsis shit\wansis\include\wansis.nsh"
!addplugindir "d:\appz\nsis\my nsis shit\wansis\plugins"
!define SHIT_PATH "D:\appz\nsis\my nsis shit\wansis\contrib\wansis\zman3_forum_theme"
!define SHIT_SKIN "forum"
!define MUI_UI "${SHIT_PATH}\..\flatui.exe"
!define MUI_ICON "${SHIT_PATH}\marooncd.ico"
!define MUI_UNICON "${SHIT_PATH}\marooncd.ico"
!define MUI_COMPONENTSPAGE_CHECKBITMAP  "${SHIT_PATH}\checks.bmp"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${SHIT_PATH}\header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${SHIT_PATH}\welcomefinish.bmp"
!ifdef MUI_INSERT_NSISCONF
  !undef MUI_INSERT_NSISCONF
!endif

; Define this to the file to generate
!define OUTPUT_FILE "nxsskinnedmenus.exe"

; Product defines
!define PRODUCT_NAME                "NxS Skinned Menus plug-in"
!define PRODUCT_VERSION             "2.8"
!define PRODUCT_PUBLISHER           "Saivert"
!define PRODUCT_WEB_SITE            "http://saivertweb.no-ip.com"
!define PRODUCT_UNINST_KEY          "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY     "HKLM"

; Winamp Plug-in defines
!define WAPLUGIN_SRCFILE            ".\Release\gen_nxsskinnedmenus.dll"
!define WAPLUGIN_FILENAME           "gen_nxsskinnedmenus.dll"
!define WAPLUGIN_DOCS               "gen_nxsskinnedmenus.txt"
!define WAPLUGIN_SOURCEDIR          "NxS Skinned Menus source"
!define WAPLUGIN_UNINSTALLER        "$INSTDIR\uninst_nxsskinnedmenus.exe"

!define PLUGIN_INSTREGKEY "Software\Saivert\NSIS\nxsskinnedmenus"

; Define the following to make the installer intercept multiple instances of itself.
; If a previous instance is already running it will bring that instance to top instead
; of starting a new instance.
; Note: This will include the System NSIS plugin in the installer.
!define INTERCEPT_MULTIPLE_INSTANCES 1


SetCompressor /SOLID lzma

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
!insertmacro MUI_PAGE_LICENSE "${WAPLUGIN_DOCS}"

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
XPStyle off ; Required by wansis

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

SectionEnd

Section "Source" SecSource
  SectionIn 2
  
  SetOutPath "$INSTDIR\plugins\${WAPLUGIN_SOURCEDIR}"
  SetOverwrite off

  ; Include all source files
  File /NONFATAL "*.sln"
  File /NONFATAL "*.vcproj"
  File /NONFATAL "*.def"
  File /NONFATAL "*.cpp"
  File /NONFATAL "*.c"
  File /NONFATAL "*.h"
  File /NONFATAL "*.rc"
  File /NONFATAL "${WAPLUGIN_DOCS}"

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
  !insertmacro MUI_DESCRIPTION_TEXT ${SecSource} "MS Visual Studio .NET 2003 project files"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

!define WINAMP_FILE_EXIT 40001
Function .onInit

!ifdef INTERCEPT_MULTIPLE_INSTANCES
  System::Call 'kernel32::CreateMutexA(i 0, i 0, t "nxsskinnedmenus") i .r1 ?e'
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
  ${INCLUDESKIN} ${SHIT_SKIN}
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

${SKINIT} ${SHIT_SKIN}
FunctionEnd

Function .onGUIEnd
${UNSKINIT}

!ifdef INTERCEPT_MULTIPLE_INSTANCES
  DeleteRegValue HKCU "${PLUGIN_INSTREGKEY}" "WindowHandle"
  DeleteRegKey /ifempty HKCU "${PLUGIN_INSTREGKEY}"
!endif
FunctionEnd

