/*
** Written by Saivert
**
** License:
** This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held 
** liable for any damages arising from the use of this software. 
**
** Permission is granted to anyone to use this software for any purpose, including commercial applications, and to 
** alter it and redistribute it freely, subject to the following restrictions:
**
**   1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. 
**      If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
**
**   2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
**
**   3. This notice may not be removed or altered from any source distribution.
**
*/

#define _WIN32_IE 0x600
#include <windows.h>
#include "shellapi.h"

#include "NxSTrayIcon.h"

typedef struct _DLLVERSIONINFO
{
    DWORD cbSize;
    DWORD dwMajorVersion;                   // Major version
    DWORD dwMinorVersion;                   // Minor version
    DWORD dwBuildNumber;                    // Build number
    DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO, *LPDLLVERSIONINFO;

typedef HRESULT (CALLBACK *_tDllGetVersionProc)(DLLVERSIONINFO *);

static _tDllGetVersionProc DllGetVersion=0;

// This ID value is increased with 1 each time a new CNxSTrayIcon is created.
// It will not decrease when a CNxSTrayIcon is destroyed so you must not use
// it as a reference count.
static int g_ID=1;

CNxSTrayIcon::CNxSTrayIcon(void)
{
    DLLVERSIONINFO dvi;

	dvi.cbSize = sizeof(DLLVERSIONINFO);
	dvi.dwMajorVersion = 0;

	if (!DllGetVersion)
	{
		HMODULE hlib = GetModuleHandle("shell32.dll");
		DllGetVersion = (_tDllGetVersionProc)GetProcAddress(hlib, "DllGetVersion");
	}
    
	if (DllGetVersion) DllGetVersion(&dvi);

	m_IsNewShell = (dvi.dwMajorVersion >= 5);

	m_created = false;
	m_visible = false;
	m_timeout = 0;

	// 5.0 = Windows 2000
	// 5.1 = Windows XP
	if (dvi.dwMajorVersion >= 5) {
		m_nid.cbSize = (dvi.dwMinorVersion >= 1)?sizeof(NOTIFYICONDATA):NOTIFYICONDATA_V2_SIZE;
	} else {
		m_nid.cbSize = NOTIFYICONDATA_V1_SIZE;
	}
	m_nid.hIcon = 0;
	m_nid.hWnd = 0;
	m_nid.szTip[0] = 0;
	m_nid.uCallbackMessage = 0;
	m_nid.uFlags = NIF_MESSAGE;
	m_nid.uID = g_ID++;

	// Inform shell controls we want the old messaging style
	SetVersion(0);
}

CNxSTrayIcon::~CNxSTrayIcon(void)
{
	Remove();
}

BOOL CNxSTrayIcon::SetVersion(int version)
{
	m_nid.uVersion = version;
	return Shell_NotifyIcon(NIM_SETVERSION, &m_nid);
}

BOOL CNxSTrayIcon::Show(void)
{
	m_visible = true;

	if (!m_nid.hWnd && !m_nid.uID) return FALSE;

	if (m_IsNewShell && m_created)
	{
		m_nid.uFlags = NIF_STATE;
		m_nid.dwStateMask = NIS_HIDDEN;
		m_nid.dwState = 0;
		return Shell_NotifyIcon(NIM_MODIFY, &m_nid);
	} else {
		m_created = true;

		m_nid.uFlags = NIF_MESSAGE;
		if (m_nid.szTip[0]) m_nid.uFlags |= NIF_TIP;
		if (m_nid.hIcon) m_nid.uFlags |= NIF_ICON;

		return Shell_NotifyIcon(NIM_ADD, &m_nid);
	}
}

BOOL CNxSTrayIcon::Hide(void)
{
	m_visible = false;

	if (!m_nid.hWnd && !m_nid.uID) return FALSE;

	if (m_IsNewShell)
	{
		m_nid.uFlags = NIF_STATE;
		m_nid.dwState = m_nid.dwStateMask = NIS_HIDDEN;
		return Shell_NotifyIcon(NIM_MODIFY, &m_nid);
	} else {
		m_created = false;
		//Old style hiding by removing it
		return Shell_NotifyIcon(NIM_DELETE, &m_nid);
	}
}

BOOL CNxSTrayIcon::Remove(void)
{
	m_visible = false;
	m_created = false;

	if (!m_nid.hWnd && !m_nid.uID) return FALSE;

	return Shell_NotifyIcon(NIM_DELETE, &m_nid);
}

void CNxSTrayIcon::Restore(void)
{
	if (IsVisible())
	{
		m_created = m_visible = false;
		Show();
	}
}

void CNxSTrayIcon::SetTooltip(const char *newtip)
{
	lstrcpyn(m_nid.szTip, newtip, sizeof(m_nid.szTip)-1);

	//Update taskbar if created
	if (m_created)
	{
		m_nid.uFlags = NIF_TIP;
		Shell_NotifyIcon(NIM_MODIFY, &m_nid);
	}
}

HICON CNxSTrayIcon::SetIcon(HICON newicon)
{
	HICON tmp;
	tmp = m_nid.hIcon;
	m_nid.hIcon = newicon;

	//Update taskbar
	m_nid.uFlags = NIF_ICON;
	Shell_NotifyIcon(NIM_MODIFY, &m_nid);
	return tmp;
}

DWORD CNxSTrayIcon::SetTimeout(DWORD newtimeout)
{
	DWORD tmp;
	tmp = m_timeout;
	m_timeout = newtimeout;
	return tmp;
}

BOOL CNxSTrayIcon::ShowBalloon(const char *szInfo, const char *szInfoTitle, DWORD dwFlags)
{
	if (!m_created || !m_visible) Show();

	m_nid.uFlags = NIF_INFO;
	m_nid.uTimeout = m_timeout;
	m_nid.dwInfoFlags = dwFlags;

	lstrcpyn(m_nid.szInfo, szInfo, sizeof(m_nid.szInfo)-1);
	lstrcpyn(m_nid.szInfoTitle, szInfoTitle, sizeof(m_nid.szInfoTitle)-1);

	return Shell_NotifyIcon(NIM_MODIFY, &m_nid);
}

void CNxSTrayIcon::HideBalloon(void)
{
	m_nid.uFlags = NIF_INFO;
	m_nid.szInfo[0] = '\0';
	m_nid.szInfoTitle[0] = '\0';
	Shell_NotifyIcon(NIM_MODIFY, &m_nid);
}

BOOL CNxSTrayIcon::SetFocus()
{
	return Shell_NotifyIcon(NIM_SETFOCUS, &m_nid);
}

BOOL CNxSTrayIcon::SetGUID(const GUID Guid)
{
	m_nid.uFlags = NIF_GUID;
	m_nid.guidItem = Guid;
	return Shell_NotifyIcon(NIM_MODIFY, &m_nid);
}

bool CNxSTrayIcon::IsTaskbarAvailable(void)
{
	HWND hwnd = FindWindow("Shell_TrayWnd", NULL);
	return hwnd > 0;
}


UINT CNxSTrayIcon::SetCallbackMsg(UINT uMsg)
{
	UINT old=m_nid.uCallbackMessage;
	if (uMsg > 0)
		m_nid.uCallbackMessage = uMsg;
	return old;
}

HWND CNxSTrayIcon::SetWnd(HWND hWnd)
{
	HWND old=m_nid.hWnd;
	if (hWnd > 0)
		m_nid.hWnd = hWnd;
	return old;
}

void CNxSTrayIcon::Attach(HWND wnd, UINT id)
{
	SetWnd(wnd);
	SetID(id);
	m_created = true;
}

void CNxSTrayIcon::Detach()
{
	// Here we must directly access the m_nid members since bothe SetWnd and SetID has protection
	// agains null parameters (being invalid).
	m_nid.hWnd = 0;
	m_nid.uID = 0;
	m_created = false;
	m_visible = false;
}
