// gen_nxsscriptcontrol.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "resource.h"
#include "gen.h"
#include "wa_ipc.h"
#include "wa_hotkeys.h"
#include "wa_msgids.h"
#include "nxstabpages.h"
#include "nxsweblink.h"

#include "comdef.h"
#include "initguid.h"
#include "ScriptedFrame.h"

// P L U G I N   D E F I N E S
#define PLUGIN_TITLE "NxS Script Control v2.4"
#define PLUGIN_PAGETITLE "NxS Script Control"
#define PLUGIN_TITLE_NOTLOAD PLUGIN_TITLE" <Error: Requires Winamp 5.4+>"
#define PLUGIN_CAPTION "NxS Script Control"
#define PLUGIN_INISECTION "NxS Script Control"
#define MENUID_OPENPREFS (46176)
#define PLUGIN_HOMEPAGE "http://members.tripod.com/files_saivert/"

// F U N C T I O N   P R O T O T Y P E S
LRESULT CALLBACK WinampSubclass(HWND, UINT, WPARAM, LPARAM);
void config(void);
void quit(void);
int init(void);
void config_write(void);
void config_read(void);
BOOL LoadFile(HWND hEdit, LPSTR pszFileName);
BOOL SaveFile(HWND hEdit, LPSTR pszFileName);
BOOL CALLBACK ConfigProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK ScriptPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AboutPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);



// G L O B A L   V A R I A B L E S
CScriptedFrame *g_scriptframe=0;
char g_szTypeLibFile[MAX_PATH]={0,};
HWND g_hwndScriptDlg=0;
char g_scriptlang[256]={0,};

static int g_waver;
static WNDPROC lpOldWinampWndProc; //Old window procedure pointer
static int config_pageindex=0;
static CNxSTabPages *tp;


// Plug-in structure
winampGeneralPurposePlugin plugin = {
	GPPHDR_VER,
	PLUGIN_TITLE,
	init,
	config,
	quit,
};

// Structure used to add a custom page to Winamp's preferences dialog
prefsDlgRec prefsrec = {
	0,					//HInstance (filled in later)
	IDD_CONFIGPAGE,		//Dialog resource ID
	ConfigProc,			//Dialog procedure
	PLUGIN_PAGETITLE,	//Page title
	0,					//Location in tree-view
};

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

// Most important!!
extern "C" __declspec(dllexport) winampGeneralPurposePlugin* winampGetGeneralPurposePlugin()
{
	return &plugin;
}


char *GetResStr(int id)
{
	static char str[8192];
	if (LoadString(plugin.hDllInstance, id, str, sizeof(str)/sizeof(str[0])))
		return str;
	else
		return NULL;
}


int init()
{
	RegisterNxSWebLink(plugin.hDllInstance);

	g_waver = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETVERSION);

	if (g_waver < 0x5001)
	{
		plugin.description = PLUGIN_TITLE_NOTLOAD;
		return 0;
	}

	config_read();

	HMENU WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);

	MENUITEMINFO mii;
	mii.cbSize		= sizeof(MENUITEMINFO);
	mii.fMask		= MIIM_TYPE|MIIM_ID;
	mii.dwTypeData	= PLUGIN_CAPTION;
	mii.fType		= MFT_STRING;
	mii.wID			= MENUID_OPENPREFS;
	
	InsertMenuItem(WinampMenu, WINAMP_OPTIONS_VIDEO, FALSE, &mii);
	// Always remember to adjust "Option" submenu position.
	SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_ADJUST_OPTIONSMENUPOS);


	// Add preferences page
	prefsrec.hInst = plugin.hDllInstance;
	SendMessage(plugin.hwndParent, WM_WA_IPC, (int)&prefsrec, IPC_ADD_PREFS_DLG);

	lpOldWinampWndProc = (WNDPROC)SetWindowLong(plugin.hwndParent, GWL_WNDPROC,
		(int) WinampSubclass);



	// Extract type library file (.tlb)
	BYTE *data;
	HANDLE hfile, hres;
	DWORD len,c;
	char temppath[MAX_PATH];
	// get the type library resource
	if (!(hres=FindResource(plugin.hDllInstance,(LPCTSTR)1, "TYPELIB"))
		|| !(len=SizeofResource(plugin.hDllInstance,(HRSRC)hres))
		|| !(hres=LoadResource(plugin.hDllInstance,(HRSRC)hres))
		|| !(data=(BYTE *)LockResource((HGLOBAL)hres))) {
		return FALSE;
	}
	// get a temporary filename
	GetTempPath(MAX_PATH,temppath);
	GetTempFileName(temppath,"tlb",0, g_szTypeLibFile);

	// write type library to the temporary file
	if (INVALID_HANDLE_VALUE==(hfile=CreateFile(g_szTypeLibFile,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY,NULL))) {
		return FALSE;
	}
	WriteFile(hfile,data,len,&c,NULL);
	CloseHandle(hfile);

//	OleInitialize(NULL);

	g_scriptframe=new CScriptedFrame();
	g_scriptframe->AddRef();


	return 0; //success
}

void quit()
{
	g_scriptframe->Release();
	DeleteFile(g_szTypeLibFile);
	config_write();
}

void config()
{
	if (g_waver >= 0x5001)
	{
		SendMessage(plugin.hwndParent, WM_WA_IPC, prefsrec._id, IPC_OPENPREFSTOPAGE);
	} else {
		MessageBox(plugin.hwndParent,
			TEXT(PLUGIN_TITLE "\r\n"
			"This plug-in requires a version of Winamp greater than or equal to 5.1!"),
			PLUGIN_CAPTION, MB_ICONWARNING);
	}
}

BOOL CALLBACK ConfigProc(HWND hwndDlg,UINT uMsg,WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			HWND h;
			RECT r;

			// Adjust dialogbox and the tab control so it fits in the
			// preferences dialog. It just looks better that way!
			h = GetNextWindow(hwndDlg, GW_HWNDPREV); // Get handle of "placeholder" static
			GetClientRect(h, &r);
			MoveWindow(hwndDlg, 0, 0, r.right, r.bottom, TRUE);
			MoveWindow(GetDlgItem(hwndDlg, IDC_TAB1), 0, 0, r.right, r.bottom, TRUE);

			tp = new CNxSTabPages(GetDlgItem(hwndDlg, IDC_TAB1), plugin.hDllInstance);
			tp->AddPage(NULL, MAKEINTRESOURCE(IDD_GENERAL), GeneralPageProc, (char*)IDS_TIPGENERALPAGE);
			tp->AddPage(NULL, MAKEINTRESOURCE(IDD_SCRIPT), ScriptPageProc, (char*)IDS_TIPSCRIPTPAGE);
			tp->AddPage(NULL, MAKEINTRESOURCE(IDD_ABOUT), AboutPageProc, (char*)IDS_TIPABOUTPAGE);
			tp->SelectPage(config_pageindex);
		}
		break;
	case WM_NOTIFY:
		{
			LRESULT lRes = tp->HandleNotifications(wParam, lParam);
			if (lRes) {
				config_pageindex = tp->GetSelPage();
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

BOOL CALLBACK GeneralPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			int i;
			char *strings[4] = {"VBScript", "JScript", "PerlScript", "PythonScript"};
			for (i=0; i<sizeof(strings)/sizeof(strings[0]); i++) {
				SendDlgItemMessage(hwndDlg, IDC_SCLANGCOMBO, CB_ADDSTRING, 0, LPARAM(strings[i]));
			}

			char *samples[4] = {"Using AddToLog", "Get volume/panning", "Using IE to display HTML", "Creating shortcuts on desktop"};
			for (i=0; i<sizeof(samples)/sizeof(samples[0]); i++) {
				SendDlgItemMessage(hwndDlg, IDC_SAMPLECOMBO, CB_ADDSTRING, 0, LPARAM(samples[i]));
			}
		}
		break;
	case WM_NOTIFY:
		{
			LPNMHDR pnmhdr = (LPNMHDR)lParam;
			if (pnmhdr->code==PSN_SETACTIVE) {
			} else if (pnmhdr->code==PSN_KILLACTIVE) {
			}
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_SCLANGCOMBO:
			if (HIWORD(wParam)==CBN_EDITCHANGE) {
				GetDlgItemText(hwndDlg, IDC_SCLANGCOMBO, g_scriptlang, 256);
				SetDlgItemText(hwndDlg, IDC_SCLANG, g_scriptlang);
			} else if ( HIWORD(wParam)==CBN_SELCHANGE ) {
				int sel=SendDlgItemMessage(hwndDlg, IDC_SCLANGCOMBO, CB_GETCURSEL, 0, 0);
				SendDlgItemMessage(hwndDlg, IDC_SCLANGCOMBO, CB_GETLBTEXT, sel, LPARAM(g_scriptlang));
				SetDlgItemText(hwndDlg, IDC_SCLANG, g_scriptlang);
			}
			break;
		case IDC_LOADSAMPLE:
			{
				int sampleids[4] = {IDS_EXAMPLE1, IDS_EXAMPLE2, IDS_EXAMPLE3, IDS_EXAMPLE4};
				int sel=SendDlgItemMessage(hwndDlg, IDC_SAMPLECOMBO, CB_GETCURSEL, 0, 0);
				tp->SelectPage(1);
				SetDlgItemText(tp->GetCurrentPageHWND(), IDC_SCRIPTEDIT, GetResStr(sampleids[sel]));
			}
			break;
		case IDC_LINKTOPAGE:
			tp->SelectPage(1);
			break;
		}
		break;
	}
	return FALSE;
}

BOOL CALLBACK ScriptPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char szBuf[8192];
	LOGFONT lf={0,};

	switch (uMsg)
	{
	case WM_INITDIALOG:
		g_hwndScriptDlg = hwndDlg;
		SetDlgItemText(hwndDlg, IDC_SCRIPTEDIT, GetResStr(IDS_EXAMPLE1));
		lstrcpyn(lf.lfFaceName, "Lucida Console", LF_FACESIZE);
		lf.lfHeight = -11;
		SendDlgItemMessage(hwndDlg, IDC_SCRIPTEDIT, WM_SETFONT,
			WPARAM(CreateFontIndirect(&lf)), 0);
		break;
	case WM_DESTROY:
		g_hwndScriptDlg = 0;
		break;
	case WM_NOTIFY:
		{
			LPNMHDR pnmhdr = (LPNMHDR)lParam;
			/*
			if (pnmhdr->code==PSN_SETACTIVE) {
				MessageBox(hwndDlg, "PSN_SETACTIVE", NULL, 0);
			} else if (pnmhdr->code==PSN_KILLACTIVE) {
				MessageBox(hwndDlg, "PSN_KILLACTIVE", NULL, 0);
			} else if (pnmhdr->code==PSN_RESET) {
				MessageBox(hwndDlg, "PSN_RESET", NULL, 0);
			} else if (pnmhdr->code==PSN_APPLY) {
				MessageBox(hwndDlg, "PSN_APPLY", NULL, 0);
			}
			*/
		}
		break;
	case WM_USER:
		switch (wParam)
		{
		case 1:
			{
				RECT rectLog;
				RECT rectEdit;
				HWND hwndLog = GetDlgItem(hwndDlg, IDC_LOGLIST);
				HWND hwndEdit = GetDlgItem(hwndDlg, IDC_SCRIPTEDIT);

				// Get Rectangles
				GetWindowRect(hwndLog, &rectLog);
				MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rectLog, 2);
				GetWindowRect(hwndEdit, &rectEdit);
				MapWindowPoints(HWND_DESKTOP, hwndDlg, (LPPOINT)&rectEdit, 2);


				if (lParam) {
					SetWindowPos(hwndEdit, 0, 0, 0,  rectEdit.right-rectEdit.left,
						(rectLog.top-rectEdit.top)-5, SWP_NOMOVE | SWP_NOZORDER);

					ShowWindow(hwndLog, SW_SHOWNA);
				} else {
					SetWindowPos(hwndEdit, 0, 0, 0,  rectEdit.right-rectEdit.left,
						(rectLog.bottom-rectEdit.top), SWP_NOMOVE | SWP_NOZORDER);

					ShowWindow(hwndLog, SW_HIDE);
				}
			}
			break;
		case 2:
			if (lParam)	{
				SendDlgItemMessage(hwndDlg, IDC_LOGLIST, LB_ADDSTRING, 0, lParam);
			} else {
				SendDlgItemMessage(hwndDlg, IDC_LOGLIST, LB_RESETCONTENT, 0, 0);
			}
			break;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_RUNSCRIPT:
			if (!g_scriptframe->IsActive()) {
				GetDlgItemText(hwndDlg, IDC_SCRIPTEDIT, szBuf, sizeof(szBuf));
				SendMessage(hwndDlg, WM_USER, 1, TRUE); // Show log
				SendMessage(hwndDlg, WM_USER, 2, 0); //Clear log

				if ( g_scriptframe->InitializeScriptFrame(plugin.hwndParent) )
				{
					g_scriptframe->SetScript(szBuf);
					g_scriptframe->RunScript();
				}

				SetDlgItemText(hwndDlg, IDC_RUNSCRIPT, GetResStr(IDS_HALT));
			} else {
				g_scriptframe->QuitScript(true);
				SendMessage(hwndDlg, WM_USER, 1, FALSE); // Hide log
				SetDlgItemText(hwndDlg, IDC_RUNSCRIPT, GetResStr(IDS_EXECUTE));
			}
			break;
		case IDC_CLEARSCRIPT:
			SetDlgItemText(hwndDlg, IDC_SCRIPTEDIT, TEXT(""));
			break;
		case IDC_SAVETOFILE:
		case IDC_LOADFROMFILE:
			{
				TCHAR szFileName[MAX_PATH]={0,};
				OPENFILENAME ofn;
				ZeroMemory(&ofn, sizeof(OPENFILENAME));
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hwndDlg;
				ofn.lpstrTitle = (LOWORD(wParam)==IDC_LOADFROMFILE)?TEXT("Load script"):TEXT("Save script");
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrFile = szFileName;
				ofn.lpstrFilter =
					"Script files (*.vbs, *.js, *.psc, *.pys)\0*.vbs;*.js;*.psc;*.pys\0"
					"Text files (*.txt)\0*.txt\0"
					"All files\0*.*\0";
				ofn.lpstrDefExt = "txt";
				ofn.Flags = OFN_EXPLORER|OFN_FILEMUSTEXIST;

				if (LOWORD(wParam)==IDC_LOADFROMFILE) {
					if (GetOpenFileName(&ofn)) {
						LoadFile(GetDlgItem(hwndDlg, IDC_SCRIPTEDIT), szFileName);
					}
				} else {
					if (GetSaveFileName(&ofn)) {
						SaveFile(GetDlgItem(hwndDlg, IDC_SCRIPTEDIT), szFileName);
					}
				}
			}
			break;
		}
		break;
	}
	return FALSE;
}

BOOL CALLBACK AboutPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HFONT hf;
	LOGFONT lf;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hwndDlg, IDC_VERSION, PLUGIN_TITLE);

		// Get base font parameters
		hf = HFONT(SendMessage(hwndDlg, WM_GETFONT, 0, 0));
		GetObject(hf, sizeof(LOGFONT), &lf);

		// Set font for IDC_VERSION
		lstrcpyn(lf.lfFaceName, "Verdana", LF_FACESIZE);
		lf.lfHeight = -17;
		lf.lfWeight = FW_BOLD;
		hf = CreateFontIndirect(&lf);
		SendDlgItemMessage(hwndDlg, IDC_VERSION, WM_SETFONT, (WPARAM)hf, 0);

		// Set font for IDC_AUTHOR
		lstrcpyn(lf.lfFaceName, "MS Shell Dlg", LF_FACESIZE);
		lf.lfHeight = -11;
		lf.lfWeight = FW_BOLD;
		hf = CreateFontIndirect(&lf);
		SendDlgItemMessage(hwndDlg, IDC_AUTHOR, WM_SETFONT, (WPARAM)hf, 0);


		//SetWindowLong(GetDlgItem(hwndDlg, IDC_HOMEPAGE), GWL_STYLE,
		//	WS_CHILD|WS_VISIBLE|WS_TABSTOP|WLS_RIGHT|WLS_OPAQUE);
		SendDlgItemMessage(hwndDlg, IDC_HOMEPAGE, WM_SETFONT, (WPARAM)hf, 1);

		break;
	case WM_NOTIFY:
		{
			LPNMHDR pnmhdr = (LPNMHDR)lParam;
			if (pnmhdr->code==PSN_SETACTIVE)
			{
			}
			if (pnmhdr->code==PSN_KILLACTIVE)
			{
			}
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_HOMEPAGE:
			ExecuteURL(PLUGIN_HOMEPAGE);
			break;
		case IDC_HOMEPAGE2:
			ExecuteURL("http://www.winamp.com/");
			break;
		case IDC_HOMEPAGE3:
			ExecuteURL("http://forums.winamp.com/show.php?thread=blah");
			break;
		}
		break;
	}
	return FALSE;
}

LRESULT CALLBACK WinampSubclass(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (LOWORD(wParam)==MENUID_OPENPREFS && HIWORD(wParam) == 0)
		{
			config_pageindex = 1;
			SendMessage(plugin.hwndParent, WM_WA_IPC, prefsrec._id, IPC_OPENPREFSTOPAGE);
			Sleep(1);
			SendMessage(plugin.hwndParent, WM_WA_IPC, prefsrec._id, IPC_OPENPREFSTOPAGE);
			return 0;
		}
		break;
	/* Also handle WM_SYSCOMMAND if people are selectng the item through
	   Winamp submenu in system menu. */
	case WM_SYSCOMMAND:
		if (wParam == MENUID_OPENPREFS)
		{
			config_pageindex = 1;
			SendMessage(plugin.hwndParent, WM_WA_IPC, prefsrec._id, IPC_OPENPREFSTOPAGE);
			Sleep(1);
			SendMessage(plugin.hwndParent, WM_WA_IPC, prefsrec._id, IPC_OPENPREFSTOPAGE);
			return 0;
		}
		break;
	case WM_WA_IPC:
		switch (lParam)
		{
		case IPC_CB_MISC:
			switch(wParam)
			{
			case IPC_CB_MISC_VOLUME:
			case IPC_CB_MISC_STATUS:
			case IPC_CB_MISC_INFO:
			case IPC_CB_MISC_VIDEOINFO:
				break;
			case IPC_CB_MISC_TITLE:
				{
					static unsigned int twice=0;
					if (twice < GetTickCount() ) {
						twice = GetTickCount()+1000;
						if (g_scriptframe && g_scriptframe->IsActive()) g_scriptframe->FireEvent();
					}
				}
				break;
			}
			break;
		}
		break;
	}
	return CallWindowProc((WNDPROC)lpOldWinampWndProc,hwnd,message,wParam,lParam);
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

void config_read()
{
	TCHAR ini_file[MAX_PATH];
	if (!GetPluginINIPath(ini_file)) return;

#define INI_READ_INT_(x) GetPrivateProfileInt(PLUGIN_INISECTION, #x, x, ini_file);
#define INI_READ_INT(x) INI_READ_INT_(x)
#define INI_READ_STR_(x,def) GetPrivateProfileString(PLUGIN_INISECTION, #x, def, x, 512, ini_file);
#define INI_READ_STR(x,def) INI_READ_STR_(x)

	config_pageindex = INI_READ_INT(config_pageindex);
// INI_READ_STR(config_string,"a default string");

#undef INI_READ_INT_
#undef INI_READ_INT
#undef INI_READ_STR_
#undef INI_READ_STR
}

void config_write()
{
	char string[32];
	TCHAR ini_file[MAX_PATH];
	if (!GetPluginINIPath(ini_file)) return;

#define INI_WRITE_INT_(x) \
	wsprintf(string,"%d",x); \
	WritePrivateProfileString(PLUGIN_INISECTION, #x, string, ini_file)
#define INI_WRITE_INT(x) INI_WRITE_INT_(x)
#define INI_WRITE_STR_(x) WritePrivateProfileString(PLUGIN_INISECTION, #x, x, ini_file)
#define INI_WRITE_STR(x) INI_WRITE_STR_(x)

	INI_WRITE_INT(config_pageindex);
// INI_WRITE_STR(config_string);

#undef INI_WRITE_INT_
#undef INI_WRITE_INT
#undef INI_WRITE_STR_
#undef INI_WRITE_STR
}


BOOL LoadFile(HWND hEdit, LPSTR pszFileName)
{
	HANDLE hFile;
	BOOL bSuccess = FALSE;
	
	hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwFileSize;
		dwFileSize = GetFileSize(hFile, NULL);
		if(dwFileSize != 0xFFFFFFFF)
		{
			LPSTR pszFileText;
			pszFileText = LPSTR(GlobalAlloc(GPTR, dwFileSize + 1));
			
			if(pszFileText != NULL)
			{
				DWORD dwRead;
				if(ReadFile(hFile, pszFileText, dwFileSize, &dwRead, NULL))
				{
					pszFileText[dwFileSize] = 0; // Null terminator
				
					if(SetWindowText(hEdit, pszFileText))
						bSuccess = TRUE; // It worked!
				}
				GlobalFree(pszFileText);
			}
		}
		CloseHandle(hFile);
	}
	return bSuccess;
}

BOOL SaveFile(HWND hEdit, LPSTR pszFileName)
{
	HANDLE hFile;
	BOOL bSuccess = FALSE;
	
	hFile = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwTextLength;
		dwTextLength = GetWindowTextLength(hEdit);
		if(dwTextLength > 0)// No need to bother if there's no text.
		{
			LPSTR pszText;
			pszText = LPSTR(GlobalAlloc(GPTR, dwTextLength + 1));
			
			if(pszText != NULL)
			{
				if(GetWindowText(hEdit, pszText, dwTextLength + 1))
				{
					DWORD dwWritten=0;
					
					if(WriteFile(hFile, pszText, dwTextLength, &dwWritten, NULL))
						bSuccess = TRUE;
				}
				GlobalFree(pszText);
			}
		} else {
			bSuccess = TRUE; //let user successfully save a empty file
		}
		CloseHandle(hFile);
	}
	return bSuccess;
}
