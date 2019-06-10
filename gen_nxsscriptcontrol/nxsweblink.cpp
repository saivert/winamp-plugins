#include "stdafx.h"
#include "nxsweblink.h"
#include "shellapi.h"

WNDPROC oldButtonWndProc;
int oldButtonWndExtra;
HRESULT DrawThemeParentBackground(HWND,HDC,LPRECT);

typedef struct _WEBLINKINFO
{
	RECT txtRect;
	BOOL allowClick;

	HBITMAP hbm, holdbm;
	HDC memDC;
	BOOL hasSavedBkgnd;
	BOOL isThemed;
	BOOL isTransparent;
} WEBLINKINFO, *PWEBLINKINFO;

#define NXS_IDC_HAND MAKEINTRESOURCE(32649)
#define GWL_WEBLINKINFOPTR 0



ATOM RegisterNxSWebLink(HINSTANCE hinst)
{
	WNDCLASS weblinkwc;
	ATOM weblinkatom;

	ZeroMemory(&weblinkwc, sizeof(WNDCLASS));

	GetClassInfo(NULL, "BUTTON", &weblinkwc);
	oldButtonWndProc = weblinkwc.lpfnWndProc;

	weblinkwc.lpszClassName = WC_NXSWEBLINK;
	weblinkwc.lpfnWndProc = WebLinkProc;
	weblinkwc.hInstance = hinst;
	oldButtonWndExtra = weblinkwc.cbWndExtra;
	weblinkwc.cbWndExtra += 4;

	if (!(weblinkatom=RegisterClass (&weblinkwc)) )
		return FALSE;

	return weblinkatom;
}


HWND CreateNxSWebLink(HINSTANCE hinst, char *text, int x, int y, int cx, int cy,
	BOOL visible, HWND parentwnd, int id)
{
	return CreateWindow("NxSWebLink", text,
		BS_OWNERDRAW | WS_CHILD | (visible?WS_VISIBLE:0),
		x, y, cx, cy, parentwnd, (HMENU)id, hinst, NULL);
}

LRESULT CALLBACK WebLinkProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PWEBLINKINFO pwbi=0;
	if (uMsg != WM_CREATE)
	  pwbi = (PWEBLINKINFO)GetWindowLong(hWnd, oldButtonWndExtra + GWL_WEBLINKINFOPTR);


	switch (uMsg)
	{
	case WM_CREATE:
	{
		LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
 		HINSTANCE hlib;
		LRESULT lr;
		HDC dc;
		RECT cr;
		lr = CallWindowProc(oldButtonWndProc,hWnd, uMsg, wParam, lParam);

		// Ensure BS_OWNERDRAW style flag is set, so we don't get weird paintjobs done.
		//SetWindowLong(hWnd, GWL_STYLE, lpcs->style | BS_OWNERDRAW);
		
		pwbi = (PWEBLINKINFO) LocalAlloc(LPTR, sizeof(WEBLINKINFO));
		if (!pwbi) MessageBox(hWnd, "Error creating WebLink control!", NULL, 0);
		pwbi->allowClick = FALSE;
		pwbi->hasSavedBkgnd = FALSE;
		SetWindowLong(hWnd, oldButtonWndExtra + GWL_WEBLINKINFOPTR, (long) pwbi);

		//Currently fixed. Should be dependent on window style.
		pwbi->isTransparent = !(lpcs->style & WLS_OPAQUE);

		GetClientRect(hWnd, &cr);

		if (hlib = LoadLibrary(TEXT("uxtheme.dll")))
		{
			BOOL (WINAPI * _fnIsThemeDialogTextureEnabled)(HWND hwnd);
			_fnIsThemeDialogTextureEnabled = (BOOL (WINAPI*)(HWND hwnd))GetProcAddress(hlib, "IsThemeDialogTextureEnabled");
			pwbi->isThemed = _fnIsThemeDialogTextureEnabled(GetParent(hWnd));
			FreeLibrary(hlib);
		}

		/* Create a memory DC and a bitmap for it. This will be used to store
		   the background of the control (what is painted on the parent).
		   It is only used if the control does not have the WLS_OPAQUE style. */
		if (pwbi->isTransparent && !pwbi->isThemed)
		{
			dc = GetDC(hWnd);
			pwbi->memDC = CreateCompatibleDC(dc);
			pwbi->hbm = CreateCompatibleBitmap(dc, cr.right, cr.bottom);
			ReleaseDC(hWnd, dc);
			pwbi->holdbm = (HBITMAP)SelectObject(pwbi->memDC, pwbi->hbm);
		}


		return lr;
	}
	case WM_DESTROY:
	{
		if (pwbi->isTransparent && !pwbi->isThemed)
		{
			pwbi->hasSavedBkgnd=FALSE;
			SelectObject(pwbi->memDC, pwbi->holdbm);
			DeleteObject(pwbi->hbm);
			DeleteDC(pwbi->memDC);
		}
		LocalFree((HLOCAL)pwbi);
		return CallWindowProc(oldButtonWndProc,hWnd, uMsg, wParam, lParam);
	}
	case WM_NCHITTEST:
	{
		POINT pt = {LOWORD(lParam), HIWORD(lParam)};
		ScreenToClient(hWnd, &pt);
		pwbi->allowClick = PtInRect(&pwbi->txtRect, pt);
		if (pwbi->allowClick)
			return HTCLIENT;
		else
			return HTNOWHERE;
	}
	case WM_SETCURSOR:
	{
		if ((HWND)wParam == hWnd)
		{
			HCURSOR hCur = LoadCursor(NULL,
				(LOWORD(lParam) == HTCLIENT)?NXS_IDC_HAND:IDC_ARROW);
			if (hCur)
			{
				SetCursor(hCur);
				return 1; // halt further processing
			}
		}
	}
	case WM_SETFOCUS:
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	case WM_ERASEBKGND:
		/* We need to make a copy of the background as it is before we do
		   anything else. This makes the control transparent. */
		if ((pwbi->isTransparent && !pwbi->isThemed) && !pwbi->hasSavedBkgnd)
		{
			RECT tr;
			GetClientRect(hWnd, &tr);

			BitBlt(pwbi->memDC, 0, 0, tr.right, tr.bottom, (HDC)wParam, 0, 0, SRCCOPY);
			pwbi->hasSavedBkgnd=TRUE;
		}

		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC dc;
		char ctxt[512];
		int oldBkMode;
		HFONT hf, oldfont;
		LOGFONT font;
		RECT tr, fr;

		GetWindowText(hWnd, ctxt, sizeof(ctxt));
		GetClientRect(hWnd, &tr);

		dc=BeginPaint(hWnd, &ps);

		if (pwbi->isTransparent)
		{
			if (pwbi->isThemed)
				DrawThemeParentBackground(hWnd, dc, &tr);
			else
				BitBlt(dc, 0, 0, tr.right, tr.bottom, pwbi->memDC, 0, 0, SRCCOPY);
		} else
			FillRect(dc, &tr, (HBRUSH)GetSysColorBrush(COLOR_BTNFACE));

		SetTextColor(dc, 0x00FF0000);

		// Fixed: Now uses own font handle (dialog sends WM_SETFONT)
		hf = (HFONT)SendMessage(hWnd, WM_GETFONT, 0, 0);
		GetObject(hf, sizeof(LOGFONT), &font);
		font.lfUnderline = true; //Just change to underlined
		hf = CreateFontIndirect(&font);
		oldfont = (HFONT)SelectObject(dc, hf);

		/* Calculate actual rectangle where text is written
		   used to paint focus rectangle. */
		CopyRect(&fr, &tr);
		DrawText(dc, ctxt, strlen(ctxt), &fr, DT_CALCRECT | DT_SINGLELINE);
		// center RECT fr inside RECT tr
		fr.left = ( (tr.right-tr.left) - (fr.right-fr.left) ) / 2;
		fr.right += fr.left;
		fr.top = ( (tr.bottom-tr.top) - (fr.bottom-fr.top) ) / 2;
		fr.bottom += fr.top;
		CopyRect(&pwbi->txtRect, &fr); /* Store rectangle in static var above,
								          used by other message handlers. */

		// draw text
		LONG style=GetWindowLong(hWnd, GWL_STYLE);
		int textpos=DT_CENTER;
		if (style & WLS_LEFT)
			textpos = DT_LEFT;
		if (style & WLS_RIGHT)
			textpos = DT_RIGHT;

		oldBkMode = SetBkMode(dc, TRANSPARENT);
		DrawText(dc, ctxt, strlen(ctxt), &tr, textpos | DT_VCENTER | DT_SINGLELINE);

		if (GetFocus() == hWnd)	DrawFocusRect(dc, &fr);
		SelectObject(dc, oldfont);
		SetBkMode(dc, oldBkMode);
		DeleteObject(hf);
		EndPaint(hWnd, &ps);
		return 0;
	}
	case WM_GETFONT:
	case WM_SETFONT:
		return CallWindowProc(DefWindowProc,hWnd, uMsg, wParam, lParam);
		break;
	default:
		return CallWindowProc(oldButtonWndProc,hWnd, uMsg, wParam, lParam);
	}
	return ((LONG) TRUE);
}


typedef (WINAPI * DrawThemeParentBackgroundType)(HWND,HDC,LPRECT);

HRESULT DrawThemeParentBackground(HWND hwnd, HDC hdc, LPRECT prc)
{
	DrawThemeParentBackgroundType pfnDTPB;
	HINSTANCE hDll;
	HRESULT res=-1;

	if (NULL != (hDll = LoadLibrary(TEXT("uxtheme.dll"))))
	{
		if (NULL != (pfnDTPB = (DrawThemeParentBackgroundType)GetProcAddress(hDll, "DrawThemeParentBackground")))
		{
			res = pfnDTPB(hwnd, hdc, prc);
		}
		FreeLibrary(hDll);
	}
	return res;
}

/* Creates a URL file and executes it.
   Returns 1 if everything was okey! */
int ExecuteURL(char *url)
{
	HANDLE hf;
	char szTmp[4096];
	DWORD written;
	int hinst;
	char szName[MAX_PATH+1];
	char szTempPath[MAX_PATH+1];
	char *p;

	GetTempPath(MAX_PATH, szTempPath);
	GetTempFileName(szTempPath, "lnk", 0, szName);
	DeleteFile(szName);
	/* We got a temporary filename, change the extension to ".URL" */
	p = szName;
	while (*p) p++; /*go to end*/
	while (p >= szName && *p != '.') p--;
	*p = 0;
	lstrcat(szName, ".URL");


	hf = CreateFile(szName, /*GENERIC_READ|*/GENERIC_WRITE,
		FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	wsprintf(szTmp, "[InternetShortcut]\r\nurl=%s\r\n", url);
	WriteFile(hf, szTmp, lstrlen(szTmp), &written, NULL);
	CloseHandle(hf);
	hinst = (int)ShellExecute(0, "open", szName, NULL, NULL, SW_SHOWNORMAL);
	DeleteFile(szName);
	return (written && hinst>32);
}
