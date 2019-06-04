##############################
# Author: Saivert & <HM NIS Edit Script Wizard>.
# Created:29.02.2004 20:27:58
# Description: Defines a installer for my plug-in.
##############################

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "NxS Info Tool Plug-in"
!define PRODUCT_VERSION "1.6"
!define PRODUCT_PUBLISHER "Saivert"
!define PRODUCT_WEB_SITE "http://members.tripod.com/files_saivert/"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; [Saivert] Winamp Plug-in helper defines
!define WAPLUGIN_FILEPATHNAME "d:\programfiler\winamp\plugins\gen_infotool.dll"
!define WAPLUGIN_FILENAME "gen_infotool.dll"
!define WAPLUGIN_README "gen_infotool.html"

SetCompressor lzma

; MUI 1.67 compatible ------
!include "MUI.nsh"

; Required by System::Call
!include "${NSISDIR}\Contrib\System\system.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!define MUI_WELCOMEPAGE_TITLE_3LINES
!insertmacro MUI_PAGE_WELCOME

; [Saivert] Readme page (license)
PageEx license
  PageCallbacks "" ReadmePage_Show
  LicenseData "readme.txt"
  LicenseText "Please read this document before installing. \
    Contains useful information..." "$(^NextBtn)"
  Caption " "
PageExEnd

; Components page
!insertmacro MUI_PAGE_COMPONENTS
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\plugins\${WAPLUGIN_README}"
; Added by Saivert
!define MUI_FINISHPAGE_LINK "${PRODUCT_WEB_SITE}"
!define MUI_FINISHPAGE_LINK_LOCATION "${PRODUCT_WEB_SITE}"
!define MUI_FINISHPAGE_TITLE_3LINES
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"


; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "nxsinfotool.exe"
DirText "Please select your Winamp path below \
  (you will be able to proceed when Winamp is detected):"
InstallDir "$PROGRAMFILES\Winamp\"
; detect winamp path from uninstall string if available
InstallDirRegKey HKLM \
                 "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \
                 "UninstallString"
ShowInstDetails show
ShowUnInstDetails show

Section "Plug-in" SEC01
  SectionIn RO
  SetOutPath "$INSTDIR\plugins"
  SetOverwrite ifnewer
  File "${WAPLUGIN_FILEPATHNAME}"
  File "${WAPLUGIN_README}"
SectionEnd

Section "Source" SEC02
  SetOutPath "$INSTDIR\plugins\NxS Info Tool Source"
  SetOverwrite off

  File ${WAPLUGIN_README}
  File "AggressiveOptimize.h"
  File "gen.h"
  File "gen_infotool.aps"
  File "gen_infotool.cpp"
  File "gen_infotool.dsp"
  File "gen_infotool.dsw"
  File "gen_infotool.opt"
  File "gen_infotool.rc"
  File "ipc_pe.h"
  File "resource.h"
  File "nxstabpages.cpp"
  File "nxstabpages.h"
  File "wa_dlg.h"
  File "wa_ipc.h"

  ; Include this file as well
  File ${__FILE__}
SectionEnd

; Section descriptions
LangString desc1 ${LANG_ENGLISH} "Required files"
LangString desc2 ${LANG_ENGLISH} "MS Visual C++ 6.0 project files"

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} $(desc1)
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} $(desc2)
!insertmacro MUI_FUNCTION_DESCRIPTION_END


Section -Post
  WriteUninstaller "$INSTDIR\uninst_nxsinfotool.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst_nxsinfotool.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\winamp.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoModify" "1"
  WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "NoRepair" "1"
SectionEnd

Section Uninstall
  Delete "$INSTDIR\uninst_nxsinfotool.exe"
  
  Delete "$INSTDIR\plugins\NxS Info Tool Source\AggressiveOptimize.h"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\gen.h"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\gen_infotool.aps"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\gen_infotool.cpp"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\gen_infotool.dsp"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\gen_infotool.dsw"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\gen_infotool.opt"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\gen_infotool.rc"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\ipc_pe.h"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\list.txt"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\resource.h"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\nxstabpages.cpp"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\nxstabpages.h"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\wa_dlg.h"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\wa_ipc.h"

  Delete "$INSTDIR\plugins\NxS Info Tool Source\${WAPLUGIN_README}"
  Delete "$INSTDIR\plugins\NxS Info Tool Source\${__FILE__}"


  Delete "$INSTDIR\plugins\${WAPLUGIN_FILENAME}"
  Delete "$INSTDIR\plugins\${WAPLUGIN_README}"

  RMDir "$INSTDIR\plugins\NxS Info Tool Source"
  RMDir "$INSTDIR\plugins"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  SetAutoClose false
SectionEnd

; -= Functions =-

Function .onVerifyInstDir
  ;Check for Winamp installation
  IfFileExists $INSTDIR\Winamp.exe Good
    Abort
  Good:
FunctionEnd

LangString TEXT_IO_TITLE ${LANG_ENGLISH} "Readme"
LangString TEXT_IO_SUBTITLE ${LANG_ENGLISH} "Useful information."
Function ReadmePage_Show
  !insertmacro MUI_HEADER_TEXT "$(TEXT_IO_TITLE)" "$(TEXT_IO_SUBTITLE)"
FunctionEnd

Function .onInit
  InitPluginsDir
FunctionEnd

