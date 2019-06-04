// Contains function for "skinning" controls
// Version 0.2
// Written by Saivert
// Homepage: http://inthegray.com/saivert/
// E-Mail: saivert@email.com  or  saivert@gmail.com
// You can easily turn this into a native C++ class if you want to,
// since it basically has the structure of a class.

#ifndef _CTRLSKIN_H
#define _CTRLSKIN_H 1

#include <windows.h>
#include <commctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "wa_dlg.h"


/* You must call this at least once, to set the handle to Winamp's main window.
   This window handle is required by the skinning functions.
   Returns the previous window handle set. */
int CtrlSkin_Init(HWND hwndWinamp);

/* Call with fActive set to TRUE to skin, set fActive to FALSE to unskin */
BOOL CtrlSkin_SkinControls(HWND hDlg, BOOL fActive);

/* Call with fActive = True to put the dialog in a skinned frame.
   This function automatically sets the window style of the dialog
   to WS_CHILD|WS_VISIBLE and the extended window style to WS_EX_CONTROLPARENT
   Note: You must only use this function with a modeless dialog (i.e: Dialogs
   created using CreateDialog function). */
embedWindowState* CtrlSkin_EmbedWindow(HWND hDlg, BOOL fActive, UINT flags);


/* Returns TRUE if the window is embedded by CtrlSkin.
   WARNING: Does return TRUE if the window is embedded by any other means. */
BOOL CtrlSkin_IsEmbedded(HWND hWnd);

/* Same as Win32's SetForegroundWindow except it uses the framewindow
   if the window is embeddded. */
BOOL CtrlSkin_SetForegroundWindow(HWND hWnd);	


#ifdef __cplusplus
}
#endif

#endif //_CTRLSKIN_H
