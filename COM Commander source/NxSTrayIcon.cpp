// NxSTrayIcon.cpp: implementation of the CNxSTrayIcon class.
// This class defines it's own required typedefs in NxSTrayIcon.h
// No need for latest Platform SDK (version 6.0) headers.
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "NxSTrayIcon.h"
//include <strsafe.h>

int g_ID=0;

typedef struct _DLLVERSIONINFO
{
    DWORD cbSize;
    DWORD dwMajorVersion;                   // Major version
    DWORD dwMinorVersion;                   // Minor version
    DWORD dwBuildNumber;                    // Build number
    DWORD dwPlatformID;                     // DLLVER_PLATFORM_*
} DLLVERSIONINFO, *LPDLLVERSIONINFO;

typedef HRESULT (CALLBACK *_tDllGetVersionProc)(DLLVERSIONINFO *pdvi);
typedef BOOL (__stdcall *_tShell_NotifyIconProc)(DWORD dwMessage, PNOTIFYICONDATA lpdata);

_tShell_NotifyIconProc Shell_NotifyIconProc=0;
_tDllGetVersionProc DllGetVersion=0;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNxSTrayIcon::CNxSTrayIcon(HWND wnd, UINT cbmsg)
{
    DLLVERSIONINFO dvi;
	if (!DllGetVersion || !Shell_NotifyIconProc)
	{
		HMODULE shldll = LoadLibrary("shell32.dll");
		DllGetVersion = (_tDllGetVersionProc)GetProcAddress(shldll, "DllGetVersion");
		Shell_NotifyIconProc = (_tShell_NotifyIconProc)GetProcAddress(shldll, "Shell_NotifyIconA");
	}
    dvi.cbSize = sizeof(DLLVERSIONINFO);
    DllGetVersion(&dvi);
	m_created = FALSE;
	
	m_ID = g_ID++;
	m_hwnd = wnd;
	m_cbmsg = cbmsg;
	m_timeout = 0;
	m_hicon = 0;

	m_nif.cbSize = (dvi.dwMajorVersion >= 5)?(sizeof(NOTIFYICONDATA)):(NOTIFYICONDATA_V1_SIZE);
	m_nif.hWnd = m_hwnd;
	m_nif.uID = m_ID;
	m_nif.uFlags = NIF_MESSAGE;
	m_nif.uCallbackMessage = m_cbmsg;

	// inform shell controls we want old messaging style
	m_nif.uVersion = 0;
	Shell_NotifyIconProc(NIM_SETVERSION, (PNOTIFYICONDATA)&m_nif);
}

CNxSTrayIcon::~CNxSTrayIcon()
{
	Hide();
	//Shell_NotifyIcon(NIM_DELETE, (PNOTIFYICONDATA)&m_nif);
}

//////////////////////////////////////////////////////////////////////
// Public functions
//////////////////////////////////////////////////////////////////////

// call with NULL or empty ("") szInfo to hide balloon
BOOL CNxSTrayIcon::ShowBalloon(const char *szInfo, const char *szInfoTitle,
							   NXSTRAY_BTENUM balloonicon, BOOL nosound)
{
	if (!m_created) Show();

	m_nif.hWnd = m_hwnd;
	m_nif.uID = m_ID;

	m_nif.uFlags = NIF_INFO;
	m_nif.uTimeout = m_timeout;
	m_nif.dwInfoFlags = (int)balloonicon | (nosound?NIIF_NOSOUND:0);

/* Set the szInfo member */
	memset(m_nif.szInfo, '\000', sizeof(m_nif.szInfo));
	strncpy(m_nif.szInfo, szInfo, sizeof(m_nif.szInfo)-1);
/* Set the szInfoTitle member */
	memset(m_nif.szInfoTitle, '\000', sizeof(m_nif.szInfoTitle));
	strncpy(m_nif.szInfoTitle, szInfoTitle, sizeof(m_nif.szInfoTitle)-1);

	return Shell_NotifyIconProc(NIM_MODIFY, &m_nif);
}

BOOL CNxSTrayIcon::Hide()
{
	m_created = FALSE;
	return Shell_NotifyIconProc(NIM_DELETE, &m_nif);
}

BOOL CNxSTrayIcon::Show()
{
	int ret=0;

	m_created = TRUE;

	m_nif.hWnd = m_hwnd;
	m_nif.uID = m_ID;
	m_nif.uCallbackMessage = m_cbmsg;

	m_nif.uFlags = NIF_MESSAGE | NIF_TIP;
	if (m_hicon > 0)
	{
		m_nif.uFlags |= NIF_ICON;
		m_nif.hIcon = m_hicon;
	}

	memset(m_nif.szTip, '\000', sizeof(m_nif.szTip));
	strncpy(m_nif.szTip, m_tip, sizeof(m_nif.szTip)-1);
	return Shell_NotifyIconProc(NIM_ADD, &m_nif);
}

void CNxSTrayIcon::SetTooltip(const char *newtip)
{
	m_nif.hWnd = m_hwnd;
	m_nif.uID = m_ID;

	m_nif.uFlags = NIF_TIP;

	memset(m_tip, '\000', sizeof(m_tip));
	strncpy(m_tip, newtip, sizeof(m_tip)-1);

	if (m_created)
		Shell_NotifyIconProc(NIM_MODIFY, &m_nif);
}

void CNxSTrayIcon::SetIcon(HICON newicon)
{
	m_hicon = newicon;

	m_nif.hIcon = newicon;
	m_nif.uFlags = NIF_ICON;
	Shell_NotifyIconProc(NIM_MODIFY, &m_nif);
}

BOOL CNxSTrayIcon::IsVisible()
{
	return m_created;
}

BOOL CNxSTrayIcon::SetTimeout(DWORD newtimeout)
{
	m_timeout = newtimeout;
	return (m_timeout == newtimeout);
}

/* One single function instead of multiple Get... ones */
void* CNxSTrayIcon::GetValue(NXSTRAY_VALUE type)
{
	switch (type)
	{
	case tipvalue:      return (void*)m_tip;
	case iconvalue:     return (void*)m_hicon;
	case idvalue:       return (void*)m_ID;
	case timeoutvalue:  return (void*)m_timeout;
	case wndvalue:      return (void*)m_hwnd;
	case msgvalue:      return (void*)m_cbmsg;
	default: return NULL;
	}
}

/* static function > show balloon tooltip */
BOOL CNxSTrayIcon::PutBalloon(const char *szInfo, const char *szInfoTitle, DWORD flags, DWORD timeout, HWND hwnd, UINT id)
{
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA_V1_SIZE);
	nid.hWnd = hwnd;
	nid.uID = id;
	nid.uFlags = NIF_INFO;
	nid.dwInfoFlags = flags;
	nid.uTimeout = timeout;
/* Set the szInfo member */
	memset(nid.szInfo, '\000', sizeof(nid.szInfo));
	strncpy(nid.szInfo, szInfo, sizeof(nid.szInfo)-1);
/* Set the szInfoTitle member */
	memset(nid.szInfoTitle, '\000', sizeof(nid.szInfoTitle));
	strncpy(nid.szInfoTitle, szInfoTitle, sizeof(nid.szInfoTitle)-1);
	return Shell_NotifyIconProc(NIM_MODIFY, (PNOTIFYICONDATA)&nid);
}

/* static function > remove balloon tooltip */
BOOL CNxSTrayIcon::PopBalloon(HWND hwnd, UINT id)
{
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA_V1_SIZE);
	nid.hWnd = hwnd;
	nid.uID = id;
	nid.uFlags = NIF_INFO;
	memset(nid.szInfo, '\000', sizeof(nid.szInfo));
	return Shell_NotifyIconProc(NIM_MODIFY, (PNOTIFYICONDATA)&nid);
}
