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

#if !defined(__NXSTRAYICON_H)
#define __NXSTRAYICON_H

#if _MSC_VER > 1000
#pragma once
#endif

#include <windows.h>
#include <shellapi.h>

class CNxSTrayIcon
{
private:
	bool m_IsNewShell;
	bool m_created;
	bool m_visible;
	DWORD m_timeout;
	NOTIFYICONDATA m_nid;
public:
	CNxSTrayIcon(void);
	virtual ~CNxSTrayIcon(void);

	// Sets the callback message for the tray icon. Returns previous value.
	UINT SetCallbackMsg(UINT uMsg);

	// Associates a window with the tray icon. Returns previous value.
	HWND SetWnd(HWND hWnd);

	// Methods to manipulate tray icon after it is setup
	BOOL Show(void); // Shows the tray icon, or adds it to the taskbar if it's not already added.
	BOOL Hide(void); // Hides the tray icon
	BOOL IsVisible(void) { return m_visible; } // Don't rely on this to be true though.
	BOOL Remove(void); // Removes the tray icon from the taskbar. Do not confuse with Hide()!!

	DWORD SetTimeout(DWORD newtimeout); // Returns old timeout value
	DWORD GetTimeout(void) { return m_timeout; }

	HICON SetIcon(HICON newicon); // Returns previous icon.
	HICON GetIcon(void) { return m_nid.hIcon; }

	void SetTooltip(const char *newtip);
	const char * GetTooltip(void) { return m_nid.szTip; }

	// Balloon Popup methods

	// Displays a balloon style tooltip withs it's stem pointing at the tray icon
	BOOL ShowBalloon(const char *szInfo, const char *szInfoTitle, DWORD dwFlags);
	void HideBalloon(void);

	// Special stuff

	// Sets the id used for the tray icon. Normally you don't need to call this.
	void SetID(UINT uID) { m_nid.uID = uID; }

	// Sets the version for the tray icon. Modifies the messaging style and so on...
	BOOL SetVersion(int version);

	// Sets the focus to the tray icon. A side effect of this is that the
	// tooltip is displayed for about a second.
	BOOL SetFocus(void);

	// Sets the GUID for the tray icon. Used to uniquely distinguish between tray icons.
	BOOL SetGUID(const GUID Guid);

	// Call this when your window procedure receives the "TaskbarCreated" message
	// registered using RegisterWindowMessage(). It will add the icon to the taskbar
	// and show it, if it was previously visible. It will add it but not show it, if it
	// was previously hidden.
	void Restore(void);
	
	bool IsTaskbarAvailable(void); // Returns true if the taskbar is available

	// Attaches the CNxSTrayIcon object to an existing tray icon.
	// No error checking is performed, so make sure the parameters are valid.
	// This is actually the same as calling both SetWnd and SetID methods,
	// but I inluded this methods to follow the MFC style classes.
	void Attach(HWND wnd, UINT id);
	void Detach();

};

#endif // !defined(__NXSTRAYICON_H)
