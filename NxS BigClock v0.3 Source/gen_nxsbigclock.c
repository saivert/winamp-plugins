/* NxS BigClock plugin for Winamp 2.x/5.x
 * Author: Saivert
 * Homepage: http://inthegray.com/saivert/
 * E-Mail: saivert@gmail.com
 *
 * BUILD NOTES:
 *  Before building this project, please check the post-build step in the project settings.
 *  You must make sure the path that the file is copied to is referring to where you have your
 *  Winamp plugins directory.
 */
#define _WIN32_WINNT 0x0501
#include "windows.h"
#include <commctrl.h>

#include "gen.h"
#define NO_IVIDEO_DECLARE
#include "wa_ipc.h"
#include "wa_hotkeys.h"
#include "wa_dlg.h"
#include "nxsweblink.h"

#include "NxSThingerAPI.h"

#include "resource.h"

/* Include header for CtrlSkin module */
#include "ctrlskin.h"

/* global data */
static const char szAppName[] = "NxS BigClock";
#define PLUGIN_INISECTION szAppName
#define PLUGIN_CAPTION "NxS BigClock control v0.3"
#define PLUGIN_DISABLED PLUGIN_CAPTION" <DISABLED>"
#define PLUGIN_URL "http://saivertweb.no-ip.com"
#define PLUGIN_READMEFILE "gen_nxsbigclock.html"

/* Metrics
   Note: Sizes must be in increments of 25x29
*/
#define WND_HEIGHT      116
#define WND_WIDTH       275

// Menu ID's
#define MENUID_BIGCLOCK (ID_GENFF_LIMIT+101)

// Display mode constants
#define NXSBCDM_ELAPSEDTIME 0
#define NXSBCDM_REMAININGTIME 1
#define NXSBCDM_PLELAPSED 2
#define NXSBCDM_PLREMAINING 3
#define NXSBCDM_TIMEOFDAY 4
#define NXSBCDM_MAX 4


typedef BOOL (WINAPI * AlphaBlend_func)(HDC, int, int, int, int,
                                      HDC, int, int, int, int,
                                      BLENDFUNCTION);



/* Thinger API stuff */
int g_fHasAddedIconToThinger=FALSE;

/* BigClock window */
static HWND g_BigClockWnd;
static TCHAR g_BigClockClassName[] = "NxSBigClockWnd";
typedef HWND (*embedWindow_t)(embedWindowState *);
embedWindowState *ews;
LRESULT CALLBACK BigClockWndProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static HMENU g_hPopupMenu=0;
static UINT ipc_bigclockinit;

void DrawVisualization(HDC hdc, RECT r, AlphaBlend_func);
void DrawAnalyzer(HDC hdc, RECT r, char *sadata);

BOOL GetFormattedTime(LPTSTR lpszTime, UINT size, int iPos);

DWORD_PTR CALLBACK ConfigDlgProc(HWND,UINT,WPARAM,LPARAM);

/* subclass of Winamp's main window */
WNDPROC lpWinampWndProcOld;
LRESULT CALLBACK WinampSubclass(HWND,UINT,WPARAM,LPARAM);

/* subclass of skinned frame window (GenWnd) */
LRESULT CALLBACK GenWndSubclass(HWND,UINT,WPARAM,LPARAM);
WNDPROC lpGenWndProcOld;

/* Menu item functions */
void InsertMenuItemInWinamp();
void RemoveMenuItemFromWinamp();

#define NXSBCVM_OSC 1
#define NXSBCVM_SPEC 2

/* configuration items */
static int config_enabled=TRUE;
static int config_show=TRUE;
static int config_x=-1;
static int config_y=-1;
static int config_width=WND_WIDTH;
static int config_height=WND_HEIGHT;
static int config_shadowtext=TRUE;
static int config_vismode=TRUE;
static int config_displaymode=NXSBCDM_ELAPSEDTIME;
static int config_skinconfig=1;
static int config_showms=1;

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
	ShowWindow(hwndCfg, SW_SHOW);

	while (IsWindow(hwndCfg) && IsWindow(plugin.hwndParent) && GetMessage(&msg, 0, 0, 0)) {
		if (!IsDialogMessage(hwndCfg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}


void quit() {

	/* Update position and size */
	config_x = ews->r.left;
	config_y = ews->r.top;
	config_width = ews->r.right-ews->r.left;
	config_height = ews->r.bottom-ews->r.top;

	config_write();

	DestroyWindow(g_BigClockWnd); /* delete our window */
	UnregisterClass(g_BigClockClassName, plugin.hDllInstance);
	DestroyWindow(ews->me);
	GlobalFree((HGLOBAL)ews);

	if ((LONG)WinampSubclass == GetWindowLongPtr(plugin.hwndParent, GWLP_WNDPROC))
		SetWindowLongPtr(plugin.hwndParent, GWLP_WNDPROC, (LONG)lpWinampWndProcOld);
}


int init() {
	RECT r;
	embedWindow_t embedWindow;
	ATOM wndclass;
	WNDCLASSEX wcex;

	/* Must call this at least once to get the skinning done! */
	CtrlSkin_Init(plugin.hwndParent);

	config_read();

	if (!config_enabled) {
		plugin.description = (PLUGIN_DISABLED);
		return 0;
	}

	ZeroMemory(&wcex, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpszClassName = g_BigClockClassName;
	wcex.hInstance = plugin.hDllInstance;
	wcex.lpfnWndProc = BigClockWndProc;
	wndclass = RegisterClassEx(&wcex);

	if (!wndclass) {
		MessageBox(plugin.hwndParent, "Error: Could not register window class!", szAppName, MB_OK|MB_ICONERROR);
		return 1;
	}


	RegisterNxSWebLink(plugin.hDllInstance);

    /* Allocate an embedWindowState struct. */
	ews = (embedWindowState*) GlobalAlloc(GPTR, sizeof(embedWindowState));
	ews->flags = 0;

    /* Get the pointer to the embedWindow function.
       Calls SendMessage with IPC_GET_EMBEDIF Winamp message. */
	embedWindow=(embedWindow_t)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_EMBEDIF);

	if (config_x==-1 || config_y==-1) {
		GetWindowRect(plugin.hwndParent, &r);
		config_x = r.left;
		config_y = r.bottom;
	}

	ews->r.top = config_y;
	ews->r.left = config_x;
	ews->r.right = ews->r.left + config_width;
	ews->r.bottom = ews->r.top + config_height;

	embedWindow(ews);
	SetWindowText(ews->me, szAppName);

	g_BigClockWnd = CreateWindowEx( 0, (LPCTSTR)wndclass, szAppName, WS_CHILD|WS_VISIBLE,
		0, 0, config_width, config_height,
		ews->me, NULL, plugin.hDllInstance, NULL);

	if (!g_BigClockWnd) {
		MessageBox(plugin.hwndParent, "Error: Could not create window!", szAppName, MB_OK|MB_ICONERROR);
		return 1;
	}

	ShowWindow(ews->me, (config_show)?SW_SHOW:SW_HIDE);

	/* Subclass Winamp's main window */
	lpWinampWndProcOld = (WNDPROC)SetWindowLongPtr(plugin.hwndParent, GWLP_WNDPROC, (LONG)WinampSubclass);

	/* Subclass skinned window frame */
	lpGenWndProcOld = (WNDPROC)SetWindowLong(ews->me, GWL_WNDPROC, (LONG)GenWndSubclass);


	// Add menu item to Winamp's main menu
	InsertMenuItemInWinamp();

	/* Always remember to adjust "Option" submenu position. */
	/* Note: In Winamp 5.0+ this is unneccesary as it is more intelligent when
	   it comes to menus, but you must do it so it works with older versions. */
	SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_ADJUST_OPTIONSMENUPOS);

	ipc_bigclockinit = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_REGISTER_WINAMP_IPCMESSAGE);
	PostMessage(plugin.hwndParent, WM_WA_IPC, 0, ipc_bigclockinit);

	return 0;
}



LRESULT CALLBACK WinampSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT res=0;

	if (!config_enabled) {
		return  CallWindowProc(lpWinampWndProcOld, hwnd, uMsg, wParam, lParam);
	}

	/* Init time */
	if (uMsg==WM_WA_IPC && lParam==ipc_bigclockinit) {
		UINT genhotkeys_add_ipc;
		genHotkeysAddStruct genhotkey;

		/* Get message value */
		genhotkeys_add_ipc = SendMessage(plugin.hwndParent, WM_WA_IPC,
			(WPARAM)"GenHotkeysAdd", IPC_REGISTER_WINAMP_IPCMESSAGE);

		/* Set up the genHotkeysAddStruct */
		genhotkey.name = "NxS BigClock: Toggle BigClock window";
		genhotkey.flags = HKF_NOSENDMSG;
		genhotkey.id = "NxSBigClockToggle";
		genhotkey.uMsg = WM_USER+1;
		genhotkey.wParam = 0;
		genhotkey.lParam = 0;
		genhotkey.wnd = g_BigClockWnd;
		SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&genhotkey, genhotkeys_add_ipc);

		if (!g_fHasAddedIconToThinger) {
			NxSThingerIconStruct ntis;
			UINT thingeripc;

			ntis.dwFlags = NTIS_ADD | NTIS_BITMAP;
			ntis.lpszDesc = (char*)szAppName;
			ntis.hBitmap = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_ICON));
			ntis.hBitmapHighlight = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_ICON_H));
			ntis.hWnd = g_BigClockWnd;
			ntis.uMsg = WM_USER+1;
			ntis.wParam = 0;
			ntis.lParam = 0;

			/* Get the message number. */
			thingeripc = SendMessage(plugin.hwndParent, WM_WA_IPC,
				(WPARAM)&"NxSThingerIPCMsg", IPC_REGISTER_WINAMP_IPCMESSAGE);

			/* Add the icon to NxS Thinger control */

			if (SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&ntis, thingeripc))	
				g_fHasAddedIconToThinger = TRUE;
		}
	}

	if (uMsg==WM_TIMER)
		PostMessage(g_BigClockWnd, WM_USER+3, 0, 0);


	/* Menu item clicks */
	if (uMsg==WM_COMMAND && HIWORD(wParam)==0 && LOWORD(wParam)==MENUID_BIGCLOCK) {
		SendMessage(g_BigClockWnd, WM_USER+1, 0, 0);
		return 0;
	}
	/* Menu item clicks (system menu) */
	if (uMsg==WM_SYSCOMMAND && LOWORD(wParam)==MENUID_BIGCLOCK) {
		SendMessage(g_BigClockWnd, WM_USER+1, 0, 0);
		return 0;
	}

	return CallWindowProc(lpWinampWndProcOld, hwnd, uMsg, wParam, lParam);
}


BOOL GetFormattedTime(LPTSTR lpszTime, UINT size, int iPos) {

	double time_s;
	int hours, minutes, seconds, dsec;
	char szFmt[] = "%d:%.2d:%.2d\0";
	char szMsFmt[] = ".%.2d";

	time_s    = iPos*0.001;
	hours     = (int)(time_s / 60 / 60) % 60;
	time_s   -= (hours*60*60);
	minutes   = (int)(time_s/60);
	time_s   -= (minutes*60);
	seconds   = (int)(time_s);
	time_s   -= seconds;
	dsec      = (int)(time_s*100);
	

	if (hours > 0)
		wsprintf(lpszTime, szFmt, hours, minutes, seconds);
	else wsprintf(lpszTime, szFmt+3, minutes, seconds);

	if (config_showms)
		wsprintf(lpszTime+lstrlen(lpszTime), szMsFmt, dsec);

	return 1;
}

LRESULT CALLBACK BigClockWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	static AlphaBlend_func pfnAlphaBlend;

	switch (uMsg) {
	case WM_CREATE:
		{
			WADlg_init(plugin.hwndParent);

			g_hPopupMenu = GetSubMenu(LoadMenu(plugin.hDllInstance, TEXT("MENU1")), 0);

			CheckMenuItem(g_hPopupMenu, ID_CONTEXTMENU_SHADOWEDTEXT,
				MF_BYCOMMAND|(config_shadowtext?MF_CHECKED:MF_UNCHECKED));

			CheckMenuItem(g_hPopupMenu, ID_CONTEXTMENU_SHOWOSC,
				MF_BYCOMMAND|((config_vismode&NXSBCVM_OSC)==NXSBCVM_OSC?MF_CHECKED:MF_UNCHECKED));

			CheckMenuItem(g_hPopupMenu, ID_CONTEXTMENU_SHOWSPEC,
				MF_BYCOMMAND|((config_vismode&NXSBCVM_SPEC)==NXSBCVM_SPEC?MF_CHECKED:MF_UNCHECKED));

			CheckMenuRadioItem(g_hPopupMenu, ID_CONTEXTMENU_ELAPSED, ID_CONTEXTMENU_TIMEOFDAY,
				ID_CONTEXTMENU_ELAPSED+config_displaymode, MF_BYCOMMAND|MF_CHECKED);

			CheckMenuItem(g_hPopupMenu, ID_SHOWMS,
				MF_BYCOMMAND|(config_showms?MF_CHECKED:MF_UNCHECKED));


			/* Get function pointer to the "GdiAlphaBlend" function */
			pfnAlphaBlend = (AlphaBlend_func)GetProcAddress(GetModuleHandle("gdi32.dll"), "GdiAlphaBlend");


			SetTimer(hWnd, 1, 10, NULL);
			return 1;
		}
	case WM_DESTROY:
		DestroyMenu(g_hPopupMenu);
		KillTimer(hWnd, 1);
		break;
	case WM_DISPLAYCHANGE:
		if (wParam==0 && lParam==0) WADlg_init(plugin.hwndParent);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDM_CONFIG:
			config();
			break;
		case ID_CONTEXTMENU_ELAPSED:
		case ID_CONTEXTMENU_REMAINING:
		case ID_CONTEXTMENU_PLAYLISTELAPSED:
		case ID_CONTEXTMENU_PLAYLISTREMAINING:
		case ID_CONTEXTMENU_TIMEOFDAY:
			CheckMenuRadioItem(g_hPopupMenu, ID_CONTEXTMENU_ELAPSED, ID_CONTEXTMENU_TIMEOFDAY,
				LOWORD(wParam), MF_BYCOMMAND|MF_CHECKED);
			config_displaymode = LOWORD(wParam)-ID_CONTEXTMENU_ELAPSED;
			break;
		case ID_CONTEXTMENU_SHADOWEDTEXT:
			config_shadowtext = !config_shadowtext;
			CheckMenuItem(g_hPopupMenu, ID_CONTEXTMENU_SHADOWEDTEXT,
				MF_BYCOMMAND|(config_shadowtext?MF_CHECKED:MF_UNCHECKED));
			break;
		case ID_CONTEXTMENU_SHOWOSC:
			config_vismode ^= NXSBCVM_OSC;
			CheckMenuItem(g_hPopupMenu, ID_CONTEXTMENU_SHOWOSC,
				MF_BYCOMMAND|((config_vismode&NXSBCVM_OSC)==NXSBCVM_OSC?MF_CHECKED:MF_UNCHECKED));
			break;
		case ID_CONTEXTMENU_SHOWSPEC:
			config_vismode ^= NXSBCVM_SPEC;
			CheckMenuItem(g_hPopupMenu, ID_CONTEXTMENU_SHOWSPEC,
				MF_BYCOMMAND|((config_vismode&NXSBCVM_SPEC)==NXSBCVM_SPEC?MF_CHECKED:MF_UNCHECKED));
			break;
		case ID_SHOWMS:
			config_showms = !config_showms;
			CheckMenuItem(g_hPopupMenu, ID_SHOWMS,
				MF_BYCOMMAND|(config_showms?MF_CHECKED:MF_UNCHECKED));
			break;
		case ID_CONTEXTMENU_CANCEL:
			/* D O   N O T H I N G */
			break;
		}
		return 0;
	case WM_LBUTTONUP:
		if (config_displaymode>=NXSBCDM_MAX)
			config_displaymode = 0;
		else
			config_displaymode++;

		CheckMenuRadioItem(g_hPopupMenu, ID_CONTEXTMENU_ELAPSED, ID_CONTEXTMENU_TIMEOFDAY,
			ID_CONTEXTMENU_ELAPSED+config_displaymode, MF_BYCOMMAND|MF_CHECKED);

		return 0;
	case WM_CONTEXTMENU:
		{
			HMENU hPopup;
			POINT pt;
			static int iconid=0;

			if ((HWND)wParam!=hWnd) break;

			hPopup = g_hPopupMenu;

			if (LOWORD(lParam) == -1 && HIWORD(lParam) == -1) {
				pt.x = pt.y = 1;
				ClientToScreen(hWnd, &pt);
			} else {
				GetCursorPos(&pt);
			}
			TrackPopupMenu(hPopup, TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);

		}
		return 1;
	case WM_USER+1:
		if (wParam==1) {
			ShowWindow(ews->me, SW_HIDE);
			config_show = FALSE;
		} else {
			ShowWindow(ews->me, IsWindowVisible(ews->me)?SW_HIDE:SW_SHOW);
			config_show = IsWindowVisible(ews->me);
		}
		return 0;
	case WM_CLOSE:
		SendMessage(hWnd, WM_USER+1, 1, 0);
		return 1;
	case WM_TIMER:
		if (wParam==1) {
			InvalidateRect(hWnd, NULL, FALSE);
			return 1;
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HBRUSH hbr;
			RECT r;
			HFONT hf, holdfont;
			LOGFONT lf;
			TCHAR szTime[256];

// Using COMCTL32.DLL's DrawShadowText function is slow,
// that's why I have an ifdef for it.
#ifdef USE_COMCTL_DRAWSHADOWTEXT
			WCHAR wszTime[256];
#endif

			int pos; // The position we display in our big clock
			LPTSTR lpszDisplayMode;
			SIZE s;
			static UINT prevplpos=0;

			HBITMAP hbm, holdbm;
			HDC hdc, hdcwnd;

			BOOL bShouldDrawVis;


			/* Only draw visualization if Winamp is playing music and one of the visualization modes are on */
			bShouldDrawVis = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_ISPLAYING) >= 1 &&
				((config_vismode & NXSBCVM_OSC) == NXSBCVM_OSC || (config_vismode & NXSBCVM_SPEC) == NXSBCVM_SPEC);				
				


			GetClientRect(hWnd, &r);

			// Create double-buffer
			hdc = CreateCompatibleDC(NULL);
			hdcwnd = GetDC(hWnd);
			hbm = CreateCompatibleBitmap(hdcwnd, r.right, r.bottom);
			ReleaseDC(hWnd, hdcwnd);
			holdbm = (HBITMAP)SelectObject(hdc, hbm);

			// Paint the background
			SetBkColor(hdc, WADlg_getColor(WADLG_ITEMBG));
			SetTextColor(hdc, WADlg_getColor(WADLG_ITEMFG));
			SetBkMode(hdc, TRANSPARENT);
			
			hbr = CreateSolidBrush(WADlg_getColor(WADLG_ITEMBG));
			FillRect(hdc, &r, hbr);
			DeleteObject(hbr);

			/* Draw visualization here if we dont' got access to the "GdiAlphaBlend" function */
			if (!pfnAlphaBlend && bShouldDrawVis)
				DrawVisualization(hdc, r, NULL);

			ZeroMemory(&lf, sizeof(LOGFONT));
			lf.lfHeight = -50;
			lstrcpyn(lf.lfFaceName, "Lucida Console", LF_FACESIZE);
			hf = CreateFontIndirect(&lf);
			holdfont = (HFONT)SelectObject(hdc, hf);

			switch (config_displaymode) {
			case NXSBCDM_ELAPSEDTIME:
				pos = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTTIME);
				lpszDisplayMode = "Elapsed";
				break;
			case NXSBCDM_REMAININGTIME:
				pos = (SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_GETOUTPUTTIME)*1000)-
					SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTTIME);
				lpszDisplayMode = "Remaining";
				break;
			case NXSBCDM_PLELAPSED:
				{
					UINT plpos;
					static UINT pltime;
					UINT i;
					basicFileInfoStruct bfi;

					bfi.quickCheck = 0;
					bfi.title = 0;
					bfi.titlelen = 0;

					plpos = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS);

					// Only do this when song has changed, since the code encapsulated by this
					// if clause is very time consuming if the playlist is large.
					if (plpos != prevplpos) {
						prevplpos = plpos;

						// Get combined duration of all songs up to (but not including) the current song
						pltime = 0;
						for (i=0;i<plpos;i++) {
							bfi.filename = (char*)SendMessage(plugin.hwndParent, WM_WA_IPC, i, IPC_GETPLAYLISTFILE);
							SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&bfi, IPC_GET_BASIC_FILE_INFO);
							pltime += bfi.length;
						}
						pltime *= 1000; // s -> ms
					}
					// Add elapsed time of current song and store result in pos
					pos = pltime+SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTTIME);

					lpszDisplayMode = "Playlist Elapsed";
				}
				break;
			case NXSBCDM_PLREMAINING:
				{
					UINT pllen, plpos;
					static UINT pltime;
					UINT i;
					basicFileInfoStruct bfi;

					bfi.quickCheck = 0;
					bfi.title = 0;
					bfi.titlelen = 0;
				
					pllen = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTLENGTH);
					plpos = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS);

					// Only do this when song has changed, since the code encapsulated by this
					// if clause is very time consuming if the playlist is large.
					if (plpos != prevplpos) {
						prevplpos = plpos;

						// Get combined duration of all songs from and including the current song to end of list
						pltime = 0;
						for (i=plpos;i<pllen;i++) {
							bfi.filename = (char*)SendMessage(plugin.hwndParent, WM_WA_IPC, i, IPC_GETPLAYLISTFILE);
							SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&bfi, IPC_GET_BASIC_FILE_INFO);
							pltime += bfi.length;
						}
						pltime *= 1000; // s -> ms
					}

					// Subtract elapsed time of current song and store result in pos
					pos = (pltime-SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTTIME));
					
					lpszDisplayMode = "Playlist Remaining";
				}
				break;
			case NXSBCDM_TIMEOFDAY:
				{
					SYSTEMTIME st;

					GetLocalTime(&st);

					pos = ((st.wHour*60*60)+(st.wMinute*60)+st.wSecond)*1000+st.wMilliseconds;

					lpszDisplayMode = "Time of day";
				}
				break;
			}
			
			GetFormattedTime(szTime, sizeof(szTime)/sizeof(szTime[0]), pos);

			if (config_shadowtext) {
#ifdef USE_COMCTL_DRAWSHADOWTEXT
				MultiByteToWideChar(CP_ACP, 0, szTime, -1, wszTime, sizeof(wszTime)/sizeof(wszTime[0]));
				DrawShadowText(hdc, wszTime, -1, &r, DT_CENTER|DT_VCENTER|DT_SINGLELINE,
						WADlg_getColor(WADLG_ITEMFG), 0x00808080, 5, 5);
#else
				// Draw text's "shadow"
				r.left += 5;
				r.top += 5;
				SetTextColor(hdc, 0x00808080);
				DrawText(hdc, szTime, -1, &r, DT_CENTER|DT_VCENTER|DT_SINGLELINE);

				// Draw text
				r.left -= 5;
				r.top -= 5;
				SetTextColor(hdc, WADlg_getColor(WADLG_ITEMFG));
				DrawText(hdc, szTime, -1, &r, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
				
#endif
			} else {
				DrawText(hdc, szTime, -1, &r, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
			}

			SelectObject(hdc, holdfont);
			DeleteObject(hf);


			/* Draw visualization here if we got access to "GdiAlphaBlend" function */
			if (pfnAlphaBlend && bShouldDrawVis)
				DrawVisualization(hdc, r, pfnAlphaBlend);


			ZeroMemory(&lf, sizeof(LOGFONT));
			lf.lfHeight = -13;
			lstrcpyn(lf.lfFaceName, "Arial", LF_FACESIZE);
			hf = CreateFontIndirect(&lf);
			holdfont = (HFONT)SelectObject(hdc, hf);

			GetTextExtentPoint32(hdc, lpszDisplayMode, lstrlen(lpszDisplayMode), &s);
			
			TextOut(hdc, 0, r.bottom-s.cy, lpszDisplayMode, lstrlen(lpszDisplayMode));

			SelectObject(hdc, holdfont);
			DeleteObject(hf);

			hdcwnd = BeginPaint(hWnd, &ps);

			// Copy double-buffer to screen
			BitBlt(hdcwnd, r.left, r.top, r.right, r.bottom, hdc, 0, 0, SRCCOPY);

			EndPaint(hWnd, &ps);

			// Destroy double-buffer
			SelectObject(hdc, holdbm);
			DeleteObject(hbm);
			DeleteDC(hdc);

		}
		return 0;
	}

	return CallWindowProc(DefWindowProc, hWnd, uMsg, wParam, lParam);
}

void DrawVisualization(HDC hdc, RECT r, AlphaBlend_func pfnAlphaBlend)
{
	HDC hdcVis;
	HBITMAP hbmVis;
	HBITMAP holdbmVis;
	HPEN hpenVis, holdpenVis;
	BLENDFUNCTION bf={0,0,80,0};
	char *sadata; // Visualization data
	RECT rVis;
	int x;
	HBRUSH hbr;

	static char* (*export_sa_get)(void)=NULL;
	static void (*export_sa_setreq)(int)=NULL;
	

	/* Get function pointers from Winamp */

	if (!export_sa_get)
		export_sa_get = (char* (*)(void))SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETSADATAFUNC);
	if (!export_sa_setreq)
		export_sa_setreq = (void (*)(int))SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_GETSADATAFUNC);


	hdcVis = CreateCompatibleDC(NULL);
	hbmVis = CreateCompatibleBitmap(hdc, 75, 40);
	holdbmVis = (HBITMAP)SelectObject(hdcVis, hbmVis);

	/* Create the pen for the line drawings */
	hpenVis = CreatePen(PS_SOLID, 2, WADlg_getColor(WADLG_ITEMFG));
	holdpenVis = (HPEN)SelectObject(hdcVis, hpenVis);

	/* Specify, that we want both spectrum and oscilloscope data */
	export_sa_setreq(1); /* Pass 0 (zero) and get spectrum data only */
	sadata = export_sa_get();

	/* Clear background */
	hbr = CreateSolidBrush(WADlg_getColor(WADLG_ITEMBG));
	FillRect(hdcVis, &r, hbr);
	DeleteObject(hbr);

	/* Render the oscilloscope */
	if ((config_vismode & NXSBCVM_OSC) == NXSBCVM_OSC) {
		MoveToEx(hdcVis, r.left-1, r.top + 20, NULL);
		for (x = 0; x < 75; x++)
			LineTo(hdcVis, r.left+x, (r.top+20) + sadata[75+x]);
	}

	if ((config_vismode & NXSBCVM_SPEC) == NXSBCVM_SPEC) {
		rVis.top = 0;
		rVis.left = 0;
		rVis.right = 75;
		rVis.bottom = 40;
		DrawAnalyzer(hdcVis, rVis, sadata);
	}

	SelectObject(hdcVis, holdpenVis);
	DeleteObject(hpenVis);

	// Blit to screen
	if (pfnAlphaBlend)
		pfnAlphaBlend(hdc, r.left, r.top, r.right, r.bottom, hdcVis, 0, 0, 75, 40, bf);
	else
		StretchBlt(hdc, r.left, r.top, r.right, r.bottom, hdcVis, 0, 0, 75, 40, SRCCOPY);

	// Destroy vis bitmap/DC
	SelectObject(hdcVis, holdbmVis);
	DeleteObject(hbmVis);
	DeleteDC(hdcVis);

}

void DrawAnalyzer(HDC hdc, RECT r, char *sadata)
{
	static char sapeaks[150];
	static char safalloff[150];
	static char sapeaksdec[150];

	int x;

	for (x = 0; x < 75; x++)
	{
		/* Fix peaks & falloff */
		if (sadata[x] > sapeaks[x])
		{
			sapeaks[x] = sadata[x];
			sapeaksdec[x] = 0;
		}
		else
		{
			sapeaks[x] = sapeaks[x] - 1;

			if (sapeaksdec[x] >= 8)
				sapeaks[x] = (int)(sapeaks[x] - 0.3 * (sapeaksdec[x] - 8));

			if (sapeaks[x] < 0)
				sapeaks[x] = 0;
			else
				sapeaksdec[x] = sapeaksdec[x] + 1;
		}

		if (sadata[x] > safalloff[x]) safalloff[x] = sadata[x];
		else safalloff[x] = safalloff[x] - 2;

		MoveToEx(hdc, r.left+x, r.bottom, NULL);
		LineTo(hdc, r.left+x, r.bottom - safalloff[x]);

		// Draw peaks
		MoveToEx(hdc, r.left+x, r.bottom - safalloff[x], NULL);
		LineTo(hdc, r.left+x, r.bottom - safalloff[x]);

	}
}

LRESULT CALLBACK GenWndSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lres = CallWindowProc(lpGenWndProcOld, hwnd, uMsg, wParam, lParam);

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
			SetMenuItemInfo(WinampMenu, MENUID_BIGCLOCK, FALSE, &mii);
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
				if (id==MENUID_BIGCLOCK) return;
			} while (id != 0xFFFFFFFF);

			// insert menu just before the separator
			InsertMenu(WinampMenu, i-1, MF_BYPOSITION|MF_STRING, MENUID_BIGCLOCK, szAppName);
			break;
		}
	}
}

void RemoveMenuItemFromWinamp()
{
	HMENU WinampMenu;
	WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);
	RemoveMenu(WinampMenu, MENUID_BIGCLOCK, MF_BYCOMMAND);
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
			"Readme file not found!\r\n"
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
		CheckDlgButton(hDlg, IDC_SHADOWCHECK, config_shadowtext?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_SHOWOSCCHECK, (config_vismode&NXSBCVM_OSC)==NXSBCVM_OSC?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_SHOWSPECCHECK, (config_vismode&NXSBCVM_SPEC)==NXSBCVM_SPEC?BST_CHECKED:BST_UNCHECKED);
		CheckDlgButton(hDlg, IDC_SKINCONFIG, config_skinconfig?BST_CHECKED:BST_UNCHECKED);

		if (config_skinconfig) {
			CtrlSkin_SkinControls(hDlg, TRUE);
			CtrlSkin_EmbedWindow(hDlg, TRUE, 1);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			config_shadowtext = IsDlgButtonChecked(hDlg, IDC_SHADOWCHECK)==BST_CHECKED;
			config_skinconfig = IsDlgButtonChecked(hDlg, IDC_SKINCONFIG)==BST_CHECKED;

			if (IsDlgButtonChecked(hDlg, IDC_SHOWOSCCHECK)==BST_CHECKED)
				config_vismode |= NXSBCVM_OSC;
			else
				config_vismode &= ~NXSBCVM_OSC;

			if (IsDlgButtonChecked(hDlg, IDC_SHOWSPECCHECK)==BST_CHECKED)
				config_vismode |= NXSBCVM_SPEC;
			else
				config_vismode &= ~NXSBCVM_SPEC;

			CheckMenuItem(g_hPopupMenu, ID_CONTEXTMENU_SHOWOSC,
				MF_BYCOMMAND|((config_vismode&NXSBCVM_OSC)==NXSBCVM_OSC?MF_CHECKED:MF_UNCHECKED));

			CheckMenuItem(g_hPopupMenu, ID_CONTEXTMENU_SHOWSPEC,
				MF_BYCOMMAND|((config_vismode&NXSBCVM_SPEC)==NXSBCVM_SPEC?MF_CHECKED:MF_UNCHECKED));


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
	INI_READ_INT(config_show);
	INI_READ_INT(config_x);
	INI_READ_INT(config_y);
	INI_READ_INT(config_width);
	INI_READ_INT(config_height);
	INI_READ_INT(config_shadowtext);
	INI_READ_INT(config_vismode);
	INI_READ_INT(config_displaymode);
	INI_READ_INT(config_skinconfig);
	INI_READ_INT(config_showms);
}

void config_write()
{
	TCHAR ini_file[MAX_PATH];
	if (!GetPluginINIPath(ini_file)) return;

	INI_WRITE_INT(config_enabled);
	INI_WRITE_INT(config_show);
	INI_WRITE_INT(config_x);
	INI_WRITE_INT(config_y);
	INI_WRITE_INT(config_width);
	INI_WRITE_INT(config_height);
	INI_WRITE_INT(config_shadowtext);
	INI_WRITE_INT(config_vismode);
	INI_WRITE_INT(config_displaymode);
	INI_WRITE_INT(config_skinconfig);
	INI_WRITE_INT(config_showms);

	WritePrivateProfileString(NULL, NULL, NULL, ini_file);
}



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
