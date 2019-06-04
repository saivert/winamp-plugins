/* xfadedlg.c: Main source file of XFade control for Winamp
 * Defines the General Purpose Plug-in and it's functions
 *
 * Later versions of Winamp (Winamp 2.91+ and Winamp 5) is
 * bundled with a DirectSound output plug-in version 2.2.8.
 * This version creates a hidden window which it uses to
 * communicate with Winamp.
 *
 * This plug-in takes advantage of that. It subclasses the
 * hidden window's window procedure so it can intercept changes
 * in the "end of song" fade settings or as we like to call it
 * "Crossfade". You can also alter these settings through this
 * "XFade" plug-in. This plug-in sends a message to the hidden
 * window to do this.
 *
 * Tech note: The window class of this hidden window is "DSound_IPC".
 * I used Spy++ in order to write this plug-in.
 *
 * The default Modern Skin that is used in Winamp 5 has an improved
 * equalizer where you can change the crossfade settings without
 * opening the preferences dialog of the DirectSound plug-in.
 *
 * The earlier versions of the default Modern Skin didn't have
 * this feature so the only way was to open the preferences dialog.
 * That's why I wrote this plug-in. I know now that it is obsolete.
 *
 * Author: Saivert
 * Homepage: http://saivertweb.no-ip.com
 * E-Mail: saivert@email.com
 */
#define _WIN32_WINNT 0x0501
#include "windows.h"
#include <commctrl.h>

#include "gen.h"
#define NO_IVIDEO_DECLARE
#include "wa_ipc.h"

#include "wa_dlg.h"
#include "ctrlskin.h"
#include "nxsweblink.h"
#include "NxSThingerAPI.h"

#include "resource.h"

// Magic numbers for controlling out_ds.dll
static const int XFADE_STATE = 0x64;
static const int XFADE_DURATION = 0x66;

static const int XFADE_GETDURATION = 0x67;
static const int XFADE_GETSTATE = 0x65;


// Global data
static const char szAppName[] = "NxS XFade";
#define PLUGIN_TITLE "NxS XFade control v0.7"
#define PLUGIN_DISABLED PLUGIN_TITLE" <DISABLED>"
#define PLUGIN_INISECTION szAppName
#define PLUGIN_READMEFILE "gen_xfadedlg.html"
#define PLUGIN_INVALIDOUTPUTPLUGIN "NxS XFade control requires the \"Nullsoft DirectSound\" output plug-in to be selected in order to function.\r\nDespite many believings NxS XFade control only controls this partcular output plug-in. It does not implement any fading support itself.\r\nIf you don't want to use DirectSound output plug-in nor the fading features you can disable NxS XFade control in the configuration dialog for NxS XFade control and enable it when you want to use DirectSound plug-in again."
#define JOENETWORKRADIO_URL "http://www.joenetwork.de/radio/"

// Menu ID's
#define MENUID_XFADE (ID_GENFF_LIMIT+102)

// Metrics
#define PLUGIN_CXWND 275
#define PLUGIN_CYWND 58

static BOOL g_isAttachedToEq=FALSE;
static BOOL g_hasAddedIconToThinger=FALSE;
static int g_oldplpos;
static ipc_xfadeinit;

static HWND g_xfadewnd;
static HWND g_dsoundipc;
static HWND g_hwndEQ;
static HWND g_hwndPE;

/* XFade window */
embedWindowState *ews;
BOOL CALLBACK XFadeDlgProc(HWND,UINT,WPARAM,LPARAM);

BOOL CALLBACK ConfigDlgProc(HWND,UINT,WPARAM,LPARAM);

/* subclass of DSound_IPC window */
WNDPROC lpWndProcOld;
LRESULT CALLBACK DSoundSubclass(HWND,UINT,WPARAM,LPARAM);

/* subclass of Winamp's main window */
WNDPROC lpWinampWndProcOld;
LRESULT CALLBACK WinampSubclass(HWND,UINT,WPARAM,LPARAM);

/* subclass of winamp's equalizer window */
WNDPROC lpEQWndProcOld;
LRESULT CALLBACK EQSubclass(HWND,UINT,WPARAM,LPARAM);

/* subclass of skinned frame window (GenWnd) */
LRESULT CALLBACK GenWndSubclass(HWND,UINT,WPARAM,LPARAM);
WNDPROC lpGenWndProcOld;

/* subclass of playlist editor window */
LRESULT CALLBACK PESubclass(HWND, UINT, WPARAM, LPARAM);
WNDPROC lpOldPEWndProc;


/* configuration items */
static int config_enabled=TRUE;
static int config_docktoeq=TRUE;
static int config_visible=TRUE;
static int config_x;
static int config_y;
static int config_skinconfig=1;
static int config_donotxfadeduration=10;
static int config_donotxfade=0;

/* configuration read/write */
void config_read();
void config_write();

void MySetSliderPos(int pos);
void WINAPI SetupTrackbar(HWND hwndTrack, UINT iPos,
  UINT iMin, UINT iMax, UINT iPageSize, UINT iLineSize, UINT iTicFreq);

/* Menu item functions */
void InsertMenuItemInWinamp();
void RemoveMenuItemFromWinamp();

/* plugin function prototypes */
void config(void);
void quit(void);
int init(void);

winampGeneralPurposePlugin plugin = {
	GPPHDR_VER,
	PLUGIN_TITLE,
	init,
	config,
	quit,
};


void config() {
	static HWND hwndCfg=0;
	MSG msg;

	if (IsWindow(hwndCfg)) {
		CtrlSkin_SetForegroundWindow(hwndCfg);
		return;
	}

	hwndCfg = CreateDialog(plugin.hDllInstance, "CONFIG", 0, ConfigDlgProc);

	while (IsWindow(plugin.hwndParent) && IsWindow(hwndCfg) && GetMessage(&msg, 0, 0, 0)) {
		if (!IsDialogMessage(hwndCfg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void quit() {

	/* Update position */
	config_x = ews->r.left;
	config_y = ews->r.top;

	config_write();

	if (GetWindowLongPtr(g_dsoundipc, GWLP_WNDPROC)==(LONG_PTR)DSoundSubclass)
		SetWindowLongPtr(g_dsoundipc, GWLP_WNDPROC, (LONG_PTR)lpWndProcOld);

	if (GetWindowLongPtr(g_hwndEQ, GWLP_WNDPROC)==(LONG_PTR)EQSubclass)
		SetWindowLongPtr(g_hwndEQ, GWLP_WNDPROC, (LONG_PTR)lpEQWndProcOld);

	if (GetWindowLongPtr(plugin.hwndParent, GWLP_WNDPROC)==(LONG_PTR)WinampSubclass)
		SetWindowLongPtr(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)lpWinampWndProcOld);

	/* Remove subclass of Playlist Editor window */
	if (g_hwndPE && GetWindowLongPtr(g_hwndPE, GWLP_WNDPROC)==(LONG) PESubclass)
		SetWindowLongPtr(g_hwndPE, GWLP_WNDPROC, (LONG) lpOldPEWndProc);

	DestroyWindow(g_xfadewnd); /* delete our window */
	DestroyWindow(ews->me);
	GlobalFree((HGLOBAL)ews);
}

int init() {
	HWND (*embedWindow)(embedWindowState *);
	INITCOMMONCONTROLSEX icc = {ICC_BAR_CLASSES, sizeof(INITCOMMONCONTROLSEX)};
	InitCommonControlsEx(&icc);
	RegisterNxSWebLink(plugin.hDllInstance);
	CtrlSkin_Init(plugin.hwndParent);
	config_read();

	if (!config_enabled) {
		plugin.description = PLUGIN_DISABLED;
		return 0;
	}

    /* Allocate an embedWindowState struct. */
	ews = (embedWindowState*) GlobalAlloc(GPTR, sizeof(embedWindowState));
	ews->flags = 0; //not using EMBED_FLAGS_NORESIZE;

    /* Get the pointer to the embedWindow function.
       Calls SendMessage with IPC_GET_EMBEDIF Winamp message. */
	*(void **)&embedWindow=(void*)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_EMBEDIF);

	embedWindow(ews);
	SetWindowText(ews->me, szAppName);

	g_xfadewnd = CreateDialog(plugin.hDllInstance, TEXT("XFADEDLG"), ews->me, XFadeDlgProc);

	g_hwndEQ=(HWND)SendMessage(plugin.hwndParent, WM_WA_IPC, IPC_GETWND_EQ, IPC_GETWND);


	if (config_docktoeq) {
		if (SendMessage(plugin.hwndParent, WM_WA_IPC, IPC_GETWND_EQ, IPC_ISWNDVISIBLE)) {
			RECT r;
			GetWindowRect(g_hwndEQ, &r);
			ews->r.top = r.bottom;
			ews->r.left = r.left;
			ews->r.right = ews->r.left+PLUGIN_CXWND;
			ews->r.bottom = ews->r.top+PLUGIN_CYWND;
			ShowWindow(ews->me, SW_SHOW);
		}
	} else {
		ews->r.top = config_y;
		ews->r.left = config_x;
		ews->r.right = ews->r.left+PLUGIN_CXWND;
		ews->r.bottom = ews->r.top+PLUGIN_CYWND;
		ShowWindow(ews->me, config_visible?SW_SHOW:SW_HIDE);
		SetWindowPos(ews->me, 0, config_x, config_y, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
	}



	InsertMenuItemInWinamp();

	/* Subclass Winamp's main window */
	lpWinampWndProcOld = (WNDPROC)SetWindowLongPtr(plugin.hwndParent, GWLP_WNDPROC, (LONG_PTR)WinampSubclass);

	/* Subclass Winamp's EQ window */
	lpEQWndProcOld = (WNDPROC)SetWindowLongPtr(g_hwndEQ, GWLP_WNDPROC, (LONG_PTR)EQSubclass);

	/* Subclass skinned window frame */
	lpGenWndProcOld = (WNDPROC)SetWindowLongPtr(ews->me, GWLP_WNDPROC, (LONG_PTR)GenWndSubclass);

	/* Subclass playlist editor window */
	g_hwndPE = (HWND)SendMessage(plugin.hwndParent, WM_WA_IPC, IPC_GETWND_PE, IPC_GETWND);
	lpOldPEWndProc = (WNDPROC)SetWindowLongPtr(g_hwndPE, GWLP_WNDPROC, (LONG) PESubclass);


	ipc_xfadeinit = SendMessage(plugin.hwndParent, WM_WA_IPC,
					(WPARAM)"NxSXFadeInit", IPC_REGISTER_WINAMP_IPCMESSAGE);

	PostMessage(plugin.hwndParent, WM_WA_IPC, 0, ipc_xfadeinit);

	return 0;
}

/* Subclass used to intercept changes in crossfade settings of
   DirectSound output plug-in. */
LRESULT CALLBACK DSoundSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT res = CallWindowProc(lpWndProcOld, hwnd, uMsg, wParam, lParam);
	if (!config_enabled || uMsg==WM_CLOSE)
		return res;

	if (uMsg==WM_USER) {
		int state;
		int duration;

		state=CallWindowProc(lpWndProcOld, hwnd, WM_USER, 0, XFADE_GETSTATE);
		CheckDlgButton(g_xfadewnd, IDC_ENABLEDCB, state);

		duration=CallWindowProc(lpWndProcOld, hwnd, WM_USER, 0, XFADE_GETDURATION);
		MySetSliderPos(duration/1000);
	}
	return res;
}

BOOL IsSameSong(void) {
	int plpos=SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS);
	if (g_oldplpos==plpos) {
		return 1;
	} else {
		g_oldplpos = plpos;
		return 0;
	}
}

LRESULT CALLBACK PESubclass(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	/* Winamp sends a WM_USER with wParam==666 and lParam==playlist index
	   to the playlist editor window when the song changes.
	   I'm now using this method to catch song changes after I had a discussion
	   with Shane Hird and DrO on the Winamp forums. */    

	static BOOL bcleared=FALSE;

	switch (message)
	{
	case WM_USER:
		if (wParam == 666)
		{
			if ((lParam & 0x40000000) == 0x40000000 && config_donotxfade)
			{
				if (bcleared)
				{
					char *curfile;
					int plpos;

					plpos = (lParam & 0x0FFFFFFF);
					if (plpos+1 < SendMessage(plugin.hwndParent, WM_WA_IPC, plpos+1, IPC_GETLISTLENGTH))
						curfile = (char*)SendMessage(plugin.hwndParent, WM_WA_IPC, plpos+1, IPC_GETPLAYLISTFILE);
					else
						curfile = 0;

					if (curfile)
					{
						basicFileInfoStruct bfi;
						//int state;

						bfi.filename = curfile;
						bfi.quickCheck = 0;
						bfi.title = GlobalAlloc(GPTR, 256);
						bfi.titlelen = GlobalSize((HGLOBAL)bfi.title);
						SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&bfi, IPC_GET_BASIC_FILE_INFO);
						GlobalFree((HGLOBAL)bfi.title);

						// MUST BE FIXED. When DO not xfade song shorter than X secs is turned on,
						// the plug-in just enables fades for all songs longer than X secs disregarding
						// actual fade user setting.
						//state = SendMessage(g_dsoundipc, WM_USER, 0, XFADE_GETSTATE);
						//if (state)
							SendMessage(g_dsoundipc, WM_USER,
								(bfi.length > config_donotxfadeduration) &&
								(SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_GETOUTPUTTIME) > config_donotxfadeduration), XFADE_STATE);
					}
				}
				bcleared = FALSE;
			} else
				bcleared = TRUE;
		}
		break;
	}
	return CallWindowProc((WNDPROC)lpOldPEWndProc,hwnd,message,wParam,lParam);
}

/* Subclass used to intercept show/hide of the EQ window.
   It's also used to notify the XFade dialog that Winamp is finished initializing. */
LRESULT CALLBACK WinampSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	if (!config_enabled || uMsg==WM_CLOSE) {
		return CallWindowProc(lpWinampWndProcOld, hwnd, uMsg, wParam, lParam);
	}

	/* Init time */
	if (uMsg==WM_WA_IPC && lParam==ipc_xfadeinit) {
		NxSThingerIconStruct ntis;
		int ipc_thinger;

		/* Add icon to NxS Thinger plugin */
		if (!g_hasAddedIconToThinger) {
			ntis.dwFlags = NTIS_ADD|NTIS_BITMAP;
			ntis.lpszDesc = "NxS XFade Control";
			ntis.hWnd = g_xfadewnd;
			ntis.uMsg = WM_USER+1;
			ntis.wParam = 0;
			ntis.lParam = 0;
			ntis.hBitmap = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_XFADE));
			ntis.hBitmapHighlight = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_XFADE_H));

			ipc_thinger = SendMessage(plugin.hwndParent, WM_WA_IPC,
				(WPARAM)"NxSThingerIPCMsg", IPC_REGISTER_WINAMP_IPCMESSAGE);

			SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&ntis, (LPARAM)ipc_thinger);

			g_hasAddedIconToThinger = TRUE;
		}
		return 0;
	}

	/* Callbacks */
	if (uMsg==WM_WA_IPC) {
		if (wParam==IPC_CB_WND_EQ && config_docktoeq) {
			if (lParam==IPC_CB_ONSHOWWND) {
				RECT r;
				GetWindowRect(g_hwndEQ, &r);
				ews->r.left = r.left;
				ews->r.top = r.bottom;
				ews->r.right = ews->r.left+PLUGIN_CXWND;
				ews->r.bottom = ews->r.top+PLUGIN_CYWND;
				ShowWindow(ews->me, SW_RESTORE);
				SetForegroundWindow(ews->me);
			} else if (lParam==IPC_CB_ONHIDEWND) {
				ShowWindow(ews->me, SW_HIDE);
			}
		}
		if (lParam==IPC_CB_OUTPUTCHANGED) {
			char *outplug;
			/* Get current output plug-in*/
			outplug = (char*) SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTPLUGIN);
			if (lstrcmpi(outplug, "out_ds.dll")) {
				/* Warn user about the (to me) obvious pitfalls of not using DirectSound plug-in */
				MessageBox(hwnd, PLUGIN_INVALIDOUTPUTPLUGIN, "NxS XFade control - Warning", MB_OK|MB_ICONWARNING);
			}
		}
	}

	/* Menu item clicks */
	if (uMsg==WM_COMMAND && HIWORD(wParam)==0 && LOWORD(wParam)==MENUID_XFADE) {
		SendMessage(g_xfadewnd, WM_USER+1, 0, 0);
		return 0;
	}

	/* Shutdown */
	if (uMsg==WM_CLOSE) {
		config_visible = IsWindowVisible(g_xfadewnd);
	}

	return CallWindowProc(lpWinampWndProcOld, hwnd, uMsg, wParam, lParam);
}

/* Subclass used to intercept WM_MOVE */
LRESULT CALLBACK EQSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT res = CallWindowProc(lpEQWndProcOld, hwnd, uMsg, wParam, lParam);
	if (!config_enabled) return res;

	if (uMsg==WM_MOVE && config_docktoeq) {
		RECT r;
		GetWindowRect(hwnd, &r);
		g_isAttachedToEq=TRUE;
		SetWindowPos(ews->me, hwnd, r.left, r.bottom, 0, 0, SWP_NOSIZE);
		g_isAttachedToEq=FALSE;
		return 0;
	}

	return res;
}

BOOL CALLBACK XFadeDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	
	switch (uMsg) {
	case WM_INITDIALOG:
	{
		WADlg_init(plugin.hwndParent);
		SetupTrackbar(GetDlgItem(hDlg, IDC_SLIDER1), 0, 0, 20, 5, 1, 1);

		CtrlSkin_SkinControls(hDlg, TRUE);

		SetTimer(hDlg, 1, 0, NULL);
		return TRUE;
	}
	case WM_TIMER:
	{
		if (wParam == 1) {
			char *outplug;
			/* Get current output plug-in*/
			outplug = (char*) SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTPLUGIN);
			if (lstrcmpi(outplug, "out_ds.dll")) {
				MessageBox(hDlg, PLUGIN_INVALIDOUTPUTPLUGIN, "NxS XFade control - Warning", MB_OK|MB_ICONWARNING);
			}
			/* Subclass the DSound_IPC window */
			g_dsoundipc = FindWindowEx(plugin.hwndParent, 0, "DSound_IPC", NULL);
			if (g_dsoundipc) {
				int state;
				int duration;

				state=SendMessage(g_dsoundipc, WM_USER, 0, XFADE_GETSTATE);
				CheckDlgButton(g_xfadewnd, IDC_ENABLEDCB, state);

				duration=SendMessage(g_dsoundipc, WM_USER, 0, XFADE_GETDURATION);
				MySetSliderPos(duration/1000);

				lpWndProcOld = (WNDPROC)SetWindowLong(g_dsoundipc, GWL_WNDPROC, (LONG)DSoundSubclass);

				KillTimer(hDlg, 1);

				SetTimer(hDlg, 2, 1000, NULL);
			} else {
				SetDlgItemText(g_xfadewnd, IDC_POSLABEL, "Retrying...");
			}
		}

		return TRUE;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_ENABLEDCB) {
			/* "IDC_ENABLEDCB" is not a auto-toggle checkbox */
			int state;
			state = SendMessage(g_dsoundipc, WM_USER, 0, XFADE_GETSTATE);
			SendMessage(g_dsoundipc, WM_USER, !state, XFADE_STATE);
			CheckDlgButton(hDlg, IDC_ENABLEDCB, !state);
		}
		if (LOWORD(wParam) == ID_POPUP1_OPENCONFIGURATION && HIWORD(wParam) == 0) {
			config();
		}
		return FALSE;
	case WM_USER+1:
		if (wParam==1) {
			ShowWindow(ews->me, SW_HIDE);
			config_visible = FALSE;
		} else {
			ShowWindow(ews->me, IsWindowVisible(ews->me)?SW_HIDE:SW_SHOW);
			config_visible = IsWindowVisible(ews->me);
		}
		return FALSE;
	case WM_HSCROLL:
	{
		char s[128];
		DWORD dwPos;
		dwPos = SendDlgItemMessage(hDlg, IDC_SLIDER1, TBM_GETPOS, 0, 0);
		wsprintf(s, "%d secs", dwPos);
		SetDlgItemText(hDlg, IDC_POSLABEL, s);
		SendMessage(g_dsoundipc, WM_USER, dwPos*1000, XFADE_DURATION);
		SendMessage(g_dsoundipc, WM_USER,
			IsDlgButtonChecked(hDlg, IDC_ENABLEDCB), XFADE_STATE);
	}
	return FALSE;
	case WM_CONTEXTMENU:
	{
		HMENU m;
		m = LoadMenu(plugin.hDllInstance, MAKEINTRESOURCE(IDR_MENU1));
		m = GetSubMenu(m, 0);
		TrackPopupMenu(m, TPM_LEFTBUTTON, LOWORD(lParam), HIWORD(lParam), 0, hDlg, NULL);
		return FALSE;
	}
	case WM_CLOSE:
		SendMessage(hDlg, WM_USER+1, 1, 0);
		return TRUE;

	default: return FALSE;
	}
}

LRESULT CALLBACK GenWndSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lres;


	if (uMsg==WM_GETMINMAXINFO && SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETSKININFO)==1) {
		LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;

		lpmmi->ptMaxSize.y = lpmmi->ptMinTrackSize.y = lpmmi->ptMaxTrackSize.y = PLUGIN_CYWND;
		ews->r.bottom = ews->r.top+PLUGIN_CYWND;
		
		lpmmi->ptMaxSize.x = lpmmi->ptMinTrackSize.x = lpmmi->ptMaxTrackSize.x = PLUGIN_CXWND;
		ews->r.right = ews->r.left+PLUGIN_CXWND;

		return 0;
	}

	/* Force resize the modern skin frame window */
    if (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETSKININFO)!=1) {
		HWND hwndFrame;

		hwndFrame = GetParent(GetParent(hwnd));

		SetWindowPos(hwndFrame, 0, 0, 0, PLUGIN_CXWND, PLUGIN_CYWND, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE);
		
	}


	lres = CallWindowProc(lpGenWndProcOld, hwnd, uMsg, wParam, lParam);
	

	if (uMsg==WM_SHOWWINDOW) {
		/* Check if Winamp is using a Modern Skin */
		if (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETSKININFO)!=1) {
			if (wParam) {
				/* Winamp is using a Modern skin. Remove item since the Freeform plugin
				adds menu items for all skinned frame windows. */
				RemoveMenuItemFromWinamp();
			} else {
				/* Insert menu item now since the window is hidden. */
				InsertMenuItemInWinamp();
			}
		} else {
			/* Winamp is using a Classic skin. Just check/uncheck the menu item. */
			HMENU WinampMenu;
			MENUITEMINFO mii;

			// get main menu
			WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);

			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_STATE;
			mii.fState = wParam?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(WinampMenu, MENUID_XFADE, FALSE, &mii);
		}
	}

	return lres;
}

void InsertMenuItemInWinamp()
{
	int i;
	HMENU WinampMenu;
	UINT id;

	// get main menu
	WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);

	// find menu item "main window"
	for (i=GetMenuItemCount(WinampMenu); i>=0; i--)
	{
		if (GetMenuItemID(WinampMenu, i) == 40258)
		{
			// find the separator
			do {
				id=GetMenuItemID(WinampMenu, ++i);
				if (id==MENUID_XFADE) return;
			} while (id != 0xFFFFFFFF);

			// insert menu just before the separator
			InsertMenu(WinampMenu, i-1, MF_BYPOSITION|MF_STRING, MENUID_XFADE, szAppName);
			break;
		}
	}
}

void RemoveMenuItemFromWinamp()
{
	HMENU WinampMenu;
	WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);
	RemoveMenu(WinampMenu, MENUID_XFADE, MF_BYCOMMAND);
}

void MySetSliderPos(int pos)
{
	char s[128];

	SendDlgItemMessage(g_xfadewnd, IDC_SLIDER1, TBM_SETPOS, 1, pos);
	wsprintf(s, "%d secs", pos);
	SetDlgItemText(g_xfadewnd, IDC_POSLABEL, s);
}


void WINAPI SetupTrackbar(HWND hwndTrack, UINT iPos,
  UINT iMin, UINT iMax, UINT iPageSize, UINT iLineSize, UINT iTicFreq)
{  
    SendMessage(hwndTrack, TBM_SETRANGE, 
        (WPARAM) TRUE,
        (LPARAM) MAKELONG(iMin, iMax));
    SendMessage(hwndTrack, TBM_SETPAGESIZE, 
        0, (LPARAM) iPageSize);
    SendMessage(hwndTrack, TBM_SETLINESIZE, 
        0, (LPARAM) iLineSize);

    SendMessage(hwndTrack, TBM_SETTICFREQ, 
        (LPARAM) iTicFreq, 0);

    SendMessage(hwndTrack, TBM_SETPOS, 
        (WPARAM) TRUE,
        (LPARAM) iPos);
 
    return; 
}


void OpenReadMe(HWND hwndParent) {
	char syntaxfile[MAX_PATH], *p;
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	GetModuleFileName(plugin.hDllInstance, syntaxfile, sizeof(syntaxfile));
	p=syntaxfile+lstrlen(syntaxfile);
	while (p >= syntaxfile && *p != '\\') p--;
	if (++p >= syntaxfile) *p = 0;
	lstrcat(syntaxfile, PLUGIN_READMEFILE);

	hFind = FindFirstFile(syntaxfile, &wfd);
	if (hFind == INVALID_HANDLE_VALUE) {
		MessageBox(hwndParent,
			"Syntax help not found!\r\n"
			"Ensure "PLUGIN_READMEFILE" is in Winamp\\Plugins folder.",
			szAppName, MB_ICONWARNING);
	} else {
		FindClose(hFind);
		ExecuteURL(syntaxfile);
		/* ShellExecute(hwndDlg, "open", syntaxfile, NULL, NULL, SW_SHOWNORMAL); */
	}
}

BOOL CALLBACK ConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	int i;

	switch (uMsg) {
	case WM_INITDIALOG:
		CheckDlgButton(hDlg, IDC_ENABLEDCHECK, config_enabled?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_DOCKTOEQ, config_docktoeq?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_SKINCONFIG, config_skinconfig?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_DONOTXFADECHECK, config_donotxfade?BST_CHECKED:BST_UNCHECKED);
		
		SetDlgItemInt(hDlg, IDC_EDIT1, config_donotxfadeduration, FALSE);

		if (config_skinconfig) {
			CtrlSkin_SkinControls(hDlg, TRUE);
			CtrlSkin_EmbedWindow(hDlg, TRUE, 1);
		}

		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			config_docktoeq = IsDlgButtonChecked(hDlg, IDC_DOCKTOEQ)==BST_CHECKED;
			config_skinconfig = IsDlgButtonChecked(hDlg, IDC_SKINCONFIG)==BST_CHECKED;

			config_donotxfade = IsDlgButtonChecked(hDlg, IDC_DONOTXFADECHECK)==BST_CHECKED;
			config_donotxfadeduration = GetDlgItemInt(hDlg, IDC_EDIT1, NULL, FALSE);

			i = config_enabled;
			config_enabled = IsDlgButtonChecked(hDlg, IDC_ENABLEDCHECK)==BST_CHECKED;
			/* Prompt user to restart Winamp if this option changed */
			if (config_enabled != i) {
				config_write();
				i = MessageBox(hDlg, "This change requires a restart of Winamp.\r\n"
					  "Do you wish to restart Winamp now?", szAppName, MB_YESNO|MB_ICONQUESTION);
				if (i==IDYES)
					PostMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_RESTARTWINAMP);
			}

			//EndDialog(hDlg, 1);
			DestroyWindow(hDlg);
			break;
		case IDCANCEL:
			//EndDialog(hDlg, 0);
			DestroyWindow(hDlg);
			break;
		case IDC_READMELINK:
			OpenReadMe(hDlg);
			break;
		case IDC_JOELINK:
			ExecuteURL(JOENETWORKRADIO_URL);
			break;
		case IDC_SKINCONFIG:
			{
				BOOL on = IsDlgButtonChecked(hDlg, IDC_SKINCONFIG)==BST_CHECKED;
				CtrlSkin_SkinControls(hDlg, on);
				CtrlSkin_EmbedWindow(hDlg, on, 1);
			}
			break;
		}
		break;
	case WM_DESTROY:
		CtrlSkin_SkinControls(hDlg, FALSE);
		CtrlSkin_EmbedWindow(hDlg, FALSE, 1);
		break;
	}
	return FALSE;
}


int GetPluginINIPath(char *lpszFilename)
{
	TCHAR *p;
	if (!GetModuleFileName(plugin.hDllInstance, lpszFilename, MAX_PATH)) return 0;
	p=lpszFilename+lstrlen(lpszFilename);
	while (p >= lpszFilename && *p != '\\') p = CharPrev(lpszFilename, p);
	if ((p = CharNext(p)) >= lpszFilename) *p = 0;
	lstrcat(lpszFilename, "plugin.ini");
	return 1;
}

/* Macros and functions to make it easier to read and write settings */

/* Why didn't Microsoft put this into KERNEL32.DLL? Well here it is... */
__inline BOOL WritePrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName,
									 int iInt, LPCTSTR lpFileName) {
	TCHAR szTmp[64];
	wsprintf(szTmp, "%ld", iInt);
	return WritePrivateProfileString(lpAppName, lpKeyName, szTmp, lpFileName);
}

/* Macros to read a value from an INI file with the same name as the varible itself */
#define INI_READ_INT_(x) x = GetPrivateProfileInt(PLUGIN_INISECTION, #x, x, ini_file);
#define INI_READ_INT(x) INI_READ_INT_(x)
#define INI_READ_STR_(x,def) GetPrivateProfileString(PLUGIN_INISECTION, #x, def, x, 512, ini_file);
#define INI_READ_STR(x,def) INI_READ_STR_(x,def)
/* Macros to write the value of a variable to an INI key with the same name as the variable itself */
#define INI_WRITE_INT_(x) WritePrivateProfileInt(PLUGIN_INISECTION, #x, x, ini_file);
#define INI_WRITE_INT(x) INI_WRITE_INT_(x)
#define INI_WRITE_STR_(x) WritePrivateProfileString(PLUGIN_INISECTION, #x, x, ini_file)
#define INI_WRITE_STR(x) INI_WRITE_STR_(x)

void config_read()
{
	TCHAR ini_file[MAX_PATH];
	if (!GetPluginINIPath(ini_file)) return;

	INI_READ_INT(config_enabled);
	INI_READ_INT(config_docktoeq);
	INI_READ_INT(config_x);
	INI_READ_INT(config_y);
	INI_READ_INT(config_visible);
	INI_READ_INT(config_skinconfig);
	INI_READ_INT(config_donotxfadeduration);
	INI_READ_INT(config_donotxfade);

}


void config_write()
{
	TCHAR ini_file[MAX_PATH];
	if (!GetPluginINIPath(ini_file)) return;

	INI_WRITE_INT(config_enabled);
	INI_WRITE_INT(config_docktoeq);
	INI_WRITE_INT(config_x);
	INI_WRITE_INT(config_y);
	INI_WRITE_INT(config_visible);
	INI_WRITE_INT(config_skinconfig);
	INI_WRITE_INT(config_donotxfadeduration);
	INI_WRITE_INT(config_donotxfade);

	WritePrivateProfileString(NULL, NULL, NULL, ini_file);
}

__declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin()
{
	return &plugin;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    return TRUE;
}

/*

// makes a smaller DLL file
BOOL WINAPI _DllMainCRTStartup(HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	return 1;
}
*/
//EOF
