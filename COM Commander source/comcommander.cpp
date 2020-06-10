// comcommander.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "resource.h"
#include "NxSTrayIcon.h"
#include "wa_ipc.h"

HINSTANCE g_hinst;
HANDLE hPort;
OVERLAPPED overlap;
CNxSTrayIcon *tray;
int g_startport;
HICON hNormalIcon;
HICON hActiveIcon;

#undef DBGCON

#define POLLINTERVAL 150

// Thread messages (Sent as WM_USER, code passed as lParam)
#define TM_UPDATETITLE 1

// Button thread msgs. wParam is button no. starting with 1
#define TM_BUTTONDOWN 2
#define TM_BUTTONUP 3

#define WINAMP_FILE_QUIT                40001
#define WINAMP_BUTTON1                  40044 /*prev*/
#define WINAMP_BUTTON2                  40045 /*play*/
#define WINAMP_BUTTON3                  40046 /*pause*/
#define WINAMP_BUTTON4                  40047 /*stop*/
#define WINAMP_BUTTON5                  40048 /*next*/
#define WINAMP_FFWD5S                   40060
#define WINAMP_REW5S                    40061

// forwards
BOOL CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LogoSubclass(HWND, UINT, WPARAM, LPARAM);
bool IsClientCovered(HWND hwnd);
HANDLE OpenCOMPort(int);
char *GetResStr(int);
void ShowErr(int, ...);
void CALLBACK TimerProc(HWND, UINT, UINT, DWORD);
char* SendMessageRetStr(HWND, UINT, WPARAM, LPARAM);
char* GetPathToWinamp(bool);

void dbgprintf(char *, ...);

// let's start the fun...

HANDLE OpenCOMPort(int portnum)
{
	HANDLE hCom;
	char port[8];
	OSVERSIONINFO osv;
	GetVersionEx(&osv);

	SECURITY_ATTRIBUTES sa={sizeof(sa),};
	SECURITY_DESCRIPTOR sd={0,};
	if (osv.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&sd, true, NULL, false);
		sa.lpSecurityDescriptor = &sd;
	}
	else sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = true;


	wsprintf(port, "COM%d", portnum);

	hCom = CreateFile(port,
		GENERIC_READ | GENERIC_WRITE,
		0, /* exclusive access */
		&sa, /* security attrs */
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		NULL
		);

	if (hCom == INVALID_HANDLE_VALUE) {
		ShowErr(IDS_ERROROPENPORT, port);
		return 0;
	}
	SetCommMask(hCom, EV_CTS|EV_DSR|EV_RING|EV_RLSD);

	return hCom;
}

void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	static int prevpos=-1;
	HWND wa = FindWindow("Winamp v1.x", NULL);
	int pos = SendMessage(wa, WM_WA_IPC, 0, IPC_GETLISTPOS);
	if (!wa)
	{
		char *tmp;
		tmp = GetResStr(IDS_WANOTRUNNING);
		SetDlgItemText(hwnd, IDC_WATITLE, tmp);
		SetDlgItemText(hwnd, IDC_DURATION, "(00:00)");
		LocalFree((HLOCAL)tmp);
		SetDlgItemText(hwnd, IDC_STATUS, "");
	} else {
		int id;
		char *txt;
		switch (SendMessage(wa, WM_WA_IPC, 0, IPC_ISPLAYING))
		{
			case 0: id = IDS_WASTOPPED; break;
			case 1: id = IDS_WAPLAYING; break;
			case 3: id = IDS_WAPAUSED; break;
		}
		txt = GetResStr(id);
		SetDlgItemText(hwnd, IDC_STATUS, txt);
		LocalFree((HLOCAL)txt);

		if (prevpos != pos) //avoid redudant memory access
		{
			char *title;
			char t[256];
			int length = SendMessage(wa, WM_WA_IPC, 1, IPC_GETOUTPUTTIME);
			int s = length % 60;
			int m = (length / 60) % 60;
			int h = (length / 60) / 60;
			title = SendMessageRetStr(wa, WM_WA_IPC, pos, IPC_GETPLAYLISTTITLE);
			wsprintf(t, "%d. %s", pos+1, title);
			SetDlgItemText(hwnd, IDC_WATITLE, t);
			if (h > 0) wsprintf(t, "(%.2d:%.2d:%.2d)", h, m, s);
			else wsprintf(t, "(%.2d:%.2d)", m, s);
			SetDlgItemText(hwnd, IDC_DURATION, t);
			LocalFree((HLOCAL)title);
			prevpos = pos;
		}
	}
}

// This thread monitors the COM port for modem status changes
DWORD WINAPI ThreadFunc( LPVOID p )
{
	DWORD dwEventMask=0;
	DWORD dwModemStat=0;
	DWORD lastActive=0;
	DWORD lastTime=0;
	bool ignoreNext=false;
	bool ignorePrev=false;
	bool fBlinkToggle=false;
	MSG m;

	while (1)
	{
		while (PeekMessage(&m, 0, 0, 0, PM_REMOVE))
		{
			if (GetMessage(&m, 0, 0, 0))
			{
				TranslateMessage(&m);
				DispatchMessage(&m);
			}
		}

		Sleep(0); /* Relinquish remainder of time-slice */
		Sleep( dwModemStat?250:0 );

		if (GetCommModemStatus(hPort, &dwModemStat)) {
			HWND wa = FindWindowEx(0, 0, "Winamp v1.x", NULL);	
			if (dwModemStat & MS_CTS_ON)
			{
				PostMessage((HWND)p, WM_USER, 1, TM_BUTTONDOWN);

				if (SendMessage(wa, WM_WA_IPC, 0, IPC_ISPLAYING)==0)
					PostMessage(wa, WM_COMMAND, WINAMP_FILE_QUIT, 0);
				SendMessage(wa, WM_COMMAND, WINAMP_BUTTON4, 0);
			} else {
				if (lastActive & MS_CTS_ON)
					PostMessage((HWND)p, WM_USER, 1, TM_BUTTONUP);
			}

			if (dwModemStat & MS_DSR_ON)
			{
				PostMessage((HWND)p, WM_USER, 2, TM_BUTTONDOWN);

				if (!wa)
				{
					char *tmp=GetPathToWinamp(true);
					ShellExecute((HWND)p, "open", tmp, NULL, NULL, SW_SHOWNORMAL);
					LocalFree((HLOCAL)tmp);
				}
				int s = SendMessage(wa, WM_WA_IPC, 0, IPC_ISPLAYING);
				switch (s)
				{
				case 0: //stopped
					SendMessage(wa, WM_COMMAND, WINAMP_BUTTON2, 0);
					break;
				case 1: //playing
					SendMessage(wa, WM_COMMAND, WINAMP_BUTTON3, 0);
					break;
				case 3: //paused
					SendMessage(wa, WM_COMMAND, WINAMP_BUTTON2, 0);
				}
			} else {
				if (lastActive & MS_DSR_ON)
					PostMessage((HWND)p, WM_USER, 2, TM_BUTTONUP);
			}

			if (dwModemStat & MS_RING_ON)
			{
				PostMessage((HWND)p, WM_USER, 3, TM_BUTTONDOWN);

				if (lastActive & MS_RING_ON)
				{
					if (lastTime < GetTickCount())
					{
						if (SendMessage(wa, WM_WA_IPC, 0, IPC_ISPLAYING)==0)
							SendMessage(wa, WM_COMMAND, WINAMP_BUTTON1, 0);
						else
							SendMessage(wa, WM_COMMAND, WINAMP_REW5S, 0);
						ignorePrev = true;
					}

					if (lastTime == 0) lastTime = GetTickCount()+1000;
				}
				
			} else {
				if (lastActive & MS_RING_ON)
					PostMessage((HWND)p, WM_USER, 3, TM_BUTTONUP);

				if ( (lastActive & MS_RING_ON) && ignorePrev==false )
				{
					SendMessage(wa, WM_COMMAND, WINAMP_BUTTON1, 0);
					lastTime = 0;
				} else ignorePrev=false;
			}

			if (dwModemStat & MS_RLSD_ON)
			{
				PostMessage((HWND)p, WM_USER, 4, TM_BUTTONDOWN);

				if (lastActive & MS_RLSD_ON)
				{
					if (lastTime < GetTickCount())
					{
						if (SendMessage(wa, WM_WA_IPC, 0, IPC_ISPLAYING)==0)
							SendMessage(wa, WM_COMMAND, WINAMP_BUTTON5, 0);
						else
							SendMessage(wa, WM_COMMAND, WINAMP_FFWD5S, 0);
						ignoreNext = true;
					}

					if (lastTime == 0) lastTime = GetTickCount()+1000;
				}
				
			} else {
				if (lastActive & MS_RLSD_ON)
					PostMessage((HWND)p, WM_USER, 4, TM_BUTTONUP);

				if ( (lastActive & MS_RLSD_ON) && ignoreNext==false )
				{
					SendMessage(wa, WM_COMMAND, WINAMP_BUTTON5, 0);
					lastTime = 0;
				} else ignoreNext=false;
			}

			lastActive = dwModemStat;

			//Update title if button is pressed
			dwModemStat?PostMessage((HWND)p, WM_USER, 0, TM_UPDATETITLE):0;
			if (!wa)
			{
				fBlinkToggle = !fBlinkToggle;
				Sleep(150);
				EscapeCommFunction(hPort, (fBlinkToggle?SETRTS:CLRRTS));
			} else {
				int status=SendMessage(wa, WM_WA_IPC, 0, IPC_ISPLAYING);
				EscapeCommFunction(hPort, (status==1)?SETRTS:CLRRTS);
			}
			tray->SetIcon(dwModemStat?hActiveIcon:hNormalIcon);
		}
	}
	return 0;
}

WNDPROC OldLogoWndProc;
LRESULT CALLBACK LogoSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Change mouse into a "hand" to indicate a clickable area
	if (msg==WM_SETCURSOR)
		SetCursor(LoadCursor(0,IDC_HAND));
	else return CallWindowProc(OldLogoWndProc, hwnd, msg, wParam, lParam);
	return 0;
}

/* This function hides/shows the window in a fancy way.
 * First it minimizes/restores it (Windows 98 and upwards animates this).
 * Then it hides/shows it (making it disappear from the taskbar).
 * It is used frequently in the MainDlgProc dialog procedure.
 */
void MyShowWindow(HWND hwnd, bool fShow)
{
	if (fShow)
	{
		ShowWindow(hwnd, SW_SHOW);
		ShowWindow(hwnd, SW_RESTORE);
		SetForegroundWindow(hwnd);
	} else {
		ShowWindow(hwnd, SW_MINIMIZE);
		ShowWindow(hwnd, SW_HIDE);
	}
}

BOOL CALLBACK MainDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int lastportnum;
	static bool AboutBoxVisible;
	RECT wr;
	HMENU popup;
	int x;
	char *txt;
	HWND wa;

	switch (msg)
	{
	case WM_INITDIALOG:
		tray = new CNxSTrayIcon(hdlg, WM_USER+1231);
		txt = GetResStr(IDS_TOOLTIP);
		tray->SetTooltip(txt);
		LocalFree((HLOCAL)txt);
		hNormalIcon = (HICON)LoadImage(g_hinst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);
		hActiveIcon = (HICON)LoadImage(g_hinst, MAKEINTRESOURCE(IDI_ICON2), IMAGE_ICON, 16, 16, 0);
		tray->SetIcon(hNormalIcon);
		tray->Show();

		PostMessage(hdlg, WM_SETICON, 0, (LPARAM)hNormalIcon);

		txt = GetResStr(IDS_COMPORTS);
		for (x=1;x<5;x++)
		{
			char s[32];
			wsprintf(s, txt, x);
			SendDlgItemMessage(hdlg, IDC_PORTS, CB_ADDSTRING, 0, (LPARAM)s);
		}
		LocalFree((HLOCAL)txt);

		hPort = OpenCOMPort(g_startport);
		if (hPort > 0) 
		{
			SendDlgItemMessage(hdlg, IDC_PORTS, CB_SETCURSEL, g_startport-1, 0);
			lastportnum = g_startport;
		}

		/* Subclass IDC_LOGO so the cursor changes to a
		   "hand" when the mouse hovers over it. */
		OldLogoWndProc = (WNDPROC) SetWindowLong(GetDlgItem(hdlg, IDC_LOGO),
			GWL_WNDPROC, (LONG)LogoSubclass);

		SetTimer(hdlg, 0, 1000, TimerProc);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
		wa = FindWindow("Winamp v1.x", NULL);
		switch (LOWORD(wParam))
		{
		case IDCLOSE:
			delete tray;
			KillTimer(hdlg, 0);
			EscapeCommFunction(hPort, CLRRTS);
			CloseHandle(hPort);
			DestroyWindow(hdlg);
			break;
		case IDCANCEL:
			MyShowWindow(hdlg, false);
			break;
		case ID_POPUP_SHOWHIDE:
			MyShowWindow(hdlg, !IsWindowVisible(hdlg));
			break;
		case IDC_PORTS:
			if (HIWORD(wParam)==CBN_SELCHANGE)
			{
				int sel=SendDlgItemMessage(hdlg, IDC_PORTS, CB_GETCURSEL, 0, 0);
				CloseHandle(hPort); // close old handle
				hPort = OpenCOMPort(1+sel); //try to open new port
				if (hPort == 0)
				{
					// could not open new port, revert to old one
					SendDlgItemMessage(hdlg, IDC_PORTS, CB_SETCURSEL, lastportnum-1, 0);
					hPort = OpenCOMPort(lastportnum);
				}
				else lastportnum = sel+1; // all fine, remember last good
			}
			break;
		case IDC_LOGO:
		case ID_POPUP_ABOUT:
			if (AboutBoxVisible) break; //Prevent multiple about boxes chaos
			AboutBoxVisible = true;
			DialogBox(g_hinst, MAKEINTRESOURCE(IDD_ABOUTDLG), hdlg, AboutDlgProc);
			AboutBoxVisible = false;
			break;
		case IDC_COMMCFG:
			{
				// Put up configuration dialog, and update settings
				COMMCONFIG cc;
				char port[32];
				wsprintf(port, "COM%d", lastportnum);
				ZeroMemory(&cc, sizeof(COMMCONFIG));
				cc.dwSize = sizeof(COMMCONFIG);
				GetCommState(hPort, &cc.dcb);
				CommConfigDialog(port, hdlg, &cc);
				SetCommState(hPort, &cc.dcb);
			}
		}
		break;
	case WM_SYSCOMMAND:
		if ((0xFFF0 & wParam) == SC_MINIMIZE)
		{
			MyShowWindow(hdlg, false);
			return TRUE;
		}
		break;
	case WM_USER:
		switch (lParam)
		{
		case TM_UPDATETITLE:
			TimerProc(hdlg, WM_TIMER, 0, 1000);
			break;
		case TM_BUTTONDOWN:
			CheckDlgButton(hdlg, IDC_CHECKCTS+(wParam-1), TRUE);
			break;
		case TM_BUTTONUP:
			CheckDlgButton(hdlg, IDC_CHECKCTS+(wParam-1), FALSE);
			break;
		}
		break;
	case WM_USER+1231:
		switch (lParam)
		{
		case WM_LBUTTONUP:
			if (IsClientCovered(hdlg) && IsWindowVisible(hdlg))
				SetForegroundWindow(hdlg);
			else
				MyShowWindow(hdlg, !IsWindowVisible(hdlg));
			break;
		case WM_RBUTTONUP:
			MENUITEMINFO mi={sizeof(MENUITEMINFO),};

			SetForegroundWindow(hdlg); //required fix

			popup = LoadMenu(g_hinst, MAKEINTRESOURCE(IDR_MENU1));
			popup = GetSubMenu(popup, 0);

			mi.fMask = MIIM_TYPE|MIIM_STATE;
			GetMenuItemInfo(popup, ID_POPUP_SHOWHIDE, FALSE, &mi);
			mi.fState |= MFS_DEFAULT;
			txt = GetResStr(IsWindowVisible(hdlg)?IDS_MENUHIDE:IDS_MENUSHOW);
			mi.dwTypeData = txt;
			SetMenuItemInfo(popup, ID_POPUP_SHOWHIDE, FALSE, &mi);
			LocalFree((HLOCAL)txt);
			GetCursorPos((LPPOINT)&wr);
			TrackPopupMenu(popup, 0, wr.left, wr.top, 0, hdlg, NULL);
			DestroyMenu(popup);
			break;
		}
		break;

	}
	return FALSE;
}

BOOL CALLBACK AboutDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool toggle;
	static HBITMAP img1;
	static HBITMAP img2;

	HINSTANCE hlib;
	int (__cdecl *fnexecurl)(char *);

	switch (msg)
	{
	case WM_INITDIALOG:
		img1 = (HBITMAP)LoadImage(g_hinst, MAKEINTRESOURCE(IDB_BITMAP1), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		img2 = (HBITMAP)LoadImage(g_hinst, MAKEINTRESOURCE(IDB_BITMAP2), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
		SetTimer(hdlg, 0, 2000, NULL);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			DeleteObject(img1);
			DeleteObject(img2);
			KillTimer(hdlg, 0);
			EndDialog(hdlg, 0);
			break;
		case IDC_HOMEPAGE:
			/* weblink.dll has already been loaded, so just get the
			   module handle */
			hlib = GetModuleHandle("weblink");
			// Get the address of a utility function included with NxSWebLink.
			fnexecurl = (int (__cdecl *)(char *))GetProcAddress(hlib, "ExecuteURL");
			fnexecurl("http://members.tripod.com/files_saivert/");
			break;
		case IDC_HOMEPAGE2:
			/* weblink.dll has already been loaded, so just get the
			   module handle */
			hlib = GetModuleHandle("weblink");
			// Get the address of a utility function included with NxSWebLink.
			fnexecurl = (int (__cdecl *)(char *))GetProcAddress(hlib, "ExecuteURL");
			fnexecurl("http://www.winamp.com/player/");
			break;
		case IDC_HOMEPAGE3:
			{
				char path[MAX_PATH+1];
				char *p;

				GetModuleFileName(NULL, path, sizeof(path));
				p = path+lstrlen(path);
				while (p > path && *p != '\\') p--;
				if (++p > path) *p = 0;
				lstrcat(path, "readme.txt");
				
				if (GetFileAttributes(path) != 0xFFFFFFFF)
					ShellExecute(hdlg, "open", path, NULL, NULL, SW_SHOWNORMAL);
				else ShowErr(IDS_NOREADME, path);
				break;
			}
		}
		break;
	case WM_TIMER:
		/* This uses AnimateWindow(), a recently new function
		   available under Windows XP. It's an easy way to do
		   a transition between the images (like a slide-show). */
		AnimateWindow(GetDlgItem(hdlg, IDC_IMAGE), 0,
			AW_SLIDE|AW_HIDE|AW_HOR_NEGATIVE|(toggle?AW_VER_POSITIVE:AW_VER_NEGATIVE));
		SendDlgItemMessage(hdlg, IDC_IMAGE, STM_SETIMAGE, IMAGE_BITMAP,
			(LPARAM) (toggle?img1:img2));
		toggle = !toggle;
		AnimateWindow(GetDlgItem(hdlg, IDC_IMAGE), 0,
			AW_SLIDE|(toggle?AW_HOR_POSITIVE:AW_HOR_NEGATIVE)|AW_VER_POSITIVE);
		break;
	}
	return FALSE;
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	HWND hdlg;
	RECT dr, wr;
	MSG msg;
	DWORD dwThreadId;
	char *p;
	bool starthidden=false;
	OSVERSIONINFO osv;
	SECURITY_ATTRIBUTES sa={sizeof(sa),};
	SECURITY_DESCRIPTOR sd={0,};


	/* Get ourself a Security Descriptor for use by CreateThread().
	   Only required by Windows NT though. */
	GetVersionEx(&osv);
	if (osv.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&sd, true, NULL, false);
		sa.lpSecurityDescriptor = &sd;
	}
	else sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = true;
	
	
	g_startport=1; //COM1 by default


	/* Only supports lowercase variants of arguments.
	   "/H" and "/C" are ignored. Use "/h" and "/c". */

	if (p=strstr(lpCmdLine, "/?"))
	{
		p = GetResStr(IDS_USAGE);
		MessageBox(0, p, "COM Commander", 64);
		LocalFree((HLOCAL)p);
		return 0;
	}

	if (p=strstr(lpCmdLine, "/h"))
		starthidden = true;
	if (p=strstr(lpCmdLine, "/c"))
	{
		p += 2;
		g_startport = *p-48;
	}
	
	InitCommonControls();
	if (!LoadLibrary("weblink.dll"))
		ShowErr(IDS_ERRORWEBLINKREG);

	g_hinst = hInstance;
	
	

	hdlg = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAINDLG), 0, MainDlgProc);
	GetWindowRect(hdlg, &dr);
	SystemParametersInfo(SPI_GETWORKAREA, 0, &wr, 0);
	SetWindowPos(hdlg, 0, (wr.right-dr.right) / 2, (wr.bottom-dr.right) / 2, 0, 0, SWP_NOSIZE|SWP_NOZORDER);

	if (!starthidden) ShowWindow(hdlg, nCmdShow);

	CreateThread(&sa, 0, ThreadFunc, hdlg, 0, &dwThreadId);

	while (GetMessage(&msg, 0, 0, 0))
	{
		if (!IsDialogMessage(hdlg, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}



/*** UTILITY FUNCTIONS ***/

#if defined(DBGCON)
void dbgprintf(char *txt, ...)
{
	static BOOL hasConsole;
	if (!txt)
	{
		hasConsole = !FreeConsole();
		return;
	}
	if (!hasConsole) hasConsole = AllocConsole();
	va_list vl;
	va_start(vl, txt);
	DWORD written;
	char *s, *p;
	s = (char*)LocalAlloc(LPTR, 256);
	p = s;
	wvsprintf(s, txt, vl);
	while (*p) p++;
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), s, (DWORD)(p-s), &written, NULL);
	LocalFree((HLOCAL)s);
	va_end(vl);
}
#endif

/* Shows a custom error message along with the last OS errorcode and
   the OS specific error message.
   Use like wsprintf (pass any number of args).
   String from stringtable is parsed for format tags.
   msgid is any "IDS_*" defined.
*/
void ShowErr(int msgid, ...)
{
	va_list vl;
	LPVOID lpMsgBuf;
	int ec;
	char *tmp1;
	char *tmp2;
	char err[256];
	char errmsg[4096];

	ec = GetLastError();
	va_start(vl, msgid);

	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		ec,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);
	tmp1 = GetResStr(msgid);
	tmp2 = GetResStr(IDS_SHOWERR);
	wvsprintf(err, tmp1, vl);
	va_end(vl);
	wsprintf(errmsg, tmp2, err, ec, lpMsgBuf);
	LocalFree((HLOCAL)tmp1);
	LocalFree((HLOCAL)tmp2);
	MessageBox(0, errmsg, 0, MB_OK|MB_ICONERROR);

	LocalFree( lpMsgBuf );
}

/* Simple one: Load a string from a stringtable.
   Returns a LocalAlloc'ed string. Free with LocalFree();
*/
char *GetResStr(int id)
{
	char *s;
	s = (char*)LocalAlloc(LPTR, 256);
	LoadString(g_hinst, id, s, 255);
	return s;
}

/* This function returns true if the client area of hwnd
   is covered by another window, otherwise it returns false.
*/
bool IsClientCovered(HWND hwnd)
{
    HDC hdc;
    RECT rc, rcClient;
    int iType;

    hdc = GetDC(hwnd);
    iType = GetClipBox(hdc, &rc);

    ReleaseDC(hwnd, hdc);

    if (iType == NULLREGION)
        return true;
    if (iType == COMPLEXREGION)
        return true;

    GetClientRect(hwnd, &rcClient);
    if (EqualRect(&rc, &rcClient))
        return false;

    return true;
}

/* This function is just like SendMessage except it handles messages
   that returns char*s (previously for in-proc use only, like plug-ins).
   Using OpenProcess and ReadProcessMemory we can benefit from this in
   out-proc's too.
   Returns a LocalAlloc'ed string. Free with LocalFree();
*/
char* SendMessageRetStr(HWND wnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  DWORD dwProcessId;
  HANDLE phandle;
  char *P;
  DWORD C;
  char B=0;
  char *res;
  char *rp;

  res = (char*)LocalAlloc(LPTR, 4096);
  rp = res;

  GetWindowThreadProcessId(wnd,&dwProcessId);
  phandle = OpenProcess(PROCESS_VM_READ, false, dwProcessId);
  if (phandle < 0) return 0;
  P = (char*)SendMessage(wnd, uMsg, wParam, lParam);
  do {
    if (!ReadProcessMemory(phandle,P++,&B,1,&C) && C==1) break;
    if (B != 0) *rp++ = B;
  } while (B!=0);
  *rp++ = 0;

  CloseHandle(phandle);
  return res;
}

/* Utility function used by 'GetPathToWinamp()' below */
void trimslashtoend(char *buf)
{
  char *p = buf + lstrlen(buf);
  do
  {
    if (*p == '\\')
      break;
    p = CharPrev(buf, p);
  } while (p > buf);

  *p = 0;
}

/* Returns a LocalAlloc'ed string containing the path of either Winamp's folder or
   "winamp.exe" itself depending on what 'executable' is set to.
   Uses the registry to find the path. */
char* GetPathToWinamp(bool executable)
{
	HKEY key;
	DWORD size;
	DWORD type;
	char *path;
	char *res;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Winamp",0,KEY_READ,&key);

	RegQueryValueEx(key, "UninstallString", NULL, &type, NULL, &size);
	if (type==REG_SZ)
	{
		path = (char *)LocalAlloc(LPTR, size);
		res = (char *)LocalAlloc(LPTR, size);
		RegQueryValueEx(key, "UninstallString", NULL, &type, (LPBYTE)path, &size);
		/* Remove quotes */
		if (path[0] == '"') path++;
		if (path[lstrlen(path)] == '"') path[lstrlen(path)] = 0;
		trimslashtoend(path); /* Remove "\uninstwa.exe" from string */
		/* Replace with "\winamp.exe" if executable is True */
		if (executable)	lstrcat(path, "\\winamp.exe");
		lstrcpy(res, path); /* Copy to result */

		/* Clean up */
		LocalFree((HLOCAL)(path-1));
		RegCloseKey(key);
		/* Return result, a LocalAlloc'ed string */
		return res;
	}

	/* Inavlid key, close it and return NULL */
	RegCloseKey(key);
	return 0;
}