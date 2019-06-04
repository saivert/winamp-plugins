#if !defined(__NXSWEBLINK_H)
#define __NXSWEBLINK_H

#if _MSC_VER > 1000
#pragma once
#endif

#include <windows.h>

#define WC_NXSWEBLINK "NxSWebLink_879234"
//The following style is set by default
#define WLS_OPAQUE 65535+1

// prototypes
LRESULT CALLBACK WebLinkProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
ATOM RegisterNxSWebLink(HINSTANCE hinst);
HWND CreateNxSWebLink(HINSTANCE hinst, char *text, int x, int y, int cx, int cy,
	BOOL visible, HWND parentwnd, int id);
int ExecuteURL(char *url);


#endif //__NXSWEBLINK_H
