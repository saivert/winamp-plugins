// NxSTrayIcon.h: interface for the CNxSTrayIcon class.
// Usage:
// 1) construct a CNxSTrayIcon
//   CNxSTrayIcon tray(mywnd, WM_MYTRAYMSG);
// Use WM_MYTRAYMSG (#defined by you) in your window procedure to
// check for messages.
// 2) Show the tray icon
//   tray.SetTooltip("a cool tray icon");
//   tray.SetIcon(g_htrayico);
//   tray.Show();
// SetTooltip() and Show() updates the tray icon if it is currently visible.
// 
// 3) Show a balloon info tip
//   tray.SetTimeout(5000); // depends on the OS defaults to 10000 (10secs)
//   tray.ShowBalloon("some text", "Please read...", NXSTRAY_BTWARNING);
// 4) Hide all
//   tray.Hide();
// 5) destroy (and hide balloon and icon)
//   delete tray;
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NXSTRAYICON_H__23A93E2E_7E62_4673_93D8_46BA3DB87CBA__INCLUDED_)
#define AFX_NXSTRAYICON_H__23A93E2E_7E62_4673_93D8_46BA3DB87CBA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

typedef enum _NXSTRAY_BTENUM
{
	NXSTRAY_BTNONE     = 0x00000000, /*No icon.*/
	NXSTRAY_BTINFO     = 0x00000001, /*An information icon.*/
	NXSTRAY_BTWARNING  = 0x00000002, /*A warning icon.*/
	NXSTRAY_BTERROR    = 0x00000003  /*An error icon.*/
} NXSTRAY_BTENUM;
/*This enum replaces the NIIF_NONE..NIIF_ERROR defines*/

typedef enum _NXSTRAY_VALUE
{
	tipvalue,
	iconvalue,
	idvalue,
	timeoutvalue,
	wndvalue,
	msgvalue
} NXSTRAY_VALUE;

class CNxSTrayIcon
{
private:
	UINT m_ID;
	UINT m_cbmsg;
	HWND m_hwnd;
	HICON m_hicon;
	DWORD m_timeout;
	BOOL m_created;
	char m_tip[128];
	NOTIFYICONDATA m_nif;
public:
	static BOOL PutBalloon(const char *szInfo, const char *szInfoTitle,
	  DWORD flags, DWORD timeout, HWND hwnd, UINT id);
	static BOOL PopBalloon(HWND hwnd, UINT id);
	void* GetValue(NXSTRAY_VALUE type);
	BOOL SetTimeout(DWORD newtimeout);
	BOOL IsVisible();
	void SetIcon(HICON newicon);
	void SetTooltip(const char *newtip);
	BOOL Show();
	BOOL Hide();
	BOOL ShowBalloon(const char *szInfo, const char *szInfoTitle,
		NXSTRAY_BTENUM balloonicon, BOOL nosound);
	CNxSTrayIcon(HWND wnd, UINT cbmsg);
	virtual ~CNxSTrayIcon();
};

#endif // !defined(AFX_NXSTRAYICON_H__23A93E2E_7E62_4673_93D8_46BA3DB87CBA__INCLUDED_)
