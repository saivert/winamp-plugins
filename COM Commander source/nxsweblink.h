/* nxsweblink.h: A clean cut Web Link style button include file.
 *
 *  Modified: All C++ now baby!
 *
 * NOTE: This is the static-link version of NxSWebLink. If you like
 *       to use a custom control library instead try the NxSWebLink.dll
 *       package. Available at my homepage (URL below).
 *
 * To register the class "NxSWebLink", call RegisterNxSWebLink(hInstance)
 * in DllMain, WinMain or some other initialization function.
 *
 * There's also a nice function called CreateNxSWebLink which is kinda
 * a CreateWindow, only for the "NxSWebLink" class. Usually you don't
 * need this function cos' you can insert a custom control statement in
 * the dialog template, like this:
 *
 * CONTROL "http://www.mysite.com/page.html", IDC_YOURID, "NxSWebLink",
 *   WS_TABSTOP, 10, 15, 150, 8
 *
 *
 * Saivert's homepage
 *   http://members.tripod.com/files_saivert/
 *
 * You can also send an e-mail
 *   saivert AT email DOT com
 */

#if !defined(__NXSWEBLINK_H)
#define __NXSWEBLINK_H

#if _MSC_VER > 1000
#pragma once
#endif

#define WC_NXSWEBLINK "NxSWebLink"
#define NXS_IDC_HAND MAKEINTRESOURCE(32649)
#define GWL_WEBLINKINFOPTR 0

typedef struct _WEBLINKINFO
{
	RECT txtRect;
	BOOL allowClick;
} WEBLINKINFO, *PWEBLINKINFO;

// prototypes
LRESULT CALLBACK WebLinkProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
ATOM RegisterNxSWebLink(HINSTANCE hinst);
HWND CreateNxSWebLink(HINSTANCE hinst, char *text, int x, int y, int cx, int cy,
	BOOL visible, HWND parentwnd, int id);


#endif
