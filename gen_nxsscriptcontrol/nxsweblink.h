
#if !defined(__NXSWEBLINK_H)
#define __NXSWEBLINK_H

#if _MSC_VER > 1000
#pragma once
#endif

#define WC_NXSWEBLINK "NxSWebLink"
//The following style is set by default
#define WLS_OPAQUE    (48001)
#define WLS_LEFT      (48004)
#define WLS_CENTER    (0)
#define WLS_RIGHT     (48008)

// prototypes
LRESULT CALLBACK WebLinkProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
ATOM RegisterNxSWebLink(HINSTANCE hinst);
HWND CreateNxSWebLink(HINSTANCE hinst, char *text, int x, int y, int cx, int cy,
	BOOL visible, HWND parentwnd, int id);
int ExecuteURL(char *url);


#endif
