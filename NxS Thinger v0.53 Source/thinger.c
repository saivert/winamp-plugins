/* Thinger plugin for Winamp 2.x/5.x
 * Author: Saivert
 * Homepage: http://inthegray.com/saivert/
 * E-Mail: saivert@gmail.com
 *
 * BUILD NOTES:
 *  Before building this project, please check the post-build step in the project settings.
 *  You must make sure the path that the file is copied to is referring to where you have your
 *  Winamp plugins directory.
 */


// #define this to use the TransparentBlt GDI function
#define USE_TRANSPARENTBLT

#define _WIN32_WINNT 0x0501
#include "windows.h"
#include <commctrl.h>

#include "gen.h"
#define NO_IVIDEO_DECLARE
#include "wa_ipc.h"
#include "wa_hotkeys.h"
#include "wa_dlg.h"
#include "nxsweblink.h"
#include "iconlist.h"

#include "resource.h"

/* Include header for CtrlSkin module */
#include "ctrlskin.h"

/* global data */
static const char szAppName[] = "Thinger";
#define PLUGIN_INISECTION szAppName
#define PLUGIN_CAPTION "NxS Thinger control v0.53"
#define PLUGIN_DISABLED PLUGIN_CAPTION" <DISABLED>" 

#define PLUGIN_URL "http://inthegray.com/saivert/"
#define PLUGIN_READMEFILE "gen_thinger.html"

/* Metrics
   Note: Sizes must be in increments of 29
*/
#define WND_HEIGHT      87
#define WND_HEIGHT_NOSB 72
#define WND_WIDTH       275
#define WND_CXBTN       13
#define WND_CYSB        14
#define WND_CXICON      36
#define WND_CYICON      36

/* Repeat delay in ms */
#define SCROLLREPEATDELAY 60
/* How many pixels to shift each scroll */
#define SCROLLAMOUNT 5

/* Internal icons */
#define IDC_PLEDIT  32001
#define IDC_ML      32002
#define IDC_VIDEO   32003
#define IDC_VIS     32004
#define IDC_PREFS   32005
#define IDC_NEW     32006 //placeholder

// Menu ID's
#define MENUID_THINGER 48882

// Internal Thinger icon flags
// New in v0.515
#define THINGER_PLEDIT  2
#define THINGER_ML      4
#define THINGER_VIDEO   8
#define THINGER_VIS     16
#define THINGER_PREFS   32
#define THINGER_NEW     64 //placeholder

static HWND g_thingerwnd;
static BOOL g_debugmode=FALSE;
static int g_scrolloffset=0;

/* Thinger API stuff */
static LONG g_thingeripc;
static const char szThingerIPCMsg[] = "NxSThingerIPCMsg";

/* New in v0.517: Default bitmaps for invalid bitmap/icon handles */
static HBITMAP g_hbmDef;
static HBITMAP g_hbmDefHighlight;

static HICON g_hiDef;
static HICON g_hiDefHighlight;

/* Thinger window */
typedef HWND (*embedWindow_t)(embedWindowState *);
embedWindowState *ews;
DWORD_PTR CALLBACK ThingerDlgProc(HWND,UINT,WPARAM,LPARAM);

DWORD_PTR CALLBACK ConfigDlgProc(HWND,UINT,WPARAM,LPARAM);

BOOL IsClientCovered(HWND hwnd);

RECT GetIconRect(int index);
int GetIconFromPoint(POINT pt);
void UpdateIconView(HWND hDlg);
int GetNumVisibleIcons();

#ifdef USE_TRANSPARENTBLT
  // TransparentBlt
  typedef BOOL (WINAPI * TransparentBlt_func)(HDC hdcDst, int xDst, int yDst, int cxDst, int cyDst,
                                              HDC hdcSrc, int xSrc, int ySrc, int cxSrc, int cySrc,
                                              COLORREF TransColor);

  typedef BOOL (WINAPI * AlphaBlend_func)(HDC, int, int, int, int,
                                          HDC, int, int, int, int,
                                          BLENDFUNCTION);

#else
  void DrawTransparentBitmap(HDC hDC, HBITMAP hBitmap, int x, int y, COLORREF crColour);
#endif

/* subclass of Winamp's main window */
WNDPROC lpWinampWndProcOld;
LRESULT CALLBACK WinampSubclass(HWND,UINT,WPARAM,LPARAM);

/* subclass of skinned frame window (GenWnd) */
LRESULT CALLBACK GenWndSubclass(HWND,UINT,WPARAM,LPARAM);
WNDPROC lpGenWndProcOld;

/* subclass of skinned frame window (Modern Skins) */
LRESULT CALLBACK ModernGenWndSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
WNDPROC lpModernGenWndProcOld;


/* Menu item functions */
void InsertMenuItemInWinamp();
void RemoveMenuItemFromWinamp();


/* configuration items */
static int config_enabled=TRUE;
static int config_uselb=TRUE;
static int config_showsb=TRUE;
static int config_x=-1;
static int config_y=-1;
static int config_show=TRUE;
static int config_hideinticons=0; // New in v0.515
static int config_dimmedonscroll=0; // New in v0.518
static int config_skinconfig=1; // New in v0.53

/* configuration read/write */
void config_read();
void config_write();

/* plugin function prototypes */
void config(void);
void quit(void);
int init(void);

winampGeneralPurposePlugin plugin;


void config() {
	static HWND hwndCfg=NULL;
	MSG msg;

	if (IsWindow(hwndCfg)) {
		CtrlSkin_SetForegroundWindow(hwndCfg);
		return;
	}

	hwndCfg = CreateDialog(plugin.hDllInstance, "CONFIG", 0, ConfigDlgProc);
	ShowWindow(hwndCfg,SW_SHOW);

	while (IsWindow(hwndCfg) && IsWindow(plugin.hwndParent) && GetMessage(&msg, 0, 0, 0)) {
		if (!IsDialogMessage(hwndCfg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	hwndCfg = NULL;
}


void quit() {

	IconList_Free();

	SetWindowLongPtr(plugin.hwndParent, GWLP_WNDPROC, (LONG)lpWinampWndProcOld);

	/* Update position */
	config_x = ews->r.left;
	config_y = ews->r.top;

	config_write();

	DestroyWindow(g_thingerwnd); /* delete our window */
	DestroyWindow(ews->me);
	GlobalFree((HGLOBAL)ews);


}


int init() {
	RECT r;
	embedWindow_t embedWindow;
	NxSThingerIconStruct ntis;
	char szPath[MAX_PATH];
	char *p;
	HANDLE hFind;
	WIN32_FIND_DATA wfd;


	/* Must call this at least once to get the skinning done! */
	CtrlSkin_Init(plugin.hwndParent);

	config_read();

	if (!config_enabled) {
		plugin.description = PLUGIN_DISABLED;
		return 0;
	}


	/* Initialize our Icon List */
	IconList_Init();

	RegisterNxSWebLink(plugin.hDllInstance);

    /* Allocate an embedWindowState struct. */
	ews = (embedWindowState*) GlobalAlloc(GPTR, sizeof(embedWindowState));
	ews->flags = 0;

    /* Get the pointer to the embedWindow function.
       Calls SendMessage with IPC_GET_EMBEDIF Winamp message. */
	embedWindow=(embedWindow_t)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_EMBEDIF);

	embedWindow(ews);
	SetWindowText(ews->me, szAppName);

	g_thingerwnd = CreateDialog(plugin.hDllInstance, TEXT("THINGERDLG"), ews->me, ThingerDlgProc);

	if (config_x==-1 || config_y==-1) {
		GetWindowRect(plugin.hwndParent, &r);
		config_x = r.left;
		config_y = r.bottom;
	}


	ews->r.top = config_y;
	ews->r.left = config_x;
	ews->r.right = ews->r.left+WND_WIDTH;
	if (config_showsb) {
		ews->r.bottom = ews->r.top + WND_HEIGHT;
	} else {
		ews->r.bottom = ews->r.top + WND_HEIGHT_NOSB;
	}
	
	ShowWindow(ews->me, (config_enabled&&config_show)?SW_SHOW:SW_HIDE);

	/* Register our IPC message (for the Thinger API) */
	g_thingeripc = SendMessage(plugin.hwndParent, WM_WA_IPC,
		(WPARAM)szThingerIPCMsg, IPC_REGISTER_WINAMP_IPCMESSAGE);
	
	/* Subclass Winamp's main window */
	lpWinampWndProcOld = (WNDPROC)SetWindowLongPtr(plugin.hwndParent, GWLP_WNDPROC, (LONG)WinampSubclass);

	/* Subclass skinned window frame */
	lpGenWndProcOld = (WNDPROC)SetWindowLong(ews->me, GWL_WNDPROC, (LONG)GenWndSubclass);

	/* Load the default bitmaps and icons */
	g_hbmDef = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_NEW));
	g_hbmDefHighlight = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_NEW_H));

	g_hiDef = LoadIcon(plugin.hDllInstance, MAKEINTRESOURCE(IDI_NEW));
	g_hiDefHighlight = LoadIcon(plugin.hDllInstance, MAKEINTRESOURCE(IDI_NEW_H));


	/* Add the built-in icons to the icon list */
	/* Set common values */
	ntis.dwFlags = /*NTIS_ADD |*/ NTIS_BITMAP;
	ntis.hWnd = g_thingerwnd;
	ntis.uMsg = WM_COMMAND;
	ntis.wParam = 0;
	ntis.lParam = 0;

	/* Start adding icons to list */
	/* New in v0.515: Checks for presence of a flag in config_hideinticons which determines
	   if the icons are hidden or not. */
	if ((config_hideinticons & THINGER_PLEDIT)==THINGER_PLEDIT)
		ntis.dwFlags = NTIS_BITMAP|NTIS_HIDDEN;
	else
		ntis.dwFlags = NTIS_BITMAP;
	ntis.lpszDesc = "Playlist Editor";
	ntis.wParam = MAKEWPARAM(IDC_PLEDIT, 0);
	ntis.hBitmap = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_PLEDIT));
	ntis.hBitmapHighlight = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_PLEDIT_H));
	IconList_Add(&ntis);

	/* Check if the Media Library plugin (gen_ml.dll) is present */
	GetModuleFileName(plugin.hDllInstance, szPath, MAX_PATH);
	p = szPath+lstrlen(szPath);
	while (p >= szPath && *p != '\\') p = CharPrev(szPath, p);
	if ((p = CharNext(p)) >= szPath) *p = 0;
	lstrcat(szPath, "gen_ml.dll");

	hFind = FindFirstFile(szPath, &wfd);
	if (hFind != INVALID_HANDLE_VALUE) {
		FindClose(hFind);

		if ((config_hideinticons & THINGER_ML)==THINGER_ML)
			ntis.dwFlags = NTIS_BITMAP|NTIS_HIDDEN;
		else
			ntis.dwFlags = NTIS_BITMAP;
		ntis.lpszDesc = "Media Library";
		ntis.wParam = MAKEWPARAM(IDC_ML, 0);
		ntis.hBitmap = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_ML));
		ntis.hBitmapHighlight = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_ML_H));
		IconList_Add(&ntis);
	}



	if ((config_hideinticons & THINGER_VIDEO)==THINGER_VIDEO)
		ntis.dwFlags = NTIS_BITMAP|NTIS_HIDDEN;
	else
		ntis.dwFlags = NTIS_BITMAP;
	ntis.lpszDesc = "Video window";
	ntis.wParam = MAKEWPARAM(IDC_VIDEO, 0);
	ntis.hBitmap = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_VIDEO));
	ntis.hBitmapHighlight = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_VIDEO_H));
	IconList_Add(&ntis);

	if ((config_hideinticons & THINGER_VIS)==THINGER_VIS)
		ntis.dwFlags = NTIS_BITMAP|NTIS_HIDDEN;
	else
		ntis.dwFlags = NTIS_BITMAP;
	ntis.lpszDesc = "Toggle Visualizations";
	ntis.wParam = MAKEWPARAM(IDC_VIS, 0);
	ntis.hBitmap = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_VIS));
	ntis.hBitmapHighlight = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_VIS_H));
	IconList_Add(&ntis);

	if ((config_hideinticons & THINGER_PREFS)==THINGER_PREFS)
		ntis.dwFlags = NTIS_BITMAP|NTIS_HIDDEN;
	else
		ntis.dwFlags = NTIS_BITMAP;
	ntis.lpszDesc = "Toggle Preferences";
	ntis.wParam = MAKEWPARAM(IDC_PREFS, 0);
	ntis.hBitmap = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_PREFS));
	ntis.hBitmapHighlight = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_PREFS_H));
	IconList_Add(&ntis);


	if (config_enabled) {
		// Add menu item to Winamp's main menu, but only if using a classic skin
		if (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETSKININFO)==1) {
			InsertMenuItemInWinamp();

			/* Always remember to adjust "Option" submenu position.
			   Note: In Winamp 5.0+ this is unneccesary as it is more intelligent when
			   it comes to menus, but you must do it so it works with older versions. */
			SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_ADJUST_OPTIONSMENUPOS);
		}
	}
	
	SetTimer(g_thingerwnd, 1, 2000, NULL);

	return 0;
}



/* Subclass used to intercept our IPC messages and be notified when someone clicks
   the lightning bolt icon. */   
LRESULT CALLBACK WinampSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!config_enabled || uMsg==WM_CLOSE)
		return CallWindowProc(lpWinampWndProcOld, hwnd, uMsg, wParam, lParam);


	/* Check for our IPC message */
	if (uMsg == WM_WA_IPC && lParam == g_thingeripc) {
		lpNxSThingerIconStruct lpntis=(lpNxSThingerIconStruct)wParam;
		char s[256];

		if (g_debugmode)
			wsprintf(s, "flags: %ld\r\ndesc: \"%s\"\r\nicon: %ld\r\nhwnd: %ld\r\n"
				"msg: %ld\r\nwParam: %ld\r\nlParam: %ld",
				lpntis->dwFlags, lpntis->lpszDesc, lpntis->hIcon, lpntis->hWnd,
				lpntis->uMsg, lpntis->wParam, lpntis->lParam);

		if ((lpntis->dwFlags & NTIS_ADD) == NTIS_ADD) {
			unsigned int i=0;
			/* Safai was here! :-) */
			if (lpntis->hWnd == NULL) lpntis->hWnd = plugin.hwndParent;
			if (lpntis->hIcon && IsWindow(lpntis->hWnd)) {
				/* Substitute invalid icon/bitmap with a default one */
				if ((lpntis->dwFlags & NTIS_BITMAP) == NTIS_BITMAP) {
					if (!GetObjectType(lpntis->hBitmap))
						lpntis->hBitmap=g_hbmDef;

					if (!GetObjectType(lpntis->hBitmapHighlight))
						lpntis->hBitmapHighlight=g_hbmDefHighlight;
				} else {
					if (!GetObjectType(lpntis->hIcon))
						lpntis->hIcon=g_hiDef;

					if (!GetObjectType(lpntis->hIconHighlight))
						lpntis->hIconHighlight=g_hiDefHighlight;
				}

				/* Now add the damn icon */
				i = IconList_Add(lpntis);
			}

			if (g_debugmode)
				MessageBox(hwnd, s, "DEBUG: Got Add request!", 64);

			UpdateIconView(g_thingerwnd);
			return i;
		} else if ((lpntis->dwFlags & NTIS_MODIFY) == NTIS_MODIFY) {
			lpNxSThingerIconStruct lpntisCur;
			lpntisCur = IconList_GetFromID(lpntis->uIconId);

			if (lpntisCur != NULL) {

				if ((lpntis->dwFlags & NTIS_HIDDEN) == NTIS_HIDDEN) {
					lpntisCur->dwFlags |= NTIS_HIDDEN;
				} else {
					lpntisCur->dwFlags &= ~NTIS_HIDDEN;
				}

				if ((lpntis->dwFlags & NTIS_NODESC) != NTIS_NODESC)
					lpntisCur->lpszDesc = lpntis->lpszDesc;


				if ((lpntis->dwFlags & NTIS_NOICON) != NTIS_NOICON) {
					if ((lpntis->dwFlags & NTIS_BITMAP) == NTIS_BITMAP) {
						lpntisCur->dwFlags |= NTIS_BITMAP;

						lpntisCur->hBitmap = lpntis->hBitmap;
						lpntisCur->hBitmapHighlight = lpntis->hBitmapHighlight;
					} else {
						lpntisCur->dwFlags &= ~NTIS_BITMAP;

						lpntisCur->hIcon = lpntis->hIcon;
						lpntisCur->hIconHighlight = lpntis->hIconHighlight;
					}
				}
			}

			if (g_debugmode)
				MessageBox(hwnd, s, "DEBUG: Got Modify request!", 64);

			UpdateIconView(g_thingerwnd);
			if (lpntisCur) return lpntisCur->uIconId;
		} else if ((lpntis->dwFlags & NTIS_DELETE)==NTIS_DELETE) {
			IconList_DelWithID(lpntis->uIconId);

			if (g_debugmode)
				MessageBox(hwnd, s, "DEBUG: Got Delete request!", 64);

			UpdateIconView(g_thingerwnd);
		}
	}

	if (config_uselb) {
		/* Override action for Lightning bolt icon */
		if (uMsg==WM_COMMAND && LOWORD(wParam)==WINAMP_LIGHTNING_CLICK && lParam==0) {
			SendMessage(g_thingerwnd, WM_USER+1, 0, 0);
			return 0;
		}

		/* Override tooltip for Lightning bolt icon */
		if (uMsg==WM_WA_IPC && lParam==IPC_CB_GETTOOLTIP && wParam==16) {
				return (LRESULT)"Toggle Thinger";
		}
	}

	/* Menu item clicks */
	if (uMsg==WM_COMMAND && HIWORD(wParam)==0 && LOWORD(wParam)==MENUID_THINGER) {
		SendMessage(g_thingerwnd, WM_USER+1, 0, 0);
		return 0;
	}

	/* Menu item clicks (system menu) */
	if (uMsg==WM_SYSCOMMAND && LOWORD(wParam)==MENUID_THINGER) {
		SendMessage(g_thingerwnd, WM_USER+1, 0, 0);
		return 0;
	}

	return CallWindowProc(lpWinampWndProcOld, hwnd, uMsg, wParam, lParam);
}

/* New in v0.4: Calculates a rectangle based on index. Not aware of hidden icons,
   simply returns the rectangle of a "icon" at the given index.
   Only used by the "GetIconFromPoint" function. */
RECT GetIconRect(int index) {
	RECT tr;
	tr.left = (WND_CXBTN+(index*WND_CXICON))+g_scrolloffset;
	tr.top = 0;
	tr.right = (tr.left+WND_CXICON)-g_scrolloffset;
	tr.bottom = tr.top+WND_CYICON;
	return tr;
}

/* Fixed in v0.5: Accounts for hidden icons.
   Returns the index of the icon at point pt. */
int GetIconFromPoint(POINT pt) {
	int c;
	int i;
	int n;
	RECT r;

	c = IconList_GetSize();
	i = 0;
	n = 0;
	while (i < c) {
		if ((IconList_Get(i)->dwFlags & NTIS_HIDDEN)!=NTIS_HIDDEN) {
			r = GetIconRect(n);
			if (PtInRect(&r, pt))
				return i;
			n++;
		}
		i++;
	}
	return -1;
}

/* New in v0.512: Returns the number of visible icons */
int GetNumVisibleIcons() {
	int i, c, n;

	c = IconList_GetSize();
	n = 0;
	for (i=0;i<c;i++) {
		if ((IconList_Get(i)->dwFlags & NTIS_HIDDEN)!=NTIS_HIDDEN) n++;
	}
	
	return n;
}

/* New in v0.4: Invalidates the area where the icons are painted. */
void UpdateIconView(HWND hDlg) {
	RECT r;
	GetClientRect(hDlg, &r);
	
	r.left = WND_CXBTN;
	r.top = 0;
	r.right -= WND_CXBTN;
	if (config_showsb)
		r.bottom -= WND_CYSB;

	InvalidateRect(hDlg, &r, TRUE);
}


/* New in v0.4: Subclass used to make the buttons notify it's parent when down/up. */
WNDPROC oldButtonWndProc;
LRESULT CALLBACK ButtonSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_LBUTTONDOWN:
		SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hWnd), WM_LBUTTONDOWN), (LPARAM)hWnd);
		break;
	case WM_LBUTTONUP:
		SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hWnd), WM_LBUTTONUP), (LPARAM)hWnd);
		break;
	}

	return CallWindowProc(oldButtonWndProc, hWnd, uMsg, wParam, lParam);
}


DWORD_PTR CALLBACK ThingerDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	static UINT lastItem=0;
	static int iHighlightItem=-1;
	static UINT g_uLastIconID=0;
	static TransparentBlt_func pfnTransparentBlt;
	static AlphaBlend_func pfnAlphaBlend;
	static BOOL buttonsDown=FALSE;

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			/* Subclass controls in order to make them look right */
			CtrlSkin_SkinControls(hDlg, TRUE);

			/* Get function pointers to the "GdiAlphaBlend" and "GdiTransparentBlt" functions */
			pfnTransparentBlt = (TransparentBlt_func)GetProcAddress(GetModuleHandle("gdi32.dll"), "GdiTransparentBlt");
			pfnAlphaBlend = (AlphaBlend_func)GetProcAddress(GetModuleHandle("gdi32.dll"), "GdiAlphaBlend");


			/* Subclass buttons so we get notified when they are down/up */
			SetWindowLongPtr(GetDlgItem(hDlg, IDC_LEFTSCROLLBTN), GWLP_WNDPROC, (LONG)ButtonSubclass);
			SetWindowLongPtr(GetDlgItem(hDlg, IDC_RIGHTSCROLLBTN), GWLP_WNDPROC, (LONG)ButtonSubclass);
			return TRUE;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_LEFTSCROLLBTN:
			if (HIWORD(wParam)==WM_LBUTTONDOWN) {
				SetTimer(hDlg, IDC_LEFTSCROLLBTN, SCROLLREPEATDELAY, NULL);
				buttonsDown = TRUE;
			}
			if (HIWORD(wParam)==WM_LBUTTONUP) {
				KillTimer(hDlg, IDC_LEFTSCROLLBTN);
				buttonsDown = FALSE;
				UpdateIconView(hDlg);
			}

			break;
		case IDC_RIGHTSCROLLBTN:
			if (HIWORD(wParam)==WM_LBUTTONDOWN) {
				SetTimer(hDlg, IDC_RIGHTSCROLLBTN, SCROLLREPEATDELAY, NULL);
				buttonsDown = TRUE;
			}
			if (HIWORD(wParam)==WM_LBUTTONUP) {
				KillTimer(hDlg, IDC_RIGHTSCROLLBTN);
				buttonsDown = FALSE;
				UpdateIconView(hDlg);
			}

			break;

		case IDC_PLEDIT:
			SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_OPTIONS_PLEDIT, 0);
			break;
		case IDC_ML:
			{
				LONG libhwndipc;
				HWND hwndML;
				/* Use an undocumented API to get the Media Library window.
				   We need to get the handle to ML's window in order to check if
				   it is visible or not. */
				libhwndipc = (LONG)SendMessage(plugin.hwndParent, WM_WA_IPC,
					(WPARAM)"LibraryGetWnd", IPC_REGISTER_WINAMP_IPCMESSAGE);
				hwndML = (HWND)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, (LPARAM)libhwndipc);
				
				if (IsWindowVisible(hwndML)) {
					SendMessage(plugin.hwndParent, WM_COMMAND, ID_FILE_CLOSELIBRARY, 0);
				} else {
					SendMessage(plugin.hwndParent, WM_COMMAND, ID_FILE_SHOWLIBRARY, 0);
				}
			}
			break;
		case IDC_VIDEO:
			SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_OPTIONS_VIDEO, 0);
			break;
		case IDC_VIS:
			SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_VISPLUGIN, 0);
			break;
		case IDC_PREFS:
			SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_OPTIONS_PREFS, 0);
			break;
		case IDC_NEW:
			config();
			break;
		}
		return FALSE;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		{
			lpNxSThingerIconStruct lpntis;
			POINT pt;
			int i;

			GetCursorPos(&pt);
			ScreenToClient(hDlg, &pt);
			pt.x -= g_scrolloffset;
			i = GetIconFromPoint(pt);
			if (i >= 0 && i<IconList_GetSize()) {
				lpntis = IconList_Get(i);
				g_uLastIconID = lpntis->uIconId;
				if (uMsg==WM_LBUTTONUP)
					PostMessage(lpntis->hWnd, lpntis->uMsg, lpntis->wParam, lpntis->lParam);
			} else {
				g_uLastIconID = -1;
			}
		}
		return FALSE;
	case WM_CONTEXTMENU:
		{
			HMENU hPopup;
			int cmd;
			POINT pt;
			static int iconid=0;
			lpNxSThingerIconStruct lpntis;

			if ((HWND)wParam!=hDlg) break;


			hPopup = GetSubMenu(LoadMenu(plugin.hDllInstance, TEXT("MENU1")), 0);
			EnableMenuItem(hPopup, 11, MF_BYCOMMAND|(g_uLastIconID != -1)?MF_ENABLED:MF_GRAYED);
			CheckMenuItem(hPopup, 10, MF_BYCOMMAND|g_debugmode?MF_CHECKED:MF_UNCHECKED);
			
			// Check the "Show built-in icons" menu items
			if ((config_hideinticons & THINGER_PLEDIT)!=THINGER_PLEDIT)
				CheckMenuItem(hPopup, 20, MF_BYCOMMAND|MF_CHECKED);
			if ((config_hideinticons & THINGER_ML)!=THINGER_ML)
				CheckMenuItem(hPopup, 21, MF_BYCOMMAND|MF_CHECKED);
			if ((config_hideinticons & THINGER_VIDEO)!=THINGER_VIDEO)
				CheckMenuItem(hPopup, 22, MF_BYCOMMAND|MF_CHECKED);
			if ((config_hideinticons & THINGER_VIS)!=THINGER_VIS)
				CheckMenuItem(hPopup, 23, MF_BYCOMMAND|MF_CHECKED);
			if ((config_hideinticons & THINGER_PREFS)!=THINGER_PREFS)
				CheckMenuItem(hPopup, 24, MF_BYCOMMAND|MF_CHECKED);

			if (LOWORD(lParam) == -1 && HIWORD(lParam) == -1) {
				pt.x = pt.y = 1;
				ClientToScreen(hDlg, &pt);
			} else {
				GetCursorPos(&pt);
			}
			cmd = TrackPopupMenu(hPopup, TPM_RETURNCMD|TPM_NONOTIFY|TPM_LEFTBUTTON,
				pt.x, pt.y, 0, hDlg, NULL);
			DestroyMenu(hPopup);

			switch (cmd) {
			case 10:
				g_debugmode = !g_debugmode;
				break;
			case 11:
				{
					lpNxSThingerIconStruct lpntis;

					lpntis = IconList_GetFromID(g_uLastIconID);
					if (lpntis) {
						char s[256];
						wsprintf(s, "flags: %ld\r\ndesc: \"%s\"\r\nicon: %ld\r\nhwnd: %ld\r\n"
							"msg: %ld\r\nwParam: %ld\r\nlParam: %ld",
							lpntis->dwFlags, lpntis->lpszDesc, lpntis->hIcon, lpntis->hWnd,
							lpntis->uMsg, lpntis->wParam, lpntis->lParam);
						MessageBox(hDlg, s, "DEBUG: Icon info", 64);
					}
				}
				break;
			case 1:
				config();
				break;
			case 20:
				lpntis = IconList_Get(0);
				lpntis->dwFlags ^= NTIS_HIDDEN;
				config_hideinticons ^= THINGER_PLEDIT;
				UpdateIconView(hDlg);
				break;
			case 21:
				lpntis = IconList_Get(1);
				lpntis->dwFlags ^= NTIS_HIDDEN;
				config_hideinticons ^= THINGER_ML;
				UpdateIconView(hDlg);
				break;
			case 22:
				lpntis = IconList_Get(2);
				lpntis->dwFlags ^= NTIS_HIDDEN;
				config_hideinticons ^= THINGER_VIDEO;
				UpdateIconView(hDlg);
				break;
			case 23:
				lpntis = IconList_Get(3);
				lpntis->dwFlags ^= NTIS_HIDDEN;
				config_hideinticons ^= THINGER_VIS;
				UpdateIconView(hDlg);
				break;
			case 24:
				lpntis = IconList_Get(4);
				lpntis->dwFlags ^= NTIS_HIDDEN;
				config_hideinticons ^= THINGER_PREFS;
				UpdateIconView(hDlg);
				break;
			}
		}
		return FALSE;
	case WM_MOUSEMOVE:
		{
			int i;
			POINT pt;
			lpNxSThingerIconStruct lpntis;
			RECT r;
			TRACKMOUSEEVENT tme;

			GetCursorPos(&pt);
			ScreenToClient(hDlg, &pt);
			pt.x -= g_scrolloffset;
			i = GetIconFromPoint(pt);

			if (iHighlightItem!=i) {
				r = GetIconRect(iHighlightItem);
				InvalidateRect(hDlg, &r, TRUE);
				//UpdateIconView(hDlg);
			}

			if (i >= 0 && i < IconList_GetSize()) {
				lpntis = IconList_Get(i);
				SetDlgItemText(hDlg, IDC_EDIT1, lpntis->lpszDesc);
				if (iHighlightItem!=i) {
					r = GetIconRect(i);
					InvalidateRect(hDlg, &r, TRUE);
				}
			} else {
				SetDlgItemText(hDlg, IDC_EDIT1, "");
			}
			iHighlightItem = i;

			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = /*TME_HOVER|*/TME_LEAVE;
			tme.hwndTrack = hDlg;
			tme.dwHoverTime = 0;
			TrackMouseEvent(&tme);
		}
		return FALSE;
	case WM_MOUSELEAVE:
		{
			UpdateIconView(hDlg);

			iHighlightItem = -1;
			SetDlgItemText(hDlg, IDC_EDIT1, "");

		}
		return FALSE;
	case WM_USER+1:
		if (wParam==1) {
			ShowWindow(ews->me, SW_HIDE);
			config_show = FALSE;
		} else {
			ShowWindow(ews->me, IsWindowVisible(ews->me)?SW_HIDE:SW_SHOW);
			config_show = IsWindowVisible(ews->me);
		}
		return FALSE;
	case WM_CLOSE:
		SendMessage(hDlg, WM_USER+1, 1, 0);
		return TRUE;
	case WM_SIZE:
		{
			RECT r;
			HWND hLeftScrollBtn, hRightScrollBtn;
			GetClientRect(hDlg, &r);

			hLeftScrollBtn = GetDlgItem(hDlg, IDC_LEFTSCROLLBTN);
			hRightScrollBtn = GetDlgItem(hDlg, IDC_RIGHTSCROLLBTN);

			if (config_showsb) {
				MoveWindow(hLeftScrollBtn, 0, 0, WND_CXBTN, r.bottom-WND_CYSB, TRUE);
				MoveWindow(hRightScrollBtn, r.right-WND_CXBTN, 0, WND_CXBTN, r.bottom-WND_CYSB, TRUE);
				MoveWindow(GetDlgItem(hDlg, IDC_EDIT1), 0, r.bottom-WND_CYSB, r.right, r.bottom, TRUE);
			} else {
				MoveWindow(hLeftScrollBtn, 0, 0, WND_CXBTN, r.bottom, TRUE);
				MoveWindow(hRightScrollBtn, r.right-WND_CXBTN, 0, WND_CXBTN, r.bottom, TRUE);
			}
		}
		return FALSE;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HBRUSH hbr;
			RECT r;
			int i;
			int x;
			HBITMAP hbm, holdbm;
			HDC hdc, hdcwnd;

			GetClientRect(hDlg, &r);

			/*  Create double-buffer */
			hdcwnd = GetDC(hDlg);
			hdc = CreateCompatibleDC(hdcwnd);
			hbm = CreateCompatibleBitmap(hdcwnd, r.right, r.bottom);
			ReleaseDC(hDlg, hdcwnd);
			holdbm = (HBITMAP)SelectObject(hdc, hbm);


			/* Paint the background */
			hbr = CreateSolidBrush(WADlg_getColor(WADLG_ITEMBG));
			FillRect(hdc, &r, hbr);

			x=WND_CXBTN+g_scrolloffset;
			/* Paint the icons */
			for (i=0; i<IconList_GetSize(); i++) {
				lpNxSThingerIconStruct lpntis;
				lpntis = IconList_Get(i);

				if ((lpntis->dwFlags & NTIS_HIDDEN) != NTIS_HIDDEN) {
					// Determine draw method
					if ((lpntis->dwFlags & NTIS_BITMAP) == NTIS_BITMAP) {

#ifdef USE_TRANSPARENTBLT
						HDC icon_hdc;
						HBITMAP icon_hbm, icon_oldhbm;
						BITMAP bm = {0,};

						icon_hbm = iHighlightItem==i ? lpntis->hBitmapHighlight : lpntis->hBitmap;
						
						// Create a DC and select the bitmap into the DC
						icon_hdc = CreateCompatibleDC(hdc);
						icon_oldhbm = SelectObject(icon_hdc, icon_hbm);

						// Get bitmap dimensions
						GetObject(icon_hbm, sizeof(BITMAP), (LPSTR)&bm);

						// Call the TransparentBlt GDI function
						if (pfnTransparentBlt)
							pfnTransparentBlt(hdc, x, 0, WND_CXICON, WND_CYICON,
							icon_hdc, 0, 0, bm.bmWidth, bm.bmHeight, 0x00FF00FF);

						// Clean up
						SelectObject(icon_hdc, icon_oldhbm);
						DeleteDC(icon_hdc);
#else
						// Use an old "True Mask method" function
						DrawTransparentBitmap(hdc,
							iHighlightItem==i ? lpntis->hBitmapHighlight : lpntis->hBitmap,
							x, 0, 0x00FF00FF);
#endif
					} else {

						DrawIconEx(hdc, x, 0, (iHighlightItem==i) ? lpntis->hIconHighlight : lpntis->hIcon, WND_CXICON, WND_CYICON, 0, hbr, 0);

					}
					x += WND_CXICON;
				}
			}

			DeleteObject(hbr);

			hdcwnd = BeginPaint(hDlg, &ps);

			// Copy double-buffer to screen
			if (buttonsDown && config_dimmedonscroll) {
				if (pfnAlphaBlend) {
					BLENDFUNCTION bf={0,0,128,0};

					pfnAlphaBlend(hdcwnd, r.left, r.top, r.right, r.bottom,
					  hdc, 0, 0, r.right, r.bottom, bf);
				}
			} else {
				BitBlt(hdcwnd, r.left, r.top, r.right, r.bottom, hdc, 0, 0, SRCCOPY);
			}

			EndPaint(hDlg, &ps);

			// Destroy double-buffer
			SelectObject(hdc, holdbm);
			DeleteObject(hbm);
			DeleteDC(hdc);

		}
		return FALSE;
	case WM_TIMER:
		if (wParam==1) {
			UINT genhotkeys_add_ipc;
			genHotkeysAddStruct genhotkey;
			
			KillTimer(hDlg, 1);
			
			/* Get message value */
			genhotkeys_add_ipc = SendMessage(plugin.hwndParent, WM_WA_IPC,
				(WPARAM)"GenHotkeysAdd", IPC_REGISTER_WINAMP_IPCMESSAGE);
			
			/* Set up the genHotkeysAddStruct */
			genhotkey.name = "NxS Thinger: Toggle Thinger window";
			genhotkey.flags = HKF_NOSENDMSG;
			genhotkey.id = "NxSThingerToggle";
			genhotkey.uMsg = WM_USER;
			genhotkey.wParam = 1;
			genhotkey.lParam = 0;
			genhotkey.wnd = g_thingerwnd;
			SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&genhotkey, genhotkeys_add_ipc);
		}
		if (wParam==IDC_LEFTSCROLLBTN) {
			if (g_scrolloffset < 0)
				g_scrolloffset += SCROLLAMOUNT;
			UpdateIconView(hDlg);
		}
		if (wParam==IDC_RIGHTSCROLLBTN) {
			RECT r;
			GetClientRect(hDlg, &r);
			r.right -= (WND_CXBTN*2);
			if (g_scrolloffset > r.right-(GetNumVisibleIcons()*WND_CXICON))
				g_scrolloffset -= SCROLLAMOUNT;
			UpdateIconView(hDlg);
		}
		return TRUE;

	default: return FALSE;
	}
	return FALSE;
}


#ifdef USE_MODERNGENWND_SUBCLASS
LRESULT CALLBACK ModernGenWndSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lres;
	static BOOL moving=FALSE;

	if (uMsg==WM_ENTERSIZEMOVE) moving=TRUE;
	if (uMsg==WM_EXITSIZEMOVE) moving=FALSE;



	
	lres = CallWindowProc(lpModernGenWndProcOld, hwnd, uMsg, wParam, lParam);

	return lres;
}
#endif

LRESULT CALLBACK GenWndSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lres;

	if (uMsg==WM_GETMINMAXINFO && SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETSKININFO)==1) {
		LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;

		lpmmi->ptMaxSize.y = lpmmi->ptMinTrackSize.y = lpmmi->ptMaxTrackSize.y = config_showsb?WND_HEIGHT:WND_HEIGHT_NOSB;
		ews->r.bottom = ews->r.top+(config_showsb?WND_HEIGHT:WND_HEIGHT_NOSB);
		
		return 0;
	}

	/* Force resize the modern skin frame window */
    if (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETSKININFO)!=1) {
		HWND hwndFrame;
		RECT r;

		hwndFrame = GetParent(GetParent(hwnd));
		GetWindowRect(hwndFrame, &r);
		r.right -= r.left;
		// r.bottom -= r.top;

		SetWindowPos(hwndFrame, 0, 0, 0, r.right, config_showsb?WND_HEIGHT:WND_HEIGHT_NOSB, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE);
		
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

			/* get main menu */
			WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);

			mii.cbSize = sizeof(MENUITEMINFO);
			mii.fMask = MIIM_STATE;
			mii.fState = wParam?MFS_CHECKED:MFS_UNCHECKED;
			SetMenuItemInfo(WinampMenu, MENUID_THINGER, FALSE, &mii);
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
			// find the separator and return if menu item already exists
			do {
				id=GetMenuItemID(WinampMenu, ++i);
				if (id==MENUID_THINGER) return;
			} while (id != 0xFFFFFFFF);

			// insert menu just before the separator
			InsertMenu(WinampMenu, i-1, MF_BYPOSITION|MF_STRING, MENUID_THINGER, szAppName);
			break;
		}
	}
}

void RemoveMenuItemFromWinamp()
{
	HMENU WinampMenu;
	WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);
	RemoveMenu(WinampMenu, MENUID_THINGER, MF_BYCOMMAND);
}


void OpenSyntaxHelpAndReadMe(HWND hwndParent) {
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
			PLUGIN_CAPTION, MB_ICONWARNING);
	} else {
		FindClose(hFind);
		ExecuteURL(syntaxfile);
		/* ShellExecute(hwndDlg, "open", syntaxfile, NULL, NULL, SW_SHOWNORMAL); */
	}
}

DWORD_PTR CALLBACK ConfigDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	int i;

	switch (uMsg) {
	case WM_INITDIALOG:
		CheckDlgButton(hDlg, IDC_ENABLEDCHECK, config_enabled?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_SHOWSBCHECK, config_showsb?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_USELBCHECK, config_uselb?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_DIMWHENSCROLLCB, config_dimmedonscroll?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_SKINCONFIG, config_skinconfig?BST_CHECKED:BST_UNCHECKED);

		if (config_skinconfig) {
			CtrlSkin_EmbedWindow(hDlg, TRUE, 1);
			CtrlSkin_SkinControls(hDlg, TRUE);
		}
		
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			config_dimmedonscroll = IsDlgButtonChecked(hDlg, IDC_DIMWHENSCROLLCB)==BST_CHECKED;
			config_skinconfig = IsDlgButtonChecked(hDlg, IDC_SKINCONFIG)==BST_CHECKED;
			

			i = config_uselb;
			config_uselb = IsDlgButtonChecked(hDlg, IDC_USELBCHECK)==BST_CHECKED;

			/* Update tooltips (to handle changes to config_uselb)
			   To do this we toggle the Window shade twice.
			   But only do this is if config_uselb has changed. */
			if (config_uselb != i) {
				SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_OPTIONS_WINDOWSHADE, 0);
				SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_OPTIONS_WINDOWSHADE, 0);
			}

			i = config_showsb;
			config_showsb = IsDlgButtonChecked(hDlg, IDC_SHOWSBCHECK)==BST_CHECKED;
			/* Update NxS Thinger if config_showsb changed */
			if (config_showsb != i)
				MessageBox(hDlg, "Just resize the Thinger to make the changes take effect!",
				  szAppName, MB_OK|MB_ICONINFORMATION);

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
		case IDC_HOMEPAGE:
			ExecuteURL(PLUGIN_URL);
			break;
		case IDC_README:
			OpenSyntaxHelpAndReadMe(hDlg);
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
		/* Must destroy the frame window when the config dialog is destroyed. */
		CtrlSkin_SkinControls(hDlg, FALSE);
		CtrlSkin_EmbedWindow(hDlg, FALSE, 0);
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
	TCHAR szTmp[255];
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
	INI_READ_INT(config_x);
	INI_READ_INT(config_y);
	INI_READ_INT(config_uselb);
	INI_READ_INT(config_showsb);
	INI_READ_INT(config_show);
	INI_READ_INT(config_hideinticons);
	INI_READ_INT(config_dimmedonscroll);
	INI_READ_INT(config_skinconfig);
}

void config_write()
{
	TCHAR ini_file[MAX_PATH];
	if (!GetPluginINIPath(ini_file)) return;

	INI_WRITE_INT(config_enabled);
	INI_WRITE_INT(config_x);
	INI_WRITE_INT(config_y);
	INI_WRITE_INT(config_uselb);
	INI_WRITE_INT(config_showsb);
	INI_WRITE_INT(config_show);
	INI_WRITE_INT(config_hideinticons);
	INI_WRITE_INT(config_dimmedonscroll);
	INI_WRITE_INT(config_skinconfig);

	WritePrivateProfileString(NULL, NULL, NULL, ini_file);
}


/* [ DrawTransparentBitmap ]
* written by Paul Reynolds: Paul.Reynolds@cmgroup.co.uk
* CodeGuru article: "Transparent Bitmap - True Mask Method"
* http://codeguru.earthweb.com/bitmap/CISBitmap.shtml
*/
#ifndef USE_TRANSPARENTBLT
void DrawTransparentBitmap(HDC hDC, HBITMAP hBitmap, int x, int y, COLORREF crColour)
{
	COLORREF crOldBack = SetBkColor(hDC,RGB(255,255,255));
	COLORREF crOldText = SetTextColor(hDC,RGB(0,0,0));
	HDC dcImage, dcTrans;
	HGDIOBJ hOldBitmapImage;
	HBITMAP bitmapTrans;
	BITMAP bm = {0,};
	HGDIOBJ hOldBitmapTrans;
	int nWidth;
	int nHeight;
	
	// Create two memory dcs for the image and the mask
	dcImage = CreateCompatibleDC(hDC);
	dcTrans = CreateCompatibleDC(hDC);
	
	// Select the image into the appropriate dc
	hOldBitmapImage = SelectObject(dcImage,hBitmap);
	
	// Create the mask bitmap
	bitmapTrans = NULL;
	// get the image dimensions
	GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
	nWidth = bm.bmWidth;
	nHeight = bm.bmHeight;
	bitmapTrans = CreateBitmap(nWidth, nHeight, 1, 1, NULL);
	
	// Select the mask bitmap into the appropriate dc
	hOldBitmapTrans = SelectObject(dcTrans,bitmapTrans);
	
	// Build mask based on transparent colour
	SetBkColor(dcImage,crColour);
	BitBlt(dcTrans, 0, 0, nWidth, nHeight, dcImage, 0, 0, SRCCOPY);
	
	// Do the work - True Mask method - cool if not actual display
	BitBlt(hDC,x, y, nWidth, nHeight, dcImage, 0, 0, SRCINVERT);
	BitBlt(hDC,x, y, nWidth, nHeight, dcTrans, 0, 0, SRCAND);
	BitBlt(hDC,x, y, nWidth, nHeight, dcImage, 0, 0, SRCINVERT);
	
	// Restore settings
	// don't delete this, since it is the bitmap
	SelectObject(dcImage,hOldBitmapImage);
	// delete bitmapTrans
	DeleteObject(SelectObject(dcTrans,hOldBitmapTrans));
	SetBkColor(hDC,crOldBack);
	SetTextColor(hDC,crOldText);
	// clean up
	DeleteDC(dcImage);
	DeleteDC(dcTrans);
}
#endif // USE_TRANSPARENTBLT


#ifdef __cplusplus
extern "C"
#endif
__declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin() {
	plugin.version = GPPHDR_VER;
	plugin.description = PLUGIN_CAPTION;
	plugin.init = init;
	plugin.config = config;
	plugin.quit = quit;
	return &plugin;
}


BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	if (ul_reason_for_call==DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls((HMODULE)hModule);
	}
    return TRUE;
}

/* makes a smaller DLL file */
#ifndef _DEBUG
BOOL WINAPI _DllMainCRTStartup(HINSTANCE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	return 1;
}
#endif
