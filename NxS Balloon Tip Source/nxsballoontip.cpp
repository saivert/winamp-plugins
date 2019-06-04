/* nxsballoontip.cpp : NxS Balloon Tip plug-in for Winamp 2.9x
 Displays a balloon style tool tip near the system tray, when
 a track change in Winamp is discovered.
 
 This version is written with with "AggressiveOptimize.h"
 it produces a very small DLL file because...just read it!

 This plugin uses the new IPC_GEN_HOTKEYS_ADD registered IPC message
 to add a new item to the Global Hotkeys list.
 If you don't have that plugin (it's bundled with WInamp 5), well
 then this plugin is missing Hotkey support.

 Written by Saivert
   http://inthegray.com/saivert/
   saivert@gmail.com
*/

#include "stdafx.h"
#include "shellapi.h"

#include "gen.h"
#include "resource.h"

#include "NxSTrayIcon.h"
#include "nxsweblink.h"
#include "NxSToolTip.h"
#include "nxstabpages.h"

#include "misc.h"

#define PLUGIN_TITLE         "NxS Balloon Tip Notification v2.8"
#define PLUGIN_PAGETITLE     "Balloon Tip"
#define PLUGIN_TITLE_NOTLOAD PLUGIN_TITLE" <Error: Requires Winamp 5.1+>"
#define PLUGIN_CAPTION       "NxS Balloon Tip"
#define PLUGIN_INISECTION    "NxS Balloon Tip"
#define PLUGIN_READMEFILE    "gen_nxsballoontip.html"
#define PLUGIN_HOMEPAGE      "http://inthegray.com/saivert/"
#define PLUGIN_FORUMS        "http://forums.winamp.com/"
const int PLUGIN_DEFTIMEOUT = 5000;

/* Note: This values were chosen by random. */
const int TIMERID_HIDEBALLOON = 23985;

/* Don't steal this... Get your own! */
const int NXSBALLOONTIP_TRAYICONID = 123;

/* This GUID is used as a unique id for the tray icon */
/* {C038A10C-AC8F-41aa-9CEA-0E22E5B6BE3A} */
static const GUID g_trayGuid =
	{ 0xc038a10c, 0xac8f, 0x41aa, { 0x9c, 0xea, 0xe, 0x22, 0xe5, 0xb6, 0xbe, 0x3a } };

namespace BalloonMsg {
	typedef enum _BalloonMsg {
		HIDE = 0,
		SHOW = 1,
		COPYTEXT = 2,
		REGHOTKEYS = 3,
		SHOW_UPDATEMSNONLY = 4,
	} BalloonMsg;
}



/* G L O B A L   V A R I A B L E S */

/* Configuration variables and their default values */
int config_enabled=TRUE;
int config_usewatray=FALSE;
unsigned int config_timeout=PLUGIN_DEFTIMEOUT;
unsigned int config_iconidx=6;
int config_curpageindex=0;
int config_displayonpbchange=FALSE;
int config_displayonvolchange=FALSE;
int config_useml=TRUE;
int config_displayontitlechanges=FALSE;
int config_updatemsn=TRUE;

/* Other global variables */
int g_volchanged=0;       /* Used here and in misc.cpp */
char *szAdvFormatStr;
char *szBalloonTitle;
char *szClipboardTitle;
char *szOutBuf;           /* A buffer to write formatted text to. */
UINT g_uiRegTrayMsg;      /* Registered Window Message is put here */
LONG g_BalloonIPCMsg;     /* Registered custom WM_WA_IPC message. */
int g_waver;              /* This will be set to Winamp's version number at init */
HWND g_hwndPE=0;          /* Set with handle to playlist editor window */
HWND g_hwndML=0;          /* Set with handle to media library window */
CNxSTrayIcon *pNxSTray;   /* Object that manages tray icons. */
int g_oldplpos;           /* Keeps the previous (old) playlist position. */
bool g_ShowingBalloon;    /* This variable is true if we are showing the balloon. */

/* Two variables to keep the ID and HWND of Winamp's own tray icon. */
static HWND g_twnd;
static UINT g_tid;


/* Array of resource id's for Winamp.exe's icons.
   These icons will be loaded into the hWAIcons array. */
static const int aiWAIcons[] = {102,131,162,163,164,167,169,178,179,180,181,182};
static HICON hWAIcons[sizeof(aiWAIcons)/sizeof(aiWAIcons[0])];


namespace NxSBalloonTipAPI {

	/* External API support */
	const int NBTSI_NONE       = 0x00000000;
	/* icon flags are mutually exclusive
	   and take only the lowest 2 bits */
	const int NBTSI_INFO       = 0x00000001;
	const int NBTSI_WARNING    = 0x00000002;
	const int NBTSI_ERROR      = 0x00000003;

	/* NxSBalloonTipStruct */
	typedef struct _Struct {
	  char szTitle[1024];
	  char szTip[1024];
	  int iIcon;
	  int future_expansion[16]; /* 64 bytes of space for future expansion */
	} Struct, * LPStruct;

	LONG uMessage; /* Registered "NxSBalloonTip_Show" message */

}


/* Plug-in structure */
winampGeneralPurposePlugin plugin;

/* Structure used to add a custom page to Winamp's preferences dialog */
prefsDlgRec prefsrec;

/* Old window procedure pointers */
WNDPROC lpOldWinampWndProc;
WNDPROC lpOldPEWndProc;

/* F U N C T I O N   P R O T O T Y P E S */
LRESULT CALLBACK PESubclass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WinampSubclass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TrayWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK ConfigProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK GeneralConfigPageProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK TitleFormatConfigPageProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AboutProc(HWND, UINT, WPARAM, LPARAM);

void config(void);
void quit(void);
int init(void);
void config_write(void);
void config_read(void);


char *GetResStr(int id)
{
	static char str[256];
	if (LoadString(plugin.hDllInstance, id, str, sizeof(str)/sizeof(str[0])))
		return str;
	else
		return NULL;
}


void config() {
	if (g_waver >= 0x5001) {
		SendMessage(plugin.hwndParent, WM_WA_IPC, prefsrec._id, IPC_OPENPREFSTOPAGE);
	} else {
		MessageBox(plugin.hwndParent,
			TEXT(PLUGIN_TITLE "\r\n"
			"This plug-in requires a version of Winamp greater than or equal to 5.1!"),
			PLUGIN_CAPTION, MB_ICONWARNING);
	}
}

int init() {

	g_waver = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETVERSION);

	if (g_waver < 0x5001) {
		plugin.description = PLUGIN_TITLE_NOTLOAD;
		return 0;
	}

	szAdvFormatStr	= (char*)HeapAlloc(GetProcessHeap(), 0, 1024);
	szBalloonTitle	= (char*)HeapAlloc(GetProcessHeap(), 0, 1024);
	szClipboardTitle = (char*)HeapAlloc(GetProcessHeap(), 0, 1024);

	szOutBuf = (char*)GlobalAlloc(GPTR, 1024);
	config_read();

	RegisterNxSWebLink(plugin.hDllInstance);

	/* Register own IPC message (private use) */
	g_BalloonIPCMsg = SendMessage(plugin.hwndParent, WM_WA_IPC,
		(WPARAM)"NxSBalloonTipIPC", IPC_REGISTER_WINAMP_IPCMESSAGE);

	/* Register own IPC message for Show Balloon Tip API */
	NxSBalloonTipAPI::uMessage = SendMessage(plugin.hwndParent, WM_WA_IPC,
		(WPARAM)"NxSBalloonTip_Show", IPC_REGISTER_WINAMP_IPCMESSAGE);

	/* Add preferences page */
	prefsrec.hInst = plugin.hDllInstance;
	prefsrec.dlgID = IDD_CONFIGPAGE;
	prefsrec.where = 0;
	prefsrec.name = PLUGIN_PAGETITLE;
	prefsrec.proc = ConfigProc;
	SendMessage(plugin.hwndParent, WM_WA_IPC, (int)&prefsrec, IPC_ADD_PREFS_DLG);

	/* Subclass Winamp's main window */
	lpOldWinampWndProc = (WNDPROC)SetWindowLongPtr(plugin.hwndParent, GWLP_WNDPROC,
		(LONG) WinampSubclass);

	/* Load icons from winamp.exe */
	for (int x=0;x<sizeof(aiWAIcons)/sizeof(aiWAIcons[0]);x++) {
		hWAIcons[x] = (HICON)LoadImage(GetModuleHandle(NULL), (LPCTSTR)aiWAIcons[x], IMAGE_ICON, 16, 16, 0);
	}


	/* Get a unique message value for use with plug-ins tray icon
	   (not Winamp's own or Agent) */
	g_uiRegTrayMsg = RegisterWindowMessage("NxSBalloonTipTrayIconMsg");
	/* Construct and initialize the tray icon class object */
	pNxSTray = new CNxSTrayIcon;
	pNxSTray->SetCallbackMsg(g_uiRegTrayMsg);
	pNxSTray->SetWnd(plugin.hwndParent);
	pNxSTray->SetID(NXSBALLOONTIP_TRAYICONID);
	pNxSTray->SetGUID(g_trayGuid);


	/* Subclass playlist editor window */
	g_hwndPE = (HWND)SendMessage(plugin.hwndParent, WM_WA_IPC, IPC_GETWND_PE, IPC_GETWND);
	lpOldPEWndProc = (WNDPROC)SetWindowLongPtr(g_hwndPE, GWLP_WNDPROC, (LONG) PESubclass);


	/* Post a message to notify our subclass that we can register hotkeys with gen_hotkeys.dll.
	   Winamp (and it's subclasses) will not handle messages until all plugins are loaded.
	   I'm using this method after a discussion with ShaneH at the Winamp forums. */
	PostMessage(plugin.hwndParent, WM_WA_IPC, BalloonMsg::REGHOTKEYS, g_BalloonIPCMsg);


	/* Experimental - using the Media Library */
	long libhwndipc = (LONG)SendMessage(plugin.hwndParent, WM_WA_IPC,
		WPARAM("LibraryGetWnd"), IPC_REGISTER_WINAMP_IPCMESSAGE);
	g_hwndML = (HWND)SendMessage(plugin.hwndParent, WM_WA_IPC, -1, LPARAM(libhwndipc));
	/* Disable use of Media Library if Media Library is not available */
	if (!g_hwndML) config_useml=0;

	return 0; /* success */
}

void quit()
{
    delete pNxSTray;

	/* Remove subclass of Playlist Editor window */
	if (g_hwndPE && GetWindowLongPtr(g_hwndPE, GWLP_WNDPROC)==(LONG) PESubclass)
		SetWindowLongPtr(g_hwndPE, GWLP_WNDPROC, (LONG) lpOldPEWndProc);

	/* Remove subclass of Winamp's main window */
	if (GetWindowLongPtr(plugin.hwndParent, GWLP_WNDPROC)==(LONG) WinampSubclass)
		SetWindowLongPtr(plugin.hwndParent, GWLP_WNDPROC, (LONG) lpOldWinampWndProc);

	config_write();
	config_enabled=FALSE;

	HeapFree(GetProcessHeap(), 0, szAdvFormatStr);
	HeapFree(GetProcessHeap(), 0, szBalloonTitle);
	HeapFree(GetProcessHeap(), 0, szClipboardTitle);

	GlobalFree((HGLOBAL)szOutBuf);
}


bool IsSameSong(void) {
	static char szOldSong[256]="";

	LPTSTR file;
	int iPos;
	bool same;

	iPos = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS);
	file = (char*)SendMessage(plugin.hwndParent, WM_WA_IPC, iPos, IPC_GETPLAYLISTTITLE);

	same = (lstrcmp(file, szOldSong)==0);
	if (!same) lstrcpy(szOldSong, file);
	return same;
}

bool IsHTTPStream(void) {
	LPTSTR file;
	int iPos;
	iPos = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS);
	file = (char*)SendMessage(plugin.hwndParent, WM_WA_IPC, iPos, IPC_GETPLAYLISTFILE);
	return (strstr(file, "http://") > 0);
}


bool ShowBalloonTip(NxSBalloonTipAPI::LPStruct params) {

	DWORD dwFlags=NIIF_NOSOUND;

	if (params->iIcon == NxSBalloonTipAPI::NBTSI_NONE) dwFlags |= NIIF_NONE;
	if (params->iIcon == NxSBalloonTipAPI::NBTSI_INFO) dwFlags |= NIIF_INFO;
	if (params->iIcon == NxSBalloonTipAPI::NBTSI_WARNING) dwFlags |= NIIF_WARNING;
	if (params->iIcon == NxSBalloonTipAPI::NBTSI_ERROR) dwFlags |= NIIF_ERROR;

	/* Display the balloon tip, either using our own tray icon
	   or one of Winamp's (this be Agent or built-in). */
	if (!config_usewatray) {
		/* Put up our own tray icon */
		pNxSTray->SetIcon(hWAIcons[config_iconidx]);
		pNxSTray->Show();

		pNxSTray->SetTooltip(params->szTip);

		pNxSTray->ShowBalloon(params->szTip, params->szTitle, dwFlags);
	} else {
		int tb;
		TCHAR ini_file[MAX_PATH];
		CNxSTrayIcon tmptray;

		//Causes lockups: SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_WRITECONFIG);

		lstrcpyn(ini_file, (char*)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETINIFILE), MAX_PATH);

		tb=GetPrivateProfileInt("Winamp", "taskbar", 0, ini_file);
		if (tb==1 || tb==2) {
			/* Throw up a balloon on behalf of Winamp's tray icon */
			g_twnd = plugin.hwndParent;
			g_tid = 502;
			tmptray.Attach(g_twnd, g_tid);
			tmptray.ShowBalloon(params->szTip, params->szTitle,	dwFlags);
		} else {
			/* Winamp is configured to not show a tray icon,
			   try the agent instead. */
			g_twnd = FindWindow("WinampAgentMain", NULL);
			g_tid = 1024;
			tmptray.Attach(g_twnd, g_tid);
			tmptray.ShowBalloon(params->szTip, params->szTitle,	dwFlags);
		}
		tmptray.Detach();
	}/* if (!config_usewatray)... */


	g_volchanged = 0; /* Reset this!! */
	
	/* Since the shell does not allow timeout values smaller than 10 secs
	   for the balloon tip notification feature, we need to assign a timer
	   and hide the balloon ourself. */
	SetTimer(plugin.hwndParent, TIMERID_HIDEBALLOON, config_timeout, NULL);

	return true;
}


LRESULT CALLBACK PESubclass(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	/* Winamp sends a WM_USER with wParam==666 and lParam==playlist index
	   to the playlist editor window when the song changes.
	   I'm now using this method to catch song changes after I had a discussion
	   with Shane Hird and DrO on the Winamp forums. */    
	static unsigned int bFlag=0;

	if ((message == WM_USER) && (wParam == 666) && ((lParam & 0x40000000) == 0x40000000)) {
		if (GetTickCount() > bFlag) {
			if ( !IsHTTPStream() || (IsHTTPStream() && config_displayontitlechanges && !IsSameSong()) ) {
				//PostMessage(plugin.hwndParent, WM_WA_IPC, BalloonMsg::SHOW, g_BalloonIPCMsg);
				PostMessage(plugin.hwndParent, WM_WA_IPC, BalloonMsg::SHOW, g_BalloonIPCMsg);
			}
		} else {
			bFlag = GetTickCount()+1000;
		}
	}
	return CallWindowProc((WNDPROC)lpOldPEWndProc,hwnd,message,wParam,lParam);
}

void SendTextToMSNMessenger(int active, char *fmt, char* artist, char* title, char* album) {
	COPYDATASTRUCT msndata;
	WCHAR wszBuf[512];
	CHAR szBuf[512];
	HWND msnui=NULL;

	wsprintf(szBuf, "Winamp\\0Music\\0%d\\0%s\\0%s\\0%s\\0%s\\0%s\\0",
		active, fmt, artist, title, album, "WMContentID");
	MultiByteToWideChar(CP_ACP, 0, szBuf, -1, wszBuf, sizeof(wszBuf)/sizeof(wszBuf[0]));
	msndata.dwData = 0x547;
	msndata.cbData = (lstrlenW(wszBuf)*2)+2;
	msndata.lpData = &wszBuf;
	while (msnui = FindWindowEx(NULL, msnui, "MsnMsgrUIManager", NULL))
	{
		SendMessage(msnui, WM_COPYDATA, (WPARAM)0, (LPARAM)&msndata);
	}
}

LRESULT CALLBACK WinampSubclass(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	if (!config_enabled) {
		return CallWindowProc((WNDPROC)lpOldWinampWndProc,hwnd,message,wParam,lParam);
	}

	/* In case Winamp is terminated, we must remove the balloon from
	   either the Agent's or Winamp's own tray icon. */
	if (message==WM_DESTROY) {
		CNxSTrayIcon tmptray;
		tmptray.Attach(g_twnd, g_tid);
		tmptray.HideBalloon();
		tmptray.Detach();
	}

	if (message==WM_WA_SYSTRAY && wParam==g_tid) {
		if (lParam==NIN_BALLOONUSERCLICK && !(GetKeyState(VK_CONTROL)&0x1000)) {
			if (GetKeyState(VK_SHIFT)&0x1000)
				SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_BUTTON1/*Prev button*/, 0);
			else
				SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_BUTTON5/*Next button*/, 0);
		}
		if (lParam==NIN_BALLOONHIDE && g_ShowingBalloon)
			PostMessage(plugin.hwndParent, WM_WA_IPC, BalloonMsg::SHOW, g_BalloonIPCMsg);
	}

	/* Check for messages regarding our own tray icon
	   (if we dont use one we wont get this message either) */
	if (message==g_uiRegTrayMsg && wParam==NXSBALLOONTIP_TRAYICONID) {
		if (lParam==NIN_BALLOONUSERCLICK && !(GetKeyState(VK_CONTROL)&0x1000)) {
			if (GetKeyState(VK_SHIFT)&0x1000)
				SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_BUTTON1/*Prev button*/, 0);
			else
				SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_BUTTON5/*Next button*/, 0);
			pNxSTray->Hide();
		}
		if (lParam==WM_LBUTTONUP || lParam==WM_RBUTTONUP)
			pNxSTray->Hide();
		if (lParam==NIN_BALLOONHIDE && g_ShowingBalloon)
			PostMessage(plugin.hwndParent, WM_WA_IPC, BalloonMsg::SHOW, g_BalloonIPCMsg);
	}

	/* Handle callback messages from Winamp */
	if (message==WM_WA_IPC && lParam==IPC_CB_MISC) {
		switch(wParam) {
		case IPC_CB_MISC_VOLUME:
			if (config_displayonvolchange) {
				g_volchanged = 1;
				PostMessage(plugin.hwndParent, WM_WA_IPC, BalloonMsg::SHOW, g_BalloonIPCMsg);
			}
			break;
		case IPC_CB_MISC_STATUS:
			if (config_displayonpbchange)
				PostMessage(plugin.hwndParent, WM_WA_IPC, BalloonMsg::SHOW, g_BalloonIPCMsg);
			if (config_updatemsn && (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_ISPLAYING) != 1) )
				SendTextToMSNMessenger(0, "{0}", "", "", "");
			if (config_updatemsn && (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_ISPLAYING) == 1) )
				PostMessage(plugin.hwndParent, WM_WA_IPC, BalloonMsg::SHOW_UPDATEMSNONLY, g_BalloonIPCMsg);
			break;
#if 0
		case IPC_CB_MISC_TITLE:
			if (config_displayontitlechanges) {
			}
			break;
#endif
		}
		return CallWindowProc((WNDPROC)lpOldWinampWndProc,hwnd,message,wParam,lParam);
	}

	/* Remove balloon tip when we get the timeout */
	if (message==WM_TIMER && wParam==TIMERID_HIDEBALLOON) {
		PostMessage(plugin.hwndParent, WM_WA_IPC, BalloonMsg::HIDE, g_BalloonIPCMsg);
		KillTimer(hwnd, TIMERID_HIDEBALLOON);
	}


	/* Handle our custom API message */
	if (message==WM_WA_IPC && lParam==NxSBalloonTipAPI::uMessage) {
		ShowBalloonTip(NxSBalloonTipAPI::LPStruct(wParam));
	}

	/* Handle our custom IPC message here (g_BalloonIPCMsg) */
	if (message==WM_WA_IPC && lParam==g_BalloonIPCMsg) {
		switch (wParam)	{
		case BalloonMsg::HIDE:
			{
				g_ShowingBalloon = false;
				/* Remove tray icon */
				if (!config_usewatray) {
					pNxSTray->Hide();
				} else {
					CNxSTrayIcon tmptray;
					tmptray.Attach(g_twnd, g_tid);
					tmptray.HideBalloon();
					tmptray.Detach();
				}
			}
			break;
		case BalloonMsg::SHOW:
		case BalloonMsg::SHOW_UPDATEMSNONLY:
			{
				g_ShowingBalloon = true;

				/* Create the tip string */
				waFormatTitle ft;
				char szTmp[1024];
				NxSBalloonTipAPI::Struct msg;

				/* ZeroMemory(&ft, sizeof(waFormatTitle)); */
				ft.TAGFUNC = myTagFunc;
				ft.TAGFREEFUNC = myTagFreeFunc;
				ft.out = szTmp;
				ft.out_len = sizeof(szTmp);
				ft.p = plugin.hwndParent;

				if (wParam == BalloonMsg::SHOW) {

				ft.spec = szAdvFormatStr;
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&ft, IPC_FORMAT_TITLE);
				parse_escapes(szTmp, msg.szTip); /* expand "\r", "\n", "\t" ... */

				/* Create title for balloon */
				ft.spec = szBalloonTitle;
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&ft, IPC_FORMAT_TITLE);
				parse_escapes(szTmp, msg.szTitle);

				}

				/* MSN Messenger Status updating */
				if (config_updatemsn) {
					char *infos[3];
					if (!IsHTTPStream()) {
						infos[0] = (char*)myTagFunc("artist", plugin.hwndParent);
						infos[1] = (char*)myTagFunc("title", plugin.hwndParent);
						infos[2] = (char*)myTagFunc("album", plugin.hwndParent);
					} else {
						ft.spec = "%watitle%";
						SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&ft, IPC_FORMAT_TITLE);
					}

					if (IsHTTPStream()) {
						SendTextToMSNMessenger(1, "Internet Radio: {0}", szTmp, "", "");
					} else {
						SendTextToMSNMessenger(1, "{0} - {1}", infos[0], infos[1], infos[2]);
					}

					for (int i=0; i<sizeof(infos)/sizeof(infos[0]); i++)
						myTagFreeFunc(infos[i], plugin.hwndParent);
				}

				/* Call code to actually display the balloon a.k.a use our own API */
				if (wParam == BalloonMsg::SHOW) {
					msg.iIcon = NxSBalloonTipAPI::NBTSI_INFO;
					ShowBalloonTip(&msg);
				}
			}
			break;
		case BalloonMsg::COPYTEXT:
			{
				/* Create the tip string */
				waFormatTitle ft;
				char szTmp[1024];

				ft.TAGFUNC = myTagFunc;
				ft.TAGFREEFUNC = myTagFreeFunc;
				ft.out = szTmp;
				ft.out_len = sizeof(szTmp);
				ft.p = plugin.hwndParent;
				ft.spec = szClipboardTitle;
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&ft, IPC_FORMAT_TITLE);
				parse_escapes(szTmp, szOutBuf); /* expand "\r", "\n", "\t" ... */

				return (LRESULT)szOutBuf;
			}
			break;
		case BalloonMsg::REGHOTKEYS:
			{
				UINT genhotkeys_add_ipc;
				genHotkeysAddStruct genhotkey;

				/* Get message value */
				genhotkeys_add_ipc = SendMessage(plugin.hwndParent, WM_WA_IPC,
					(WPARAM)"GenHotkeysAdd", IPC_REGISTER_WINAMP_IPCMESSAGE);

				/* Set up the genHotkeysAddStruct */
				genhotkey.name = "NxS Balloon Tip: Show balloon notification";
				genhotkey.flags = HKF_NOSENDMSG;
				genhotkey.id = "NxSBalloonTipPluginMsg";
				genhotkey.uMsg = WM_WA_IPC;
				genhotkey.wParam = BalloonMsg::SHOW;
				genhotkey.lParam = g_BalloonIPCMsg;
				genhotkey.wnd = 0; // Use Winamp's main window
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&genhotkey, genhotkeys_add_ipc);

				genhotkey.name = "NxS Balloon Tip: Copy balloon text to clipboard";
				genhotkey.flags = HKF_COPY;
				genhotkey.id = "NxSBalloonTipPluginCopyToClipboard";
				genhotkey.uMsg = WM_WA_IPC;
				genhotkey.wParam = BalloonMsg::COPYTEXT;
				genhotkey.lParam = g_BalloonIPCMsg;
				genhotkey.wnd = 0; // Use Winamp's main window
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&genhotkey, genhotkeys_add_ipc);
			}
			break;
		default: return CallWindowProc((WNDPROC)lpOldWinampWndProc,hwnd,message,wParam,lParam);
		} /*switch (lParam)...*/
		
	} /*if (message...*/


	return CallWindowProc((WNDPROC)lpOldWinampWndProc,hwnd,message,wParam,lParam);
}



INT_PTR CALLBACK ConfigProc(HWND hwndDlg,UINT uMsg,WPARAM wParam, LPARAM lParam)
{
	static CNxSTabPages *tp;

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			HWND h;
			RECT r;

			/* Adjust dialogbox and the tab control so it fits in the
			   preferences dialog. It just looks better that way! */
			h = GetNextWindow(hwndDlg, GW_HWNDPREV); // Get handle of "placeholder" static
			GetClientRect(h, &r);
			MoveWindow(hwndDlg, 0, 0, r.right, r.bottom, TRUE);
			MoveWindow(GetDlgItem(hwndDlg, IDC_TAB1), 0, 0, r.right, r.bottom, TRUE);

			tp = new CNxSTabPages(GetDlgItem(hwndDlg, IDC_TAB1), plugin.hDllInstance);
			tp->AddPage(NULL, MAKEINTRESOURCE(IDD_GENERAL), GeneralConfigPageProc, NULL);
			tp->AddPage(NULL, MAKEINTRESOURCE(IDD_TITLEFORMAT), TitleFormatConfigPageProc, NULL);
			tp->AddPage(NULL, MAKEINTRESOURCE(IDD_ABOUT), AboutProc, NULL);
			tp->SelectPage(config_curpageindex);

		}
		break;
	case WM_NOTIFY:
		{
			LRESULT lRes = tp->HandleNotifications(wParam, lParam);
			if (lRes) {
				config_curpageindex = tp->GetSelPage();
				SetWindowLong(hwndDlg, DWL_MSGRESULT, lRes);
				return TRUE;
			}
		}
		break;
	case WM_DESTROY:
		if (tp) delete tp;
		break;
	}

	return FALSE;
}

INT_PTR CALLBACK GeneralConfigPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static CNxSToolTip *tipobj;
	static HWND hwndIconSpin;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			RECT r;
			char s[10];
			HWND hwndTrack;
			int timeout_in_secs;
			
			if (!config_timeout) {
				config_timeout = PLUGIN_DEFTIMEOUT;
			}

			timeout_in_secs = config_timeout/1000;
			
			/* Initialize controls */
			CheckDlgButton(hwndDlg, IDC_ENABLEDCB, config_enabled?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_USEWATRAYCB, config_usewatray?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_VOLCHANGES, config_displayonvolchange?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_PBSTATECHANGES, config_displayonpbchange?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_USEMLCB, config_useml?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_DISPLAYONTITLECHANGES, config_displayontitlechanges?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_UPDATEMSNCHECK, config_updatemsn?BST_CHECKED:BST_UNCHECKED);
			
			

			EnableWindow(GetDlgItem(hwndDlg, IDC_USEMLCB), g_hwndML>0);

			hwndTrack = GetDlgItem(hwndDlg, IDC_TBTIMEOUT);
			SendMessage(hwndTrack, TBM_SETRANGE, (WPARAM) TRUE, (LPARAM) MAKELONG(1, 30));
			SendMessage(hwndTrack, TBM_SETPAGESIZE, 0, (LPARAM) 5);
			SendMessage(hwndTrack, TBM_SETLINESIZE,	0, (LPARAM) 1);
			SendMessage(hwndTrack, TBM_SETTICFREQ, (LPARAM) 5, 0);
			SendMessage(hwndTrack, TBM_SETPOS, (WPARAM) TRUE, (LPARAM) timeout_in_secs);

			wsprintf(s, "Timeout: %d sec.", timeout_in_secs);
			SetDlgItemText(hwndDlg, IDC_GBTIMEOUT, s);
			
			if (config_iconidx<0 || config_iconidx>(sizeof(aiWAIcons)/sizeof(aiWAIcons[0])))
				config_iconidx = 0;
			
			SendDlgItemMessage(hwndDlg, IDC_ICONCTL, STM_SETIMAGE,
				IMAGE_ICON, (WPARAM)hWAIcons[config_iconidx]);
			
			/* Create Up/Down control */
			GetWindowRect(GetDlgItem(hwndDlg, IDC_ICONCTL), &r);
			MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&r, 2);
			r.right -= r.left;
			r.bottom -= r.top;
			hwndIconSpin = CreateUpDownControl(
				WS_CHILD|WS_VISIBLE, //style
				r.left+r.right+3,    //left
				r.top,               //top
				20,                  //width
				r.bottom,            //height
				hwndDlg,             //parent
				IDC_ICONSPIN,        //id
				plugin.hDllInstance, //hinst
				0,                   //buddy
				(sizeof(aiWAIcons)/sizeof(aiWAIcons[0]))-1,//upper
				0,                   //lower
				config_iconidx);     //pos
			
			/* Set up our handy tooltip object, best to do this last! */
			tipobj = new CNxSToolTip(hwndDlg, plugin.hDllInstance);
			tipobj->SetMaxTipWidth(400);
			tipobj->AddDialogControls();

			return FALSE;
		}
	case WM_DESTROY:
		delete tipobj;
		DestroyWindow(hwndIconSpin);
		break;
	case WM_NOTIFY:
		{
			LPNMHDR pnmhdr = (LPNMHDR)lParam;
			if (pnmhdr->code==PSN_SETACTIVE) {
				tipobj->InstallHook();
			} else if (pnmhdr->code==PSN_KILLACTIVE) {
				tipobj->UninstallHook();
			}
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ENABLEDCB:
			config_enabled = (IsDlgButtonChecked(hwndDlg, IDC_ENABLEDCB)==BST_CHECKED);
			break;
		case IDC_USEWATRAYCB:
			config_usewatray = (IsDlgButtonChecked(hwndDlg, IDC_USEWATRAYCB)==BST_CHECKED);
			break;
		case IDC_VOLCHANGES:
			config_displayonvolchange = (IsDlgButtonChecked(hwndDlg, IDC_VOLCHANGES)==BST_CHECKED);
			break;
		case IDC_PBSTATECHANGES:
			config_displayonpbchange = (IsDlgButtonChecked(hwndDlg, IDC_PBSTATECHANGES)==BST_CHECKED);
			break;
		case IDC_USEMLCB:
			config_useml = (IsDlgButtonChecked(hwndDlg, IDC_USEMLCB)==BST_CHECKED);
			break;
		case IDC_DISPLAYONTITLECHANGES:
			config_displayontitlechanges = (IsDlgButtonChecked(hwndDlg, IDC_DISPLAYONTITLECHANGES)==BST_CHECKED);
			break;
		case IDC_UPDATEMSNCHECK:
			config_updatemsn = (IsDlgButtonChecked(hwndDlg, IDC_UPDATEMSNCHECK)==BST_CHECKED);
			break;
		}
		break;
		case WM_HSCROLL:
		case WM_VSCROLL:
			if ( GetDlgItem(hwndDlg, IDC_TBTIMEOUT)==(HWND)lParam ) {
				char s[256];
				DWORD dwPos = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
				wsprintf(s, "Timeout: %d sec.", dwPos);
				SetDlgItemText(hwndDlg, IDC_GBTIMEOUT, s);
				config_timeout = dwPos*1000;
			} else if ( hwndIconSpin==(HWND)lParam ) {
				DWORD dwPos = LOWORD(SendMessage((HWND)lParam, UDM_GETPOS, 0, 0));
				SendDlgItemMessage(hwndDlg, IDC_ICONCTL, STM_SETIMAGE,
					IMAGE_ICON, (WPARAM)hWAIcons[dwPos]);
				config_iconidx = dwPos;
			}
			break;
	}/* switch (uMsg)... */
	return FALSE;
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


INT_PTR CALLBACK TitleFormatConfigPageProc(HWND hwndDlg,UINT uMsg,WPARAM wParam, LPARAM lParam)
{
	static CNxSToolTip *tipobj;
	static HWND hwndIconSpin;

	static int shit;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			/* Initialize controls */
			SetDlgItemText(hwndDlg, IDC_FORMATEDIT, szAdvFormatStr);
			SetDlgItemText(hwndDlg, IDC_BALLOONTITLEEDIT, szBalloonTitle);
			SetDlgItemText(hwndDlg, IDC_CLIPBOARDTITLEEDIT, szClipboardTitle);

			if (config_iconidx<0 || config_iconidx>(sizeof(aiWAIcons)/sizeof(aiWAIcons[0])))
				config_iconidx = 0;

			/* Set up our handy tooltip object, best to do this last! */
			tipobj = new CNxSToolTip(hwndDlg, plugin.hDllInstance);
			tipobj->SetMaxTipWidth(400);
			tipobj->AddDialogControls();

			return FALSE;
		}
	case WM_DESTROY:
		delete tipobj;
		DestroyWindow(hwndIconSpin);
		break;
	case WM_NOTIFY:
		{
			LPNMHDR pnmhdr = (LPNMHDR)lParam;
			if (pnmhdr->code==PSN_SETACTIVE) {
				tipobj->InstallHook();
			} else if (pnmhdr->code==PSN_KILLACTIVE) {
				tipobj->UninstallHook();
			}
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SYNTAXBTN:
			OpenSyntaxHelpAndReadMe(hwndDlg);
			break;
		case IDC_TEST:
			SendMessage(plugin.hwndParent, WM_WA_IPC, BalloonMsg::SHOW, g_BalloonIPCMsg);
			break;
		case IDC_DEFBUTTON:
			SetDlgItemText(hwndDlg, IDC_FORMATEDIT, GetResStr(IDS_DEFFORMATSTR));
			PostMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_FORMATEDIT, EN_CHANGE), 0);
			break;
		case IDC_FORMATEDIT:
			if (HIWORD(wParam) != EN_CHANGE) break;
			GetDlgItemText(hwndDlg, IDC_FORMATEDIT, szAdvFormatStr, 512);
			break;
		case IDC_DEFBALLOONTITLEBTN:
			SetDlgItemText(hwndDlg, IDC_BALLOONTITLEEDIT, GetResStr(IDS_DEFBALLOONTITLE));
			PostMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_BALLOONTITLEEDIT, EN_CHANGE), 0);
			break;
		case IDC_BALLOONTITLEEDIT:
			if (HIWORD(wParam) != EN_CHANGE) break;
			GetDlgItemText(hwndDlg, IDC_BALLOONTITLEEDIT, szBalloonTitle, 512);
			break;
		case IDC_CLIPBOARDTITLEEDIT:
			if (HIWORD(wParam) != EN_CHANGE) break;
			GetDlgItemText(hwndDlg, IDC_CLIPBOARDTITLEEDIT, szClipboardTitle, 512);
			break;
		}
		break;
	}/* switch (uMsg)... */
	return FALSE;
}



INT_PTR CALLBACK AboutProc(HWND hwndDlg,UINT uMsg,WPARAM wParam, LPARAM lParam)
{
	static UINT timer;
	static char* (*export_sa_get)(void);
	static void (*export_sa_setreq)(int);
	static HBITMAP hbm;
	static HBITMAP oldbm;
	static HDC memDC;
	static char sapeaks[150];
	static char safalloff[150];
	static char sapeaksdec[150];

	static CNxSToolTip *tipobj;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			char *szOS;
			int iStrLen;
			HDC tmpdc;
			RECT r;
			LOGFONT lf={0,};

			SetDlgItemText(hwndDlg, IDC_VERSION, PLUGIN_TITLE);

			iStrLen = GetOSString(0, 0)+1;
			szOS = (char*)LocalAlloc(LPTR, iStrLen);
			GetOSString(szOS, iStrLen);
			SetDlgItemText(hwndDlg, IDC_OSVERSION, szOS);
			LocalFree((HLOCAL)szOS);

			/* Get function pointers from Winamp */
			export_sa_get = (char* (*)(void))SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETSADATAFUNC);
			export_sa_setreq = (void (*)(int))SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_GETSADATAFUNC);

			/* Create a memory DC to be BitBlt'ed to IDC_SAFRAME in
			   WM_DRAWITEM and WM_TIMER. */
			GetClientRect(GetDlgItem(hwndDlg, IDC_SAFRAME), &r);
			tmpdc = GetDC(hwndDlg);
			hbm = CreateCompatibleBitmap(tmpdc, (r.right-r.left), (r.bottom-r.top));
			memDC = CreateCompatibleDC(tmpdc);
			oldbm = (HBITMAP)SelectObject(memDC, hbm);
			ReleaseDC(hwndDlg, tmpdc);

			SetWindowLong(GetDlgItem(hwndDlg, IDC_SAFRAME), GWL_STYLE, SS_OWNERDRAW|WS_VISIBLE|WS_CHILD);

			/* Load "Whats new?" from resource and put it into IDC_WHATSNEWEDIT */
			HANDLE hres=FindResource(plugin.hDllInstance, (LPCTSTR)IDR_WHATSNEW, RT_RCDATA);
			hres=LoadResource(plugin.hDllInstance, (HRSRC)hres);
			hres=LockResource((HGLOBAL)hres);
			SetDlgItemText(hwndDlg, IDC_WHATSNEWEDIT, (LPCTSTR)hres);
			UnlockResource((HGLOBAL)hres);

			/* Also set the font so it looks okay... */
			lf.lfHeight = -11;
			lstrcpyn(lf.lfFaceName, "MS Shell Dlg", LF_FACESIZE);
			SendDlgItemMessage(hwndDlg, IDC_WHATSNEWEDIT, WM_SETFONT,
				(WPARAM)CreateFontIndirect(&lf), 0);


			lf.lfWeight = FW_BOLD;
			SendDlgItemMessage(hwndDlg, IDC_VERSION, WM_SETFONT, (WPARAM)CreateFontIndirect(&lf), 0);

			/* Set up our handy tooltip object, best to do this last */
			tipobj = new CNxSToolTip(hwndDlg, plugin.hDllInstance);
			tipobj->SetMaxTipWidth(400);
			tipobj->AddDialogControls();

			timer = SetTimer(hwndDlg, 1, 25, NULL);
			break;
		}
		case WM_DESTROY:
			tipobj->UninstallHook();
			delete tipobj;
			KillTimer(hwndDlg, timer);
			SelectObject(memDC, oldbm);
			DeleteObject(hbm);
			DeleteDC(memDC);
			break;
		case WM_NOTIFY:
			{
				LPNMHDR pnmhdr = (LPNMHDR)lParam;
				if (pnmhdr->code==PSN_SETACTIVE) {
					tipobj->InstallHook();
				} else if (pnmhdr->code==PSN_KILLACTIVE) {
					tipobj->UninstallHook();
				}
			}
			break;
		case WM_TIMER:
		if (wParam==1) {
			HWND hwndSA;
			char *sadata;
			RECT r;
			int x;
			COLORREF aClr[2];

			hwndSA = GetDlgItem(hwndDlg, IDC_SAFRAME);

			GetClientRect(hwndSA, &r);
			r.right -= r.left;
			r.bottom -= r.top;

			/* Specify, that we want both spectrum and oscilloscope data */
			export_sa_setreq(1); /* Pass 0 (zero) and get spectrum data only */
			sadata = export_sa_get();

			/* get colors */
			aClr[0] = GetSysColor(COLOR_BTNFACE);
			aClr[1] = GetSysColor(COLOR_BTNTEXT);
			
			/* clear background */
			HBRUSH hBr, hOldBr;
			HPEN hPn, hOldPn;

			hBr = CreateSolidBrush(aClr[0]);
			hOldBr = (HBRUSH)SelectObject(memDC, hBr);
			hPn = CreatePen(PS_SOLID, 0, aClr[1]);
			hOldPn = (HPEN)SelectObject(memDC, hPn);
			Rectangle(memDC, 0, 0, r.right, r.bottom);
			SetBkColor(memDC, aClr[0]);
			SetTextColor(memDC, aClr[1]);

			if (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_ISPLAYING) >= 1)
			{
				/* draw simple analyser... */
				for (x = 0; x < 75; x ++)
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


					MoveToEx(memDC, 10+x, r.bottom, NULL);
					LineTo(memDC, 10+x, r.bottom - safalloff[x]);

					SetPixel(memDC, 10+x, r.bottom - sapeaks[x], aClr[1]);
				}
				/* ...and a solid oscilloscope */
				for (x = 0; x < 75; x ++)
				{
					MoveToEx(memDC,10+x, (r.bottom >> 2), NULL);	
					LineTo(memDC, 10+x, (r.bottom >> 2) - sadata[75+x]);
				}
				/* draw time */
				char szTime[32];
				double time_s = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTTIME)*0.001;
				int hours     = (int)(time_s / 60 / 60) % 60;
				time_s       -= (hours*60*60);
				int minutes   = (int)(time_s/60);
				time_s       -= (minutes*60);
				int seconds   = (int)(time_s);
				time_s       -= seconds;
				int dsec      = (int)(time_s*100);

				if (hours > 0)
					wsprintf(szTime, "%2d:%.2d:%.2d.%.2d", hours, minutes, seconds, dsec);
				else wsprintf(szTime, "%.2d:%.2d.%.2d", minutes, seconds, dsec);

				DrawText(memDC, szTime, -1, &r, DT_SINGLELINE|DT_CENTER|DT_VCENTER);
			} else {
				DrawText(memDC, "Not playing!", -1, &r, DT_SINGLELINE|DT_CENTER|DT_VCENTER);
			}
			SelectObject(memDC, hOldBr);
			DeleteObject(hBr);
			SelectObject(memDC, hOldPn);
			DeleteObject(hPn);

			/* update frame */
			{
				HDC hdc = GetDC(hwndSA);
				BitBlt(hdc, 0, 0,
					r.right-r.left,
					r.bottom-r.top, memDC, 0, 0, SRCCOPY);
				ReleaseDC(hwndSA, hdc);
			}
			return 0;
		}
		case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
			if (lpdis->CtlID == IDC_SAFRAME)
			{
				BitBlt(lpdis->hDC, 0, 0,
					lpdis->rcItem.right-lpdis->rcItem.left,
					lpdis->rcItem.bottom-lpdis->rcItem.top, memDC, 0, 0, SRCCOPY);
			}
			break;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDC_URLBUTTON:
				ExecuteURL(PLUGIN_HOMEPAGE);
				break;
			case IDC_URLBUTTON2:
				ExecuteURL(PLUGIN_FORUMS);
				break;
			case IDC_READMEBTN:
				OpenSyntaxHelpAndReadMe(hwndDlg);
				break;
			}
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
	INI_READ_INT(config_timeout);
	INI_READ_INT(config_iconidx);
	INI_READ_INT(config_usewatray);
	INI_READ_INT(config_displayonpbchange);
	INI_READ_INT(config_displayonvolchange);
	INI_READ_INT(config_useml);
	INI_READ_INT(config_curpageindex);
	INI_READ_INT(config_displayontitlechanges);
	INI_READ_INT(config_updatemsn);

	INI_READ_STR(szAdvFormatStr, GetResStr(IDS_DEFFORMATSTR));
	INI_READ_STR(szBalloonTitle, GetResStr(IDS_DEFBALLOONTITLE));
	INI_READ_STR(szClipboardTitle, GetResStr(IDS_DEFCLIPBOARDTITLE));
}

void config_write()
{
	TCHAR ini_file[MAX_PATH];
	if (!GetPluginINIPath(ini_file)) return;

	INI_WRITE_INT(config_enabled);
	INI_WRITE_INT(config_timeout);
	INI_WRITE_INT(config_iconidx);
	INI_WRITE_INT(config_usewatray);
	INI_WRITE_INT(config_displayonpbchange);
	INI_WRITE_INT(config_displayonvolchange);
	INI_WRITE_INT(config_useml);
	INI_WRITE_INT(config_curpageindex);
	INI_WRITE_INT(config_displayontitlechanges);
	INI_WRITE_INT(config_updatemsn);

	INI_WRITE_STR(szAdvFormatStr);
	INI_WRITE_STR(szBalloonTitle);
	INI_WRITE_STR(szClipboardTitle);
}

extern "C" __declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin()
{
	plugin.version = GPPHDR_VER;
	plugin.config = config;
	plugin.init = init;
	plugin.quit = quit;
	plugin.description = PLUGIN_TITLE;
	return &plugin;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call==DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls((HMODULE)hModule);
	}
    return TRUE;
}
