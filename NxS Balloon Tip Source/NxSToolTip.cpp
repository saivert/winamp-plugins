/*
** Written by Saivert
**
** Read license in .H file.
*/

//Uncomment next line if building under MS Visual C++
#include "StdAfx.h"

#include <windows.h>
#include "NxSToolTip.h"

// - ctor & dtor -
CNxSToolTip::CNxSToolTip(HWND hwndOwner)
{
	CNxSToolTip(hwndOwner, GetModuleHandle(NULL));
}

CNxSToolTip::CNxSToolTip(HWND hwndOwner, HINSTANCE hinst)
{
	m_hwndOwner = hwndOwner;
	m_hinst = hinst;
	m_hwnd = CreateWindow(TOOLTIPS_CLASS, (LPSTR) NULL, TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
		NULL, (HMENU) NULL, hinst, NULL); 
}

CNxSToolTip::~CNxSToolTip()
{
	DestroyWindow(m_hwnd);
}

//- methods -

BOOL CNxSToolTip::_AddTool(UINT uFlags, UINT uId, LPRECT prect, LPTSTR lpszText, HINSTANCE hinst)
{
	TOOLINFO ti;
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = uFlags;
	ti.hwnd = m_hwndOwner;
	ti.uId = uId;
	if (prect) CopyRect(&ti.rect, prect);
	ti.hinst = hinst;
	ti.lpszText = lpszText;
	return (BOOL)SendMessage(m_hwnd, TTM_ADDTOOL, 0, (LPARAM)&ti);
}

BOOL CNxSToolTip::AddTool(UINT uFlags, UINT uId, LPRECT prect, LPTSTR lpszText)
{
	return _AddTool(uFlags, uId, prect, lpszText, NULL);
}

BOOL CNxSToolTip::AddTool(UINT uFlags, UINT uId, LPRECT prect, UINT strId)
{
	return _AddTool(uFlags, uId, prect, MAKEINTRESOURCE(strId), m_hinst);
}

BOOL CNxSToolTip::_SetToolInfo(UINT uFlags, UINT uId, LPRECT prect, LPTSTR lpszText, HINSTANCE hinst)
{
	TOOLINFO ti;
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = uFlags;
	ti.hwnd = m_hwndOwner;
	ti.uId = uId;
	if (prect) CopyRect(&ti.rect, prect);
	ti.hinst = hinst;
	return (BOOL)SendMessage(m_hwnd, TTM_SETTOOLINFO, 0, (LPARAM)&ti);
}

BOOL CNxSToolTip::SetToolInfo(UINT uFlags, UINT uId, LPRECT prect, LPTSTR lpszText)
{
	return _SetToolInfo(uFlags, uId, prect, lpszText, NULL);
}

BOOL CNxSToolTip::SetToolInfo(UINT uFlags, UINT uId, LPRECT prect, UINT strId)
{
	return _SetToolInfo(uFlags, uId, prect, MAKEINTRESOURCE(strId), m_hinst);
}


void CNxSToolTip::DelTool(UINT uId)
{
	TOOLINFO ti;
	ti.cbSize = sizeof(TOOLINFO);
	ti.hwnd = m_hwndOwner;
	ti.uId = uId;
	SendMessage(m_hwnd, TTM_DELTOOL, 0, (LPARAM)&ti);
}

char* CNxSToolTip::GetText(UINT uId)
{
	TOOLINFO ti;
	ti.cbSize = sizeof(TOOLINFO);
	ti.hwnd = m_hwndOwner;
	ti.uId = uId;
	SendMessage(m_hwnd, TTM_GETTEXT, 0, (LPARAM)&ti);
	return ti.lpszText;
}

BOOL CNxSToolTip::GetInfo(UINT uId, LPRECT prect, char **lpszText)
{
	TOOLINFO ti;
	LRESULT res;
	ti.cbSize = sizeof(TOOLINFO);
	ti.hwnd = m_hwndOwner;
	ti.uId = uId;
	res = SendMessage(m_hwnd, TTM_GETTOOLINFO, 0, (LPARAM)&ti);
	if (res)
	{
		if (lpszText) *lpszText = ti.lpszText;
		if (prect) CopyRect(prect, &ti.rect);
	}
	return (BOOL)res;
}

BOOL CNxSToolTip::HitTest(POINT point, LPRECT prect, char **lpszText)
{
	TTHITTESTINFO tthti;
	LRESULT res;
	tthti.hwnd = m_hwndOwner;
	tthti.pt = point;
	tthti.ti.cbSize = sizeof(TOOLINFO);
	res = SendMessage(m_hwnd, TTM_HITTEST, 0, (LPARAM)&tthti);
	if (res)
	{
		if (lpszText) *lpszText = tthti.ti.lpszText;
		if (prect) CopyRect(prect, &tthti.ti.rect);
	}
	return (BOOL)res;
}

void CNxSToolTip::SetDelayTime(UINT uAutoPop, UINT uInitial, UINT uReshow)
{
	SendMessage(m_hwnd, TTM_SETDELAYTIME, TTDT_AUTOPOP, uAutoPop);
	SendMessage(m_hwnd, TTM_SETDELAYTIME, TTDT_INITIAL, uInitial);
	SendMessage(m_hwnd, TTM_SETDELAYTIME, TTDT_RESHOW, uReshow);
}
void CNxSToolTip::SetDelayTime(UINT uAutomatic)
{
	SendMessage(m_hwnd, TTM_SETDELAYTIME, TTDT_AUTOMATIC, uAutomatic);
}

void CNxSToolTip::_UpdateTipText(UINT uId, char* lpszText, HINSTANCE hinst)
{
	TOOLINFO ti;
	ti.cbSize = sizeof(TOOLINFO);
	ti.hwnd = m_hwndOwner;
	ti.uId = uId;
	ti.hinst = hinst;
	ti.lpszText = lpszText;

	SendMessage(m_hwnd, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
}

void CNxSToolTip::UpdateTipText(UINT uId, char* lpszText)
{
	_UpdateTipText(uId, lpszText, NULL);
}

void CNxSToolTip::UpdateTipText(UINT uId, UINT strId)
{
	_UpdateTipText(uId, MAKEINTRESOURCE(strId), m_hinst);
}

void CNxSToolTip::RelayEvent(LPMSG pmsg)
{
	SendMessage(m_hwnd, TTM_RELAYEVENT, 0, (LPARAM)pmsg);
}

BOOL WINAPI CNxSToolTip::AddToolInfo_EnumChild(HWND hwndCtrl, LPARAM lParam)
{ 
	DWORD dwFlags;
	char szBuf[NXSTOOLTIP_MAXLOADSTRING];

	if (((CNxSToolTip*)lParam)->m_fSubClass)
		dwFlags = TTF_IDISHWND|TTF_SUBCLASS;
	else
		dwFlags = TTF_IDISHWND;

	//Only handles 256 byte long strings:
	//((CNxSToolTip*)lParam)->AddTool(dwFlags, (UINT)hwndCtrl, NULL, GetDlgCtrlID(hwndCtrl));

	//Here we load the string ourself to handle NXSTOOLTIP_MAXLOADSTRING bytes in strings.
	if ( LoadString(((CNxSToolTip*)lParam)->m_hinst, GetDlgCtrlID(hwndCtrl), szBuf, sizeof(szBuf)) )
		((CNxSToolTip*)lParam)->AddTool(dwFlags, (UINT)hwndCtrl, NULL, szBuf);
	return TRUE;
}

void CNxSToolTip::AddDialogControls(bool fSubClass)
{
	m_fSubClass = fSubClass;
	EnumChildWindows(m_hwndOwner, AddToolInfo_EnumChild, (LPARAM)this);
}

HWND g_hwndDlg;
HWND g_hwndTT;
HHOOK g_hhk;

void CNxSToolTip::InstallHook(void)
{
	g_hwndDlg = m_hwndOwner;
	g_hwndTT = m_hwnd;
	g_hhk = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc,
		(HINSTANCE) NULL, GetCurrentThreadId());
}

void CNxSToolTip::UninstallHook(void)
{
	UnhookWindowsHookEx(g_hhk);
}


//This static function is used by InstallHook
LRESULT CALLBACK CNxSToolTip::GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSG *lpmsg;
	
	lpmsg = (MSG *) lParam;
	if (nCode < 0 || !(IsChild(g_hwndDlg, lpmsg->hwnd)))
		return (CallNextHookEx(g_hhk, nCode, wParam, lParam));
	
	switch (lpmsg->message)
	{
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		if (g_hwndTT != NULL)
			SendMessage(g_hwndTT, TTM_RELAYEVENT, 0,
			(LPARAM) (LPMSG) lpmsg);
		break;
	default:
		break;
	}
	return (CallNextHookEx(g_hhk, nCode, wParam, lParam));
}

//- properties -

LONG CNxSToolTip::SetStyle(LONG style)
{
	return SetWindowLong(m_hwnd, GWL_STYLE, style);
}

LONG CNxSToolTip::GetStyle(void)
{
	return GetWindowLong(m_hwnd, GWL_STYLE);
}

HWND CNxSToolTip::SetOwnerWnd(HWND hwndNewOwner)
{
	HWND hwndTemp;

	hwndTemp = m_hwndOwner;
	if (IsWindow(hwndNewOwner))
		m_hwndOwner = hwndNewOwner;
	return hwndTemp;
}
