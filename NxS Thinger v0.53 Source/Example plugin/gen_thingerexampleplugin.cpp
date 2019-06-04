/* gen_thingerexampleplugin.cpp
** Example plugin using the NxS Thinger control.
** Written by Saivert
**/

#include "stdafx.h"
#include "resource.h"
#include "gen.h"
#include "wa_ipc.h"
#include "wa_hotkeys.h"
#include "wa_msgids.h"
//#include "ipc_pe.h"

/* Important: Include the NxS Thinger API defines and typedefs */
#include "NxSThingerAPI.h"

/* P L U G I N   D E F I N E S */
#define PLUGIN_TITLE "Funky example plugin v0.0"
#define PLUGIN_TITLE_NOTLOAD PLUGIN_TITLE" <Error: Requires Winamp 5.4+>"
/* This is what is displayed as title in Message boxes */
#define PLUGIN_CAPTION "Funky example plugin"

/* Your menu identifier for the custom menu item in Winamp's main menu */
#define MENUID_MYITEM (46184)

/* F U N C T I O N   P R O T O T Y P E S */
LRESULT CALLBACK WinampSubclass(HWND, UINT, WPARAM, LPARAM);
void config(void);
void quit(void);
int init(void);
BOOL CALLBACK DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// G L O B A L   V A R I A B L E S
static int g_waver;
static WNDPROC lpOldWinampWndProc; /* Important: Old window procedure pointer */
int g_xPos;
int g_yPos;

int g_iThingerIPC=0;
BOOL g_fHasAddedIconToThinger=FALSE;
UINT g_iMyThingerIconId=0;

// Plug-in structure
winampGeneralPurposePlugin plugin = {
	GPPHDR_VER,
	PLUGIN_TITLE,
	init,
	config,
	quit,
};

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	if (ul_reason_for_call==DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls((HMODULE)hModule);
	}
	return TRUE;
}

// Most important!!
extern "C" __declspec(dllexport) winampGeneralPurposePlugin* winampGetGeneralPurposePlugin()
{
	return &plugin;
}


int init()
{

	g_waver = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETVERSION);

	if (g_waver < 0x5001)
	{
		plugin.description = PLUGIN_TITLE_NOTLOAD;
		return 0;
	}

	HMENU WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);

	MENUITEMINFO mii;
	mii.cbSize		= sizeof(MENUITEMINFO);
	mii.fMask		= MIIM_TYPE|MIIM_ID;
	mii.dwTypeData	= PLUGIN_CAPTION;
	mii.fType		= MFT_STRING;
	mii.wID			= MENUID_MYITEM;
	
	InsertMenuItem(WinampMenu, WINAMP_OPTIONS_VIDEO, FALSE, &mii);
	/* Always remember to adjust "Option" submenu position. */
	/* Note: In Winamp 5.0+ this is unneccesary as it is more intelligent when
	   it comes to menus, but you must do it so it works with older versions. */
	SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_ADJUST_OPTIONSMENUPOS);


	/* Subclass Winamp's main window */
	lpOldWinampWndProc = WNDPROC(SetWindowLongPtr(plugin.hwndParent, GWLP_WNDPROC, int(WinampSubclass)));

	PostMessage(plugin.hwndParent, WM_WA_IPC, 666, IPC_GETVERSION);
	
	return 0; //success
}

void quit()
{

}

void config()
{
	MessageBox(plugin.hwndParent,
		TEXT(PLUGIN_TITLE "\r\nThis plug-in has nothing to configure!"),
		NULL, MB_ICONWARNING);
}

BOOL CALLBACK DlgProc(HWND hwndDlg,UINT uMsg,WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetWindowPos(hwndDlg, 0, g_xPos, g_yPos, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam)==IDOK || LOWORD(wParam)==IDCANCEL)
			DestroyWindow(hwndDlg);
		if (LOWORD(wParam)==IDC_SHOW) {
			NxSThingerIconStruct ntis;
			ntis.dwFlags = NTIS_MODIFY | NTIS_NOICON | NTIS_NOMSG | NTIS_NODESC;
			ntis.uIconId = g_iMyThingerIconId;

			/* Modify the icon */
			SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&ntis, g_iThingerIPC);
		}
		if (LOWORD(wParam)==IDC_HIDE) {
			NxSThingerIconStruct ntis;
			ntis.dwFlags = NTIS_MODIFY | NTIS_NOICON | NTIS_NOMSG | NTIS_NODESC | NTIS_HIDDEN;
			ntis.uIconId = g_iMyThingerIconId;

			/* Modify the icon */
			SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&ntis, g_iThingerIPC);
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hwndDlg);
		break;
	case WM_MOVE:
		/* Remember last position */
		{
			RECT r;
			GetWindowRect(hwndDlg, &r);
			g_xPos = r.left;
			g_yPos = r.top;
		}
		break;
	}

	return FALSE;
}

void OpenMyDialog()
{
	static HWND hwndCfg=NULL;
	MSG msg;
	HMENU WinampMenu;

	WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);

	if (IsWindow(hwndCfg)) {

		/*  Toggle dialog. If it's created, destroy it. */
		DestroyWindow(hwndCfg);
		CheckMenuItem(WinampMenu, MENUID_MYITEM, MF_BYCOMMAND|MF_UNCHECKED);
		
		/* Code for reshowing it if it's already created.
		ShowWindow(hwndCfg, SW_SHOW);
		SetForegroundWindow(hwndCfg);
		*/
		return;
	}

	hwndCfg = CreateDialog(plugin.hDllInstance, MAKEINTRESOURCE(IDD_MYDIALOG), 0, DlgProc);

	ShowWindow(hwndCfg, SW_SHOW);
	CheckMenuItem(WinampMenu, MENUID_MYITEM, MF_BYCOMMAND|MF_CHECKED);

	while (IsWindow(hwndCfg) && IsWindow(plugin.hwndParent) && GetMessage(&msg, 0, 0, 0)) {
		if (!IsDialogMessage(hwndCfg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	hwndCfg = NULL;

	

	/* Modal dialog box code
	if (DialogBox(plugin.hDllInstance, MAKEINTRESOURCE(IDD_MYDIALOG), hwnd, ConfigDlgProc)) {
		ShowWindow(ews->me, config_enabled?SW_SHOW:SW_HIDE);
	}
	*/

}

LRESULT CALLBACK WinampSubclass(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	
	switch (message)
	{
	case WM_WA_IPC:
		if (wParam == 666 && lParam == IPC_GETVERSION) {
			/* Add icon to NxS Thinger here */
			if (!g_fHasAddedIconToThinger) {
				NxSThingerIconStruct ntis;
				ntis.dwFlags = NTIS_ADD | NTIS_BITMAP;
				ntis.lpszDesc = PLUGIN_CAPTION;
				ntis.hBitmap = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_TEST));
				ntis.hBitmapHighlight = LoadBitmap(plugin.hDllInstance, MAKEINTRESOURCE(IDB_TEST_H));
				ntis.hWnd = 0; /* <-- Same as plugin.hwndParent */
				ntis.uMsg = WM_COMMAND;
				ntis.wParam = MAKEWPARAM(MENUID_MYITEM, 0);
				ntis.lParam = 0;

				/* Get the message number. */
				g_iThingerIPC = SendMessage(plugin.hwndParent, WM_WA_IPC,
					(WPARAM)&"NxSThingerIPCMsg", IPC_REGISTER_WINAMP_IPCMESSAGE);

				/* Add the icon to NxS Thinger control */
				g_iMyThingerIconId = SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&ntis, g_iThingerIPC);

				if (g_iMyThingerIconId)	
					g_fHasAddedIconToThinger = TRUE;
			}

		}
		break;
	case WM_COMMAND:
		if (LOWORD(wParam)==MENUID_MYITEM && HIWORD(wParam) == 0)
		{
			OpenMyDialog();
			return 0;
		}
		break;
	/* Also handle WM_SYSCOMMAND if people are selectng the item through
	   the Winamp submenu in system menu. */
	case WM_SYSCOMMAND:
		if (wParam == MENUID_MYITEM)
		{
			OpenMyDialog();
			return 0;
		}
		break;
	}


	/* Call previous window procedure */
	return CallWindowProc((WNDPROC)lpOldWinampWndProc,hwnd,message,wParam,lParam);
}



//#EOF