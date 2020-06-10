/* nxsweblink.c: A clean cut Web Link style button source file.
 * 
 * NOTE: This is the static-link version of NxSWebLink. If you like
 *       to use a custom control library instead try the NxSWebLink.dll
 *       package. Available at my homepage (URL below).
 *
 * This control is a super-class of the standard "BUTTON" class defined
 * by Windows. It behaves just like a owner-draw button (a button with
 * the style BS_OWNERDRAW). Just that you don't have to do any drawing
 * yourself.
 *
 * Saivert's homepage
 *   http://members.tripod.com/files_saivert/
 *
 * You can also send an e-mail
 *   saivert AT email DOT com
 */

#include "stdafx.h"
#include "nxsweblink.h"

WNDPROC oldButtonWndProc;
int oldButtonWndExtra;

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
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
		if (!pwbi->allowClick) return 0;
		return CallWindowProc(oldButtonWndProc,hWnd, uMsg, wParam, lParam);
	case WM_CREATE:
	{
		LRESULT lr;
		lr = CallWindowProc(oldButtonWndProc,hWnd, uMsg, wParam, lParam);

		// Ensure BS_OWNERDRAW style flag is set, so we don't get weird paintjobs done.
		SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) | BS_OWNERDRAW);

		pwbi = (PWEBLINKINFO) LocalAlloc(LPTR, sizeof(WEBLINKINFO));
		if (!pwbi) MessageBox(hWnd, "Error creating WebLink control!", NULL, 0);
		pwbi->allowClick = FALSE;
		SetWindowLong(hWnd, oldButtonWndExtra + GWL_WEBLINKINFOPTR, (long) pwbi);

		return lr;
	}
	case WM_DESTROY:
	{
		LocalFree((HLOCAL)pwbi);
		return CallWindowProc(oldButtonWndProc,hWnd, uMsg, wParam, lParam);
	}
	case WM_SETCURSOR:
	{
		POINT pt;
		// only show "hand" cursor when inside text rectangle
		GetCursorPos(&pt);
		ScreenToClient(hWnd, &pt);
		pwbi->allowClick = PtInRect(&pwbi->txtRect, pt);
		SetCursor(LoadCursor(0, pwbi->allowClick?NXS_IDC_HAND:IDC_ARROW));
		break;
	}
	case WM_SETFOCUS:
		InvalidateRect(hWnd, NULL, TRUE);
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

		dc=BeginPaint(hWnd, &ps);

		GetWindowText(hWnd, ctxt, sizeof(ctxt));
		GetClientRect(hWnd, &tr);
		FillRect(dc, &tr, GetSysColorBrush(COLOR_BTNFACE));
		SetTextColor(dc, 0x00FF0000);

		// Fixed: Now uses parent font handle (dialog)
		hf = (HFONT)SendMessage(GetParent(hWnd), WM_GETFONT, 0, 0);
		GetObject(hf, sizeof(LOGFONT), &font);
		font.lfUnderline = true; //Just change to underlined
		hf = CreateFontIndirect(&font);
		oldfont = (HFONT)SelectObject(dc, hf);

		// calculate actual rectangle where text is written
		// used to paint focus rectangle
		CopyRect(&fr, &tr);
		DrawText(dc, ctxt, strlen(ctxt), &fr, DT_CALCRECT | DT_SINGLELINE);
		// center RECT fr inside RECT tr
		fr.left = ( (tr.right-tr.left) - (fr.right-fr.left) ) / 2;
		fr.right += fr.left;
		fr.top = ( (tr.bottom-tr.top) - (fr.bottom-fr.top) ) / 2;
		fr.bottom += fr.top;
		CopyRect(&pwbi->txtRect, &fr); // store rectangle in static var above,
								// used by other message handlers

		// draw text
		oldBkMode = SetBkMode(dc, TRANSPARENT);
		DrawText(dc, ctxt, strlen(ctxt), &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		if (GetFocus() == hWnd)	DrawFocusRect(dc, &fr);
		SelectObject(dc, oldfont);
		SetBkMode(dc, oldBkMode);
		DeleteObject(hf);
		EndPaint(hWnd, &ps);
		return 0;
	}
	default:
		return CallWindowProc(oldButtonWndProc,hWnd, uMsg, wParam, lParam);
	}
	return ((LONG) TRUE);
}
