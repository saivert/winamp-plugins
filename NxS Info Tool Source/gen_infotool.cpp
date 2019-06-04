/* gen_infotool.cpp: NxS Info Tool main source file
* Defines most of the plug-in.
*/

/* Include AggressiveOptimize.h which defines a
couple of linker pragmas to minimize code size. */
#include "AggressiveOptimize.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <richedit.h>
#include <commctrl.h>
#include <math.h>

#include "resource.h"
#include "gen.h"
#include "wa_ipc.h"
#define WA_DLG_IMPLEMENT
#include "wa_dlg.h"
#include "ipc_pe.h"
#include "wa_msgids.h"

#include "nxstabpages.h"

/* function prototypes */
void config();
void quit();
int init();

prefsDlgRec *prefs;

/* Subclass needed to intercept click on our menu item */
WNDPROC OldWinampWndProc;
LRESULT WinampSubclass(HWND,UINT,WPARAM,LPARAM);

/* more prototypes, dialog/window procedures */
#define A_DLGPROC(name) BOOL CALLBACK name(HWND,UINT,WPARAM,LPARAM)

A_DLGPROC(ConfigProc);
A_DLGPROC(Page1Proc);
A_DLGPROC(Page2Proc);
A_DLGPROC(Page3Proc);
A_DLGPROC(Page4Proc);
A_DLGPROC(Page5Proc);
A_DLGPROC(Page6Proc);

/* Private message sendt to config window */
#define WM_NXSINFOMSG WM_USER+666
#define NXSINFOMSG_SETUPDATEINTERVAL 101

#define PLUGINSHORTNAME "NxSInfoTool"
#define MENUID_SHOWMYWND 24366

/* Our passage way to Winamp */
winampGeneralPurposePlugin plugin = {
	GPPHDR_VER,
		"NxS Info Tool v1.6",
		init,
		config,
		quit,
};

/* Shrink code... */
BOOL WINAPI _DllMainCRTStartup(HINSTANCE hModule,
							   DWORD ul_reason_for_call,
							   LPVOID lpReserved)
{
	return 1;
}

/* Two defines that we use to query out_ds.dll's
"Fade at end of song" settings */
#define XFADE_GETDURATION 0x67
#define XFADE_GETSTATE 0x65
typedef HWND (*embedWindow_t)(embedWindowState *);
embedWindowState cfgews;
BOOL g_isvisible;
BOOL g_autorefresh;
int g_starttabidx;


NxS_TabPages *tp;

#define tp() if (tp) tp

#define NXSINFO_DEFAULTUPDATEINTERVAL 150
BOOL CALLBACK ConfigProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT dr;
	static int updateinterval;
	
	int a;
	a = WADlg_handleDialogMsgs(hwnd, message, wParam, lParam);
	if (a)
	{
		SetWindowLong(hwnd, DWL_MSGRESULT, a);
		return a;
	}
	
	switch (message)
	{
	case WM_INITDIALOG:
		g_isvisible = TRUE;
		
		updateinterval = NXSINFO_DEFAULTUPDATEINTERVAL; // Default interval
		
		WADlg_init(plugin.hwndParent);
		CheckDlgButton(hwnd, IDC_AUTOREFRESH, g_autorefresh?BST_CHECKED:BST_UNCHECKED);
		
		tp = new NxS_TabPages(GetDlgItem(hwnd, IDC_TAB1), plugin.hDllInstance);
		tp->SetUseThemedDlgTexture(false);
		tp->AddPage("Info",       MAKEINTRESOURCE(IDD_INFO),        Page1Proc, "Various information");
		tp->AddPage("Vis' data",  MAKEINTRESOURCE(IDD_VISDATA),     Page2Proc, "Spectrum analyser/Oscilloscope data");
		tp->AddPage("Embed wnds", MAKEINTRESOURCE(IDD_EMBEDANDEXTS),Page3Proc, "Embedded window enumerator");
		tp->AddPage("Misc",       MAKEINTRESOURCE(IDD_MISC),        Page4Proc, "Call Winamp's common dialogs &&& get playlist editor info + a lot of buttons to click.");
		tp->AddPage("Conversion", MAKEINTRESOURCE(IDD_CONVERTBURN), Page5Proc, "Conversion/CD Burning API");
		tp->AddPage("Metadata",   MAKEINTRESOURCE(IDD_METADATA),    Page6Proc, "Access metadata information and play with advanced title formatting");

		tp->SelectPage(g_starttabidx);

		if (g_autorefresh)
			SetTimer(hwnd, 0, NXSINFO_DEFAULTUPDATEINTERVAL, NULL); //Updates text
		break;
	case WM_DESTROY:
		g_starttabidx = tp->GetSelPage();
		tp->SendApply();
		delete tp;
		break;
	case WM_TIMER:
		/* Post WM_TIMER messages to current page */
		PostMessage(tp->GetCurrentPageHWND(), WM_TIMER, wParam, lParam);
		break;
	case WM_NXSINFOMSG:
		switch (lParam)
		{
		case NXSINFOMSG_SETUPDATEINTERVAL:
			/* if wParam (interval) is zero, then revert to default */
			updateinterval = wParam?wParam:NXSINFO_DEFAULTUPDATEINTERVAL;
			/* If auto updating is enabled then reset the timer */
			if (IsDlgButtonChecked(hwnd, IDC_AUTOREFRESH))
			{
				KillTimer(hwnd, 0);
				SetTimer(hwnd, 0, updateinterval, NULL);
			}
			break;
		}
		break;
	case WM_DISPLAYCHANGE:
		WADlg_close();
		WADlg_init(plugin.hwndParent);
		InvalidateRect(hwnd, NULL, TRUE);
		break;
	case WM_NOTIFY:
		{
			if (int x=tp->HandleNotifications(wParam, lParam))
			{
				SetWindowLong(hwnd, DWL_MSGRESULT, x);
				return TRUE;
			}
			
			break;
		}
	case WM_DRAWITEM:
		{
			/* Owner-draw the tabs of the tab control so it
			   looks like the rest of the dialogs */
			LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
			if (lpdis->CtlID == IDC_TAB1)
			{
				HBRUSH hbr;

				hbr = CreateSolidBrush(WADlg_getColor(WADLG_WNDBG));
				
				FillRect(lpdis->hDC,&lpdis->rcItem, hbr);
				
				SetBkColor(lpdis->hDC, WADlg_getColor(WADLG_WNDBG));
				SetTextColor(lpdis->hDC, WADlg_getColor(WADLG_WNDFG));
				DrawText(lpdis->hDC, tp->Pages(lpdis->itemID).title, -1,
					&lpdis->rcItem, DT_VCENTER|DT_SINGLELINE|DT_CENTER);
				
				DeleteObject(hbr);

				SetWindowLong(hwnd, DWL_MSGRESULT, 0);
				return TRUE;
			}
			break;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_AUTOREFRESHTEXT:
			SendDlgItemMessage(hwnd, IDC_AUTOREFRESH, BM_CLICK, 0, 0);
			break;
		case IDC_AUTOREFRESH:
			g_autorefresh = IsDlgButtonChecked(hwnd, IDC_AUTOREFRESH);
			if (g_autorefresh)
				SetTimer(hwnd, 0, updateinterval, NULL);
			else
				KillTimer(hwnd, 0);
			break;
		case IDC_REFRESHBTN:
			PostMessage(tp->GetCurrentPageHWND(), WM_TIMER, 0, 0);
			break;
		case IDOK:
		case IDCANCEL:
			g_isvisible = FALSE;
			
			WADlg_close();
			KillTimer(hwnd, 0);
			DestroyWindow(cfgews.me);
		}
		break;
		case WM_SIZE:
			{
				HDWP hdwp;
				hdwp = BeginDeferWindowPos(6);
				GetClientRect(hwnd, &dr);
				DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_TAB1), 0, 0, 0, dr.right, dr.bottom - 30, SWP_NOZORDER);
				DeferWindowPos(hdwp, GetDlgItem(hwnd, IDOK), 0, dr.right-70, dr.bottom-25, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
				DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_REFRESHBTN), 0, dr.right-135, dr.bottom-25, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
				DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_AUTOREFRESH), 0, 5, dr.bottom-22, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
				DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_AUTOREFRESHTEXT), 0, 20, dr.bottom-22, 0, 0, SWP_NOZORDER|SWP_NOSIZE);
				EndDeferWindowPos(hdwp);
				tp->AdjustPageSize();
				break;
			}
	}//switch...
	return FALSE;
}


/////////////////
// P A G E   1 //
/////////////////

BOOL CALLBACK Page1Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int a;
	if (a = WADlg_handleDialogMsgs(hwnd, message, wParam, lParam))
	{
		SetWindowLong(hwnd, DWL_MSGRESULT, a);
		return a;
	}
	
	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND re;
			CHARFORMAT chfmt={sizeof(CHARFORMAT),};
			
			re = GetDlgItem(hwnd, IDC_EDIT1);
			SendMessage(re, EM_SETBKGNDCOLOR,
				FALSE, WADlg_getColor(WADLG_ITEMBG));
			
			chfmt.cbSize = sizeof(CHARFORMAT);
			chfmt.dwMask = CFM_COLOR;
			chfmt.crTextColor = WADlg_getColor(WADLG_ITEMFG);
			SendMessage(re, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&chfmt);
			SendMessage(re, EM_SETLIMITTEXT, 65535, 0);
			
			return TRUE;
		}
	case WM_NOTIFY:
		{
			LPNMHDR pnmhdr = (LPNMHDR) lParam;
			//Break loose if not tp is calling...
			if ((NxS_TabPages*)pnmhdr->hwndFrom == tp &&
				pnmhdr->code == PSN_SETACTIVE)
			{
				/* This page needs more frequent WM_TIMER calls */
				PostMessage(hwnd, WM_TIMER, 0, 0);
			}
			break;
		}

	case WM_TIMER:
		if (wParam==0)
		{
			char s[65535];
			char eqs[256];
			char title[MAX_PATH+1];
			char file[MAX_PATH+1];
			char mburl[4096];
			int i;
			char *szStat[4] = {"stopped","playing",NULL,"paused"}; //only indexes 0, 1 & 3 are used.
			char skin[MAX_PATH+1];
			
			char szOutPlug[32];
			lstrcpy(szOutPlug, (char*)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTPLUGIN));
			HWND ds=FindWindowEx(plugin.hwndParent, 0, "DSound_IPC", NULL);
			int ds_state=SendMessage(ds, WM_USER, 0, XFADE_GETSTATE);
			int ds_duration=SendMessage(ds, WM_USER, 0, XFADE_GETDURATION);
			
			int ver=SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETVERSION);
			int stat=SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_ISPLAYING);
			int rep=SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_REPEAT);
			int shuf=SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_SHUFFLE);
			int inetavail=SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_INETAVAILABLE);
			int srate=SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETINFO);
			int bps=SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_GETINFO);
			int nch=SendMessage(plugin.hwndParent, WM_WA_IPC, 2, IPC_GETINFO);
			int plpos=SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS);
			int pllen=SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTLENGTH);
			lstrcpy(title, (LPCTSTR)SendMessage(plugin.hwndParent, WM_WA_IPC, plpos, IPC_GETPLAYLISTTITLE));
			lstrcpy(file, (LPCTSTR)SendMessage(plugin.hwndParent, WM_WA_IPC, plpos, IPC_GETPLAYLISTFILE));
			
			int skininfo=SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETSKININFO);
			
			int vol=SendMessage(plugin.hwndParent, WM_WA_IPC, -666, IPC_SETVOLUME);
			int pan=SendMessage(plugin.hwndParent, WM_WA_IPC, -666, IPC_SETPANNING);
			int songpos=SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETOUTPUTTIME);
			int songlen=SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_GETOUTPUTTIME);
			
			int eqen=SendMessage(plugin.hwndParent, WM_WA_IPC, 11, IPC_GETEQDATA);
			int eqal=SendMessage(plugin.hwndParent, WM_WA_IPC, 12, IPC_GETEQDATA);
			int eqpa=SendMessage(plugin.hwndParent, WM_WA_IPC, 10, IPC_GETEQDATA);
			
			lstrcpy(eqs, "EQ Bands: ");
			for (i=0;i<10;i++)
			{
				wsprintf(eqs+lstrlen(eqs), "%d,", SendMessage(plugin.hwndParent, WM_WA_IPC, i, IPC_GETEQDATA));
			}
			eqs[lstrlen(eqs)-1] = 0; // remove last comma
			
			if (ver < 0x4901)
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)mburl, IPC_GETMBURL);
			else
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)mburl, IPC_MBGETCURURL);
			
			
			char *ini=(char*)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETINIFILE);
			SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)skin, IPC_GETSKIN);

			int isdoublesize = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_ISDOUBLESIZE);
			int isvisrunning = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_ISVISRUNNING);
			int dispmode = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETTIMEDISPLAYMODE);
			

			
			// One huge and messy wsprintf() call.
			wsprintf(s,
				"Winamp version: 0x%x\r\n"
				"Winamp is %s.\r\n"
				"Winamp INI: \"%s\"\r\n"
				"Skin: \"%s\"\r\n"
				"Skin is a %s skin.\r\n"
				"Repeat=%d; Shuffle=%d\r\n"
				"Internet %s available.\r\n"
				"Samplerate: %dkHz; Bitrate: %d bps; Channels: %d\r\n"
				"Song %d of %d; Song pos: %d ms; Song len: %d sec(s)\r\n"
				"Song title: \"%s\"\r\n"
				"Song file: \"%s\"\r\n"
				"Volume=%d (%d%%); Panning=%d (%d%%); EQ is %s\r\n"
				"EQ Autload is %s. Pre-amp: %d\r\n"
				"%s\r\n"
				"Output plug-in detected: \"%s\"\r\n"
				"DirectSound: Crossfade=%d; Fade duration=%d ms\r\n"
				"Current Minibrowser URL: \"%s\"\r\n"
				"Doublesize: %s\r\n"
				"Visualization plug-in is %s\r\n"
				"Display mode: %s\r\n"
				,
				ver,
				szStat[stat],
				ini,
				skin,
				(skininfo==1?"classic":"modern"),
				rep, shuf,
				inetavail?"is":"is not",
				srate, bps, nch,
				plpos+1, pllen, songpos, songlen,
				title,
				file,
				vol, vol*100/255, pan, pan*100/127, eqen?"enabled":"disabled",
				eqal?"on":"off", eqpa,
				eqs,
				szOutPlug,
				ds_state, ds_duration,
				mburl,
				isdoublesize?"on":"off",
				isvisrunning?"running":"not running",
				dispmode?"remaining":"elapsed"
				);
			
			// Set text to edit box
			SetDlgItemText(hwnd, IDC_EDIT1, s);
			UpdateWindow(GetDlgItem(hwnd, IDC_EDIT1));
			InvalidateRect(GetDlgItem(hwnd, IDC_EDIT1), NULL, TRUE);
			
			break;
		}
	case WM_SIZE:
		{
			RECT dr;
			GetClientRect(hwnd, &dr);
			SetWindowPos(GetDlgItem(hwnd, IDC_EDIT1), 0, 5, 25, dr.right-10, dr.bottom-25-5, SWP_NOZORDER|SWP_DRAWFRAME);
			break;
		}
	}//switch...
	return FALSE;
}

/////////////////
// P A G E   2 //
/////////////////

BOOL CALLBACK Page2Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char* (*export_sa_get)(void);
	static void (*export_sa_setreq)(int want);
	static HBITMAP hbm;
	static HBITMAP oldbm;
	static HDC memDC;
	static char sapeaks[150];
	static char safalloff[150];
	static char sapeaksdec[150];

	
	int a;
	a = WADlg_handleDialogMsgs(hwnd, message, wParam, lParam);
	if (a)
	{
		SetWindowLong(hwnd, DWL_MSGRESULT, a);
		return a;
	}
	
	switch (message)
	{
	case WM_INITDIALOG:
		{
			HDC tmpdc;
			RECT r;
			
			/* Get function pointers from Winamp */
			export_sa_get = (char* (*)(void))SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETSADATAFUNC);
			export_sa_setreq = (void (*)(int))SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_GETSADATAFUNC);
			
			/* Create a memory DC to be BitBlt'ed to IDC_SAFRAME in
			WM_DRAWITEM and WM_TIMER. */
			GetClientRect(GetDlgItem(hwnd, IDC_SAFRAME), &r);
			tmpdc = GetDC(hwnd);
			hbm = CreateCompatibleBitmap(tmpdc, (r.right-r.left), (r.bottom-r.top));
			memDC = CreateCompatibleDC(tmpdc);
			oldbm = (HBITMAP)SelectObject(memDC, hbm);
			ReleaseDC(hwnd, tmpdc);
			SetWindowLong(GetDlgItem(hwnd, IDC_SAFRAME), GWL_STYLE, SS_OWNERDRAW|WS_CHILD|WS_VISIBLE);
			
			
			return TRUE;
		}
	case WM_NOTIFY:
		{
			LPNMHDR pnmhdr = (LPNMHDR) lParam;
			//Break loose if not tp is calling...
			if ((NxS_TabPages*)pnmhdr->hwndFrom != tp) break;
			if (pnmhdr->code == PSN_SETACTIVE)
			{
				/* This page needs more frequent WM_TIMER calls */
				PostMessage(GetParent(hwnd), WM_NXSINFOMSG, 25, NXSINFOMSG_SETUPDATEINTERVAL);			
				PostMessage(hwnd, WM_TIMER, 0, 0);
			}
			if (pnmhdr->code == PSN_KILLACTIVE)
			{
				/*Restore old interval*/
				PostMessage(GetParent(hwnd), WM_NXSINFOMSG, 0, NXSINFOMSG_SETUPDATEINTERVAL);
			}
			break;
		}
	case WM_TIMER: /* WM_TIMER message posted from owner dialog */
		{
			
			HWND hwndSA;
			char *sadata;
			RECT r;
			int x;
			char szDigits[128];

			hwndSA = GetDlgItem(hwnd, IDC_SAFRAME);

			GetClientRect(hwndSA, &r);
			r.right -= r.left;
			r.bottom -= r.top;

			// Specify, that we want both spectrum and oscilloscope data
			export_sa_setreq(1); // Pass 0 (zero) and get spectrum data only
			sadata = export_sa_get();
			
			szDigits[0]=0;
			for (x=0; x<40; x++)
			{
				wsprintf(szDigits+lstrlen(szDigits), "%ld:", sadata[x]);
			}
			SendDlgItemMessage(hwnd, IDC_SADATAEDIT, WM_SETTEXT, 0, (LPARAM)szDigits);

			// clear background
			HBRUSH hBr, hOldBr;
			HPEN hPn, hOldPn;

			hBr = CreateSolidBrush(WADlg_getColor(WADLG_ITEMBG));
			hOldBr = (HBRUSH)SelectObject(memDC, hBr);
			hPn = CreatePen(PS_SOLID, 0, WADlg_getColor(WADLG_ITEMFG));
			hOldPn = (HPEN)SelectObject(memDC, hPn);
			Rectangle(memDC, 0, 0, r.right, r.bottom);

			if (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_ISPLAYING) >= 1)
			{
				// draw simple analyser...
				for (x = 0; x < 75; x ++)
				{
					// Fix peaks & falloff
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


					MoveToEx(memDC, x, r.bottom, NULL);
					LineTo(memDC, x, r.bottom - safalloff[x]);

					SetPixel(memDC, x, r.bottom - sapeaks[x], WADlg_getColor(WADLG_ITEMFG));
				}
				// ...and a solid oscilloscope
				for (x = 0; x < 75; x ++)
				{
					MoveToEx(memDC,85+x, (r.bottom >> 1), NULL);	
					LineTo(memDC, 85+x, (r.bottom >> 1) - sadata[75+x]);
				}
			}
			SelectObject(memDC, hOldBr);
			DeleteObject(hBr);
			SelectObject(memDC, hOldPn);
			DeleteObject(hPn);

			// update frame
			{
				HDC hdc = GetDC(hwndSA);
				BitBlt(hdc, 0, 0,
					r.right-r.left,
					r.bottom-r.top, memDC, 0, 0, SRCCOPY);
				ReleaseDC(hwndSA, hdc);
			}

			
			// update time display
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
			SendDlgItemMessage(hwnd, IDC_TIMEDISPLAY, WM_SETTEXT, 0, (LPARAM)szTime);
			
			return FALSE;
		}
	case WM_PAINT:
		{
			int tab[3] = {IDC_SADATAEDIT|DCW_SUNKENBORDER, IDC_FRAME1|DCW_SUNKENBORDER,
				IDC_TIMEDISPLAY|DCW_SUNKENBORDER};
			WADlg_DrawChildWindowBorders(hwnd, tab, 3);
			return FALSE;
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
	}
	
	return FALSE;
}


/////////////////
// P A G E   3 //
/////////////////


int embedenums(embedWindowState *ws, struct embedEnumStruct *param)
{
	char *title;
	DWORD titlelen;
	titlelen = GetWindowTextLength(ws->me)+1;
	title = (char*)LocalAlloc(LPTR, titlelen);
	GetWindowText(ws->me, title, titlelen);
	SendDlgItemMessage(tp->GetCurrentPageHWND(), IDC_EMBEDLIST, LB_ADDSTRING, 0, (LPARAM)title);
	return 0; // return 1 to abort
}


BOOL CALLBACK Page3Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int a;
	a = WADlg_handleDialogMsgs(hwnd, message, wParam, lParam);
	if (a)
	{
		SetWindowLong(hwnd, DWL_MSGRESULT, a);
		return a;
	}
	
	switch (message)
	{
	case WM_TIMER:
		break;
	case WM_NOTIFY:
		{
			LPNMHDR pnmhdr = (LPNMHDR) lParam;
			//Break loose if not tp is calling...
			if ((NxS_TabPages*)pnmhdr->hwndFrom != tp) break;
			if (pnmhdr->code == PSN_SETACTIVE)
			{
				// This page needs more frequent WM_TIMER calls
				PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_LISTEMBEDWNDS, BN_CLICKED), 0);
				PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_LISTEXTS, BN_CLICKED), 0);
			}
			if (pnmhdr->code == PSN_KILLACTIVE)
			{
				// Restore old interval
				PostMessage(GetParent(hwnd), WM_NXSINFOMSG, 0, NXSINFOMSG_SETUPDATEINTERVAL);
			}
			break;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_LISTEMBEDWNDS:
			{
				embedEnumStruct embedenum;
				
				ZeroMemory(&embedenum, sizeof(embedEnumStruct));
				embedenum.enumProc = embedenums;
				
				SendDlgItemMessage(hwnd, IDC_EMBEDLIST, LB_RESETCONTENT, 0, 0);
				
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&embedenum, IPC_EMBED_ENUM);
				break;
			}
		case IDC_LISTEXTS:
			{
				// Variables required by Extensions list:
				void *exts;
				char *current_string;
				//char ext_item[4096];
				char ext_desc[1024];
				char ext_specs[1024];
				char *p1, *p2;

				SendDlgItemMessage(hwnd, IDC_EXTSLIST, LB_RESETCONTENT, 0, 0);
				exts=(void*)SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_GET_EXTLIST);
				
				/* Since exts is a null delimited double null terminated list,
				we have to split it up and add it to the listbox as we go: */
				for (current_string = (char*)exts; *current_string; current_string++)
				{
					p1 = ext_desc;
					while (*current_string)
						*p1++ = *current_string++;
					*p1++ = 0;
					
					SendDlgItemMessage(hwnd, IDC_EXTSLIST, LB_ADDSTRING, 0, (LPARAM)ext_desc);
					current_string++;
					
					p2 = ext_specs;
					while (*current_string)
						*p2++ = *current_string++;
					*p2++ = 0;
					
					SendDlgItemMessage(hwnd, IDC_EXTSLIST, LB_ADDSTRING, 0, (LPARAM)ext_specs);
				}

			}
		}
		break;
		
		case WM_PAINT:
			{
				int tab[2] = {IDC_EMBEDLIST|DCW_SUNKENBORDER, IDC_EXTSLIST|DCW_SUNKENBORDER};
				WADlg_DrawChildWindowBorders(hwnd, tab, 2);
				return FALSE;
			}
	}
	
	return FALSE;
}


/////////////////
// P A G E   4 //
/////////////////


WNDPROC oldWAProc;
LRESULT CALLBACK WASubclass(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT res=CallWindowProc(oldWAProc, hwnd, message, wParam, lParam);

	if (message==WM_USER && lParam==IPC_HOOK_OKTOQUIT)
	{
		int shit=MessageBox(hwnd, "Do you wish to quit?", "Winamp 5", MB_ICONQUESTION|MB_YESNO);
		return (shit==IDYES?1:0);
	} else if (message==WM_WA_IPC && lParam==IPC_CB_MISC) {
		char s[32];
		switch (wParam)
		{
		case IPC_CB_MISC_VOLUME:
			wsprintf(s, "Vol: %d (%d%%)",
				CallWindowProc(oldWAProc, hwnd, WM_WA_IPC, -666, IPC_SETVOLUME),
				(CallWindowProc(oldWAProc, hwnd, WM_WA_IPC, -666, IPC_SETVOLUME)*100)/255);
			SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, s);
			break;
		case IPC_CB_MISC_EQ:
			SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, "Equalizer changed!");
			break;
		case IPC_CB_MISC_STATUS:
			SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, "State changed!");
			break;
		case IPC_CB_MISC_TITLE:
			SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, "Title changed!");
			break;
		case IPC_CB_MISC_VIDEOINFO:
			SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, "Video info changed!");
			break;
		case IPC_CB_MISC_INFO:
			SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, "Info changed!");
			break;
		}
	} else if (message==WM_WA_IPC && lParam==IPC_HOOK_TITLES) {
		LRESULT lres=CallWindowProc(oldWAProc, hwnd, message, wParam, lParam);
		/*
		char s[4096];
		waHookTitleStruct *phts = (waHookTitleStruct *)wParam;
		wsprintf(s, "Title: \"%s\"", phts->title);
		SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, s);
		*/
		return lres;
	} else if (message==WM_WA_IPC && lParam==IPC_CB_ONSHOWWND) {
		char *s;
		switch (wParam)
		{
		case IPC_CB_WND_EQ: s = "Show: Equalizer"; break;
		case IPC_CB_WND_PE: s = "Show: Playlist Editor"; break;
		case IPC_CB_WND_MB: s = "Show: Minibrowser"; break;
		case IPC_CB_WND_VIDEO: s = "Show: Video window"; break;
		case IPC_CB_WND_MAIN: s = "Show: Main window"; break;
		default: s = "Show: Unknown window";
		}
		SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, s);
	} else if (message==WM_WA_IPC && lParam==IPC_CB_ONHIDEWND) {
		char *s;
		switch (wParam)
		{
		case IPC_CB_WND_EQ: s = "Hide: Equalizer"; break;
		case IPC_CB_WND_PE: s = "Hide: Playlist Editor"; break;
		case IPC_CB_WND_MB: s = "Hide: Minibrowser"; break;
		case IPC_CB_WND_VIDEO: s = "Hide: Video window"; break;
		case IPC_CB_WND_MAIN: s = "Hide: Main window"; break;
		default: s = "Hide: Unknown window";
		}
		SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, s);
	} else if (message==WM_WA_IPC && lParam==IPC_FILE_TAG_MAY_HAVE_UPDATED) {
		SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, "IPC_FILE_TAG_MAY_HAVE_UPDATED");
	} else if (message==WM_WA_IPC && lParam==IPC_PLAYLIST_MODIFIED) {
		SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, "IPC_PLAYLIST_MODIFIED");
	} else if (message==WM_WA_IPC && lParam==IPC_CB_OUTPUTCHANGED) {
		SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, "IPC_CB_OUTPUTCHANGED");
	} else if (message==WM_WA_IPC && lParam==IPC_CB_ONTOGGLEAOT) {
		SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, "IPC_CB_ONTOGGLEAOT");
	} else if (message==WM_WA_IPC && lParam==IPC_PLAYING_FILE) {
		char s[256];
		wsprintf(s, "IPC_PLAYING_FILE (%s)", (char*)wParam);
		SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, s);
	} else if (message==WM_WA_IPC && lParam==IPC_CB_PEINFOTEXT) {
		char s[256];
		wsprintf(s, "IPC_CB_PEINFOTEXT (%s)", (char*)wParam);
		SetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDIT1, s);
	}
	return res;
}

IVideoOutput *g_video=0;

class IMyTrackSelector: public ITrackSelector
{
protected:
	int m_videotrack;
public:
	IMyTrackSelector(): m_videotrack(0) {}
    virtual int getNumAudioTracks() { return 1; };
    virtual void enumAudioTrackName(int n, const char *buf, int size) {
		switch (n)
		{
			case 0: lstrcpyn((char*)buf, "First audio track", size); break;
		}
	}
    virtual int getCurAudioTrack() { return 0; };
    virtual int getNumVideoTracks() { return 2; };
    virtual void enumVideoTrackName(int n, const char *buf, int size) {
		switch (n)
		{
			case 0: lstrcpyn((char*)buf, "Voiceprint", size); break;
			case 1: lstrcpyn((char*)buf, "Waveform", size); break;
		}
	}
    virtual int getCurVideoTrack() { return m_videotrack; };

    virtual void setAudioTrack(int n) { }
    virtual void setVideoTrack(int n) { m_videotrack = n; }
};

IMyTrackSelector MyTrackSelector;

unsigned char* vidbufdec=0;
int m_w=320;
int m_h=240;
int specpos=0;


BOOL CALLBACK Page4Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char* (*export_sa_get)(void);
	static void (*export_sa_setreq)(int want);
	static BOOL isWinampDisabled=FALSE;

	int a = WADlg_handleDialogMsgs(hwnd, message, wParam, lParam);
	if (a)
	{
		SetWindowLong(hwnd, DWL_MSGRESULT, a);
		return a;
	}
	

	switch (message)
	{
	case WM_INITDIALOG:
		/* Get function pointers from Winamp */
		export_sa_get = (char* (*)(void))SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETSADATAFUNC);
		export_sa_setreq = (void (*)(int))SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_GETSADATAFUNC);
		break;
	case WM_NOTIFY:
		{
			LPNMHDR pnmhdr = (LPNMHDR) lParam;
			//Break loose if not tp is calling...
			if ((NxS_TabPages*)pnmhdr->hwndFrom != tp) break;
			if (pnmhdr->code == PSN_SETACTIVE)
			{
				oldWAProc = (WNDPROC)SetWindowLong(plugin.hwndParent, GWL_WNDPROC, (LONG)WASubclass);
			}
			if (pnmhdr->code == PSN_KILLACTIVE)
			{
				SetWindowLong(plugin.hwndParent, GWL_WNDPROC, (LONG)oldWAProc);
			}
			if (pnmhdr->code == PSN_RESET)
			{
				SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_ENABLEDISABLE_ALL_WINDOWS);
				isWinampDisabled=FALSE;
			}
			break;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_GETURL:
			{
				char *url=(char*)SendMessage(plugin.hwndParent, WM_WA_IPC,
					(WPARAM)hwnd, IPC_OPENURLBOX);
				SetDlgItemText(hwnd, IDC_GETRES, url);
				GlobalFree((HGLOBAL)url);
				break;
			}
		case IDC_GETREGVER:
			SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETREGISTEREDVERSION);
			break;
		case IDC_SHOWNOTIFICATION:
			SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_SHOW_NOTIFICATION);
			break;
		case IDC_BUTTON1:
			{
				windowCommand wc;
				RECT r;
				GetWindowRect(GetDlgItem(hwnd, IDC_BUTTON1), &r);
				wc.cmd = PLCMD_LIST;
				wc.x = r.left;
				wc.y = r.top;
				wc.align = 0;
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&wc, IPC_PLCMD);
				break;
			}
		case IDC_RESTARTWA:
			SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_RESTARTWINAMP);
			break;
			
		case IDC_EN_DIS_WA:
			SendMessage(plugin.hwndParent, WM_WA_IPC, (isWinampDisabled?0:0xdeadbeef), IPC_ENABLEDISABLE_ALL_WINDOWS);
			SetDlgItemText(hwnd, IDC_EN_DIS_WA, isWinampDisabled?"Disable Winamp":"Enable Winamp");
			isWinampDisabled = !isWinampDisabled;
			break;
		case IDC_SHOWPAGES:
			{
				char pages[4096];
				prefsDlgRec *curpage;
				curpage = prefs;
				lstrcpy(pages, "Pages:\r\n");
				while (curpage)
				{
					lstrcat(pages, curpage->name);
					lstrcat(pages, "\r\n");
					curpage = curpage->next;
				}
				MessageBox(hwnd, pages, "Pages", 64);
			}
			break;
		case IDC_OPENVIDEO:
			{
				g_video = (IVideoOutput *)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_IVIDEOOUTPUT);
				if (!g_video)
				{
					MessageBox(hwnd, "Error getting IVideoOutput interface!", "Video", MB_OK|MB_ICONERROR);
					break;
				}

				// Set up bare-bones video

				// Allocate video buffer
				vidbufdec=(unsigned char*)GlobalAlloc(GPTR, sizeof(YV12_PLANES) + m_w*m_h*3/2);

				g_video->extended(VIDUSER_SET_TRACKSELINTERFACE, (int)&MyTrackSelector, 0);
				g_video->showStatusMsg("Video from NxS Info Tool!");
				g_video->open(m_w, m_h, 1, 1.0, VIDEO_MAKETYPE('Y','V','1','2'));

				/* Normally you would use a thread to decode video and send it to Winamp,
				   but here we just use a timer. */
				SetTimer(hwnd, 1, 25, NULL);

			}
			break;
		case IDC_CLOSEVIDEO:
			KillTimer(hwnd, 1);
			if (g_video) g_video->close();
			GlobalFree((HGLOBAL)vidbufdec);
			break;
		case IDC_SETPLCOLORS:
			{
				waSetPlColorsStruct pc;
				int e[6]={0x000000FF,0x0000FF00,0x0000FFFF,0x00FF0000,0x00FF00FF,0x00FFFF00};
				pc.elems = e;
				pc.numElems = 6;
				pc.bm = 0;
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&pc, IPC_SETPLEDITCOLORS);
			}
			break;
		}
		break;
		case WM_TIMER:
			if (wParam==0)
			{
				/* Variables required by Playlist related stuff */
				COPYDATASTRUCT cds;
				callbackinfo ci;
				HWND pe;
				int plpos, pllen;
				char s[32];
				POINT pt;
				
				pe = (HWND)SendMessage(plugin.hwndParent, WM_WA_IPC, IPC_GETWND_PE, IPC_GETWND);
				GetCursorPos(&pt);
				ScreenToClient(pe, &pt);
				plpos = SendMessage(pe, WM_USER, IPC_PE_GETIDXFROMPOINT, (LPARAM)&pt);
				pllen = SendMessage(pe, WM_USER, IPC_PE_GETINDEXTOTAL, 0);
				
				SetDlgItemInt(hwnd, IDC_SELPOS,
					SendMessage(pe, WM_USER,
					GetDlgItemInt(hwnd, IDC_SELTITLE, NULL, FALSE), 0), FALSE);
				
				if (plpos<pllen)
				{
					ci.callback = hwnd;
					ci.index = plpos;
					cds.dwData = IPC_PE_GETINDEXINFO;
					cds.cbData = sizeof(callbackinfo);
					cds.lpData = (void*)&ci;
					SendMessage(pe, WM_COPYDATA, (WPARAM)hwnd, (LPARAM)&cds);
				} else
					SetDlgItemText(hwnd, IDC_PLDATA, "N/A");
				
				wsprintf(s, "%ld/%ld", plpos+1, pllen);
				SetDlgItemText(hwnd, IDC_PLPOS, s);
			} else if (wParam==1)
			{
				char *sadata;

				// Specify, that we want both spectrum and oscilloscope data
				export_sa_setreq(1);
				sadata = export_sa_get();

				int x,y;
				if (MyTrackSelector.getCurVideoTrack() == 0) {
					// Draw a moving bar
					for (x=0;x<m_h;x++) {
						y=sadata[(x*75)/m_h]*3; // scale it (sqrt to make low values more visible)
						if (y>255) y=255; // cap it
						vidbufdec[x*m_w+specpos]=y; // plot it
					}

					// move marker onto next position
					specpos=(specpos+1) % m_w;
					for (x=0;x<m_h;x++)	vidbufdec[x*m_w+specpos]=255;
				} else if (MyTrackSelector.getCurVideoTrack() == 1) {
					for (x=0;x<m_w;x++) {
						y=sadata[(75+(x*75))/(m_h/2)]*3; // scale it (sqrt to make low values more visible)
						if (y>255) y=255; // cap it
						vidbufdec[(m_h/2)+m_w*x]=y; // plot it
					}
				}

				YV12_PLANES *image_vbd=(YV12_PLANES *)vidbufdec;
				image_vbd->y.baseAddr=(unsigned char *)(image_vbd+1);
				image_vbd->v.baseAddr=((unsigned char *)(image_vbd+1)) + m_w*m_h;
				image_vbd->u.baseAddr=((unsigned char *)(image_vbd+1)) + m_w*m_h*5/4;
				image_vbd->y.rowBytes=m_w;
				image_vbd->v.rowBytes=m_w/2;
				image_vbd->u.rowBytes=m_w/2;
				
				// Send to screen
				g_video->draw(vidbufdec);

				// Update video info string
				char szInfo[256];
				wsprintf(szInfo, "Dummy video (bar pos: %d)", specpos);
				g_video->extended(VIDUSER_SET_INFOSTRING, (int)szInfo, 0);
			}
			break;
		case WM_COPYDATA:
			{
				COPYDATASTRUCT *lpcds = (COPYDATASTRUCT*)lParam;
				if (lpcds->dwData == IPC_PE_GETINDEXINFORESULT)
				{
					fileinfo *fi = (fileinfo *)lpcds->lpData;
					SetDlgItemText(hwnd, IDC_PLDATA, fi->file);
				}
				break;
			}
		case WM_PAINT:
			{
				int tab[7] = {IDC_GETRES|DCW_SUNKENBORDER, IDC_FRAME1|DCW_SUNKENBORDER,
					IDC_PLPOS|DCW_SUNKENBORDER, IDC_PLDATA|DCW_SUNKENBORDER,
					IDC_SELPOS|DCW_SUNKENBORDER, IDC_SELTITLE|DCW_SUNKENBORDER,
					IDC_EDIT1|DCW_SUNKENBORDER};
				WADlg_DrawChildWindowBorders(hwnd, tab, 7);
				return FALSE;
			}
	}
	return FALSE;
}


/////////////////
// P A G E   5 //
/////////////////


void converterenum(int user_data, const char *desc, int fourcc)
{
	HWND lb;
	char s[256];
	int idx;

	lb = GetDlgItem((HWND)user_data, IDC_FMTLIST);
	wsprintf(s, "%s \t(%c%c%c%c)", desc,
		((char*)&fourcc)[0],
		((char*)&fourcc)[1],
		((char*)&fourcc)[2],
		((char*)&fourcc)[3]);
	idx = SendMessage(lb, LB_ADDSTRING, 0, (LPARAM)s);
	SendMessage(lb, LB_SETITEMDATA, idx, (LPARAM)fourcc);
	
	
}

WNDPROC OldConvCfgProc;
LRESULT CALLBACK ConvCfgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int a = WADlg_handleDialogMsgs(hwnd, message, wParam, lParam);

	if (message==WM_DESTROY)
	{
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG)OldConvCfgProc);
	} else if (a) return a;

	return CallWindowProc(OldConvCfgProc, hwnd, message, wParam, lParam);
}


BOOL CALLBACK MakeOwnerdrawButtons_EnumChildProc(HWND hwnd, LPARAM lParam)
{
	char szClass[64];
	GetClassName(hwnd, szClass, sizeof(szClass));
	if (!lstrcmpi(szClass, "BUTTON"))
	{
		LONG style=GetWindowLong(hwnd, GWL_STYLE);
		if ( !(style & BS_3STATE) && !(style & BS_CHECKBOX) && !(style & BS_GROUPBOX) )
			SetWindowLong(hwnd, GWL_STYLE, (style & ~BS_PUSHBUTTON) | BS_OWNERDRAW);
	}
	return TRUE;
}
 

BOOL CALLBACK Page5Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int a;
	a = WADlg_handleDialogMsgs(hwnd, message, wParam, lParam);
	if (a)
	{
		SetWindowLong(hwnd, DWL_MSGRESULT, a);
		return a;
	}
	
	static convertConfigStruct ccs;
	static convertFileStruct g_cfs;
	
	
	switch (message)
	{
	case WM_INITDIALOG:
		SendDlgItemMessage(hwnd, IDC_SOURCEFILE, WM_SETTEXT, 0, (LPARAM)"cda://E,1");
		SendDlgItemMessage(hwnd, IDC_DESTFILE, WM_SETTEXT, 0, (LPARAM)"c:\\music\\track01.mp3");
		break;
	case WM_NOTIFY:
		{
			LPNMHDR pnmhdr = (LPNMHDR) lParam;
			//Break loose if not tp is calling...
			if ((NxS_TabPages*)pnmhdr->hwndFrom != tp) break;
			if (pnmhdr->code == PSN_SETACTIVE)
			{
				PostMessage(hwnd, WM_TIMER, 0, 0);
				PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_LISTFMTS, BN_CLICKED), 0);

			}
			if (pnmhdr->code == PSN_KILLACTIVE)
			{
			}
			break;
		}

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_LISTFMTS:
			{
				converterEnumFmtStruct cnvenum;
				
				cnvenum.enumProc = converterenum;
				cnvenum.user_data = (int)hwnd;
				
				SendDlgItemMessage(hwnd, IDC_FMTLIST, LB_RESETCONTENT, 0, 0);
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&cnvenum, IPC_CONVERT_CONFIG_ENUMFMTS);
				break;
			}
		case IDC_FMTLIST:
			if (HIWORD(wParam)==LBN_SELCHANGE)
			{
				int idx = SendDlgItemMessage(hwnd, IDC_FMTLIST, LB_GETCURSEL, 0, 0);
				if (idx<0) break;

				RECT r;

				//Kill off old config shit, if any...
				if (IsWindow(ccs.hwndConfig))
					SendMessage(plugin.hwndParent, WM_WA_IPC,
					(WPARAM)&ccs, IPC_CONVERT_CONFIG_END);

				GetWindowRect(GetDlgItem(hwnd, IDC_CFGRECT), &r);
				ScreenToClient(hwnd, (LPPOINT)&r);

				ZeroMemory(&ccs, sizeof(convertConfigStruct));
				ccs.format = SendDlgItemMessage(hwnd, IDC_FMTLIST, LB_GETITEMDATA, idx, 0);
				ccs.hwndParent = hwnd;
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&ccs, IPC_CONVERT_CONFIG);

				SetWindowPos(ccs.hwndConfig, 0, r.left, r.top, 0, 0, SWP_NOZORDER|SWP_NOSIZE);

				//We need to subclass the config dialog, so we can "skin" it.
				OldConvCfgProc = (WNDPROC)SetWindowLong(ccs.hwndConfig, GWL_WNDPROC,
					(LONG)ConvCfgProc);
				//Also enumerate all it's controls and make buttons BS_OWNERDRAW
				EnumChildWindows(ccs.hwndConfig, MakeOwnerdrawButtons_EnumChildProc,0);
				ShowWindow(ccs.hwndConfig, SW_SHOWNA);
			}
			break;
		case IDC_STARTCONV:
			{
				int idx=SendDlgItemMessage(hwnd, IDC_FMTLIST, LB_GETCURSEL, 0, 0);
				if (idx<0) break;

				if (g_cfs.callbackhwnd)
				{
					SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&g_cfs, IPC_CONVERTFILE_END);
					g_cfs.callbackhwnd = NULL;
					SetDlgItemText(hwnd, IDC_STARTCONV, "Convert");
					return TRUE;
				}

				ZeroMemory(&g_cfs, sizeof(convertFileStruct));
				g_cfs.callbackhwnd = hwnd;
				g_cfs.sourcefile = (char*)GlobalAlloc(GPTR, MAX_PATH+1);
				g_cfs.destfile = (char*)GlobalAlloc(GPTR, MAX_PATH+1);
				
				GetDlgItemText(hwnd, IDC_SOURCEFILE, g_cfs.sourcefile, MAX_PATH);
				GetDlgItemText(hwnd, IDC_DESTFILE, g_cfs.destfile, MAX_PATH);
	
				ZeroMemory(&g_cfs.destformat, sizeof(g_cfs.destformat));
				g_cfs.destformat[0]=SendDlgItemMessage(hwnd, IDC_FMTLIST, LB_GETITEMDATA, idx, 0);
				g_cfs.destformat[1]=44100;
				g_cfs.destformat[2]=2;
				g_cfs.destformat[3]=16;

				
				if (0==SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&g_cfs, IPC_CONVERTFILE))
				{
					SetDlgItemText(hwnd, IDC_STATUS, g_cfs.error);
					break;
				}
				SetDlgItemText(hwnd, IDC_STARTCONV, "Abort conversion");
			}
			break;
		case IDC_BURNCD:
			{
				burnCDStruct bcs;
				char s[MAX_PATH];
				char *p;
				GetModuleFileName(NULL, s, sizeof(s));
				p = s+lstrlen(s);
				while (p >= s && *p != '\\') p--;
				lstrcpy(p+1, "winamp.m3u");
				
				if (MessageBox(hwnd, s, "Do you wish to burn?", MB_YESNO)==IDNO)break;
				
				SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_WRITEPLAYLIST);
				bcs.callback_hwnd = hwnd;
				bcs.playlist_file = s;
				bcs.cdletter = 'F';
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&bcs, IPC_BURN_CD);
				break;
			}
		}
		case WM_USER:
			{
				if (lParam==IPC_CB_CONVERT_STATUS)
				{
					TCHAR szText[256];
					wsprintf(szText,
						"Converting: %d%% done\r\n"
						"Encoder = %c%c%c%c\r\n"
						"Bytes done = %d\r\n"
						"Bytes total = %d\r\n"
						"Bytes out = %d",
						wParam,
						((char*)&g_cfs.destformat[0])[0],
						((char*)&g_cfs.destformat[0])[1],
						((char*)&g_cfs.destformat[0])[2],
						((char*)&g_cfs.destformat[0])[3],
						g_cfs.bytes_done, g_cfs.bytes_total, g_cfs.bytes_out);
					SetDlgItemText(hwnd, IDC_STATUS, szText);
				}
				else if (lParam==IPC_CB_CONVERT_DONE)
				{
					SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&g_cfs, IPC_CONVERTFILE_END);
					g_cfs.callbackhwnd = 0;
					SetDlgItemText(hwnd, IDC_STARTCONV, "Convert");
					SetDlgItemText(hwnd, IDC_STATUS, "Conversion done!");
				}
				break;
			}
			break;
		case WM_PAINT:
			{
				int tab[4] = {IDC_FMTLIST|DCW_SUNKENBORDER, IDC_SOURCEFILE|DCW_SUNKENBORDER,
					IDC_DESTFILE|DCW_SUNKENBORDER, IDC_STATUS|DCW_SUNKENBORDER};
				WADlg_DrawChildWindowBorders(hwnd, tab, 4);
				return FALSE;
			}
	}
	
	return FALSE;
}

/////////////////
// P A G E   6 //
/////////////////

char* myTagFunc(char * tag, void * p)
{
	char *s;
	char curfile[MAX_PATH+1];
	char retbuf[256];
	extendedFileInfoStruct efi;
	
	GetDlgItemText(tp->GetCurrentPageHWND(), IDC_EDITFILENAME, curfile, sizeof(curfile));
	
	if (!lstrcmpi(tag, "length"))
	{
		basicFileInfoStruct bfi;
		bfi.filename = curfile;
		bfi.quickCheck = 0;
		bfi.title = 0;
		SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&bfi, IPC_GET_BASIC_FILE_INFO);
		s = (char*)GlobalAlloc(GPTR, 256);
		if (bfi.length >= 3600)
			wsprintf(s, "%d:%.2d:%.2d", bfi.length / 60 / 60, (bfi.length / 60) % 60, bfi.length % 60);
		else
			wsprintf(s, "%d:%.2d", bfi.length / 60, bfi.length % 60);
		return s;
	}
	
	// Use IPC_GET_EXTENDED_FILE_INFO to get the metadata.
	ZeroMemory(retbuf, sizeof(retbuf)-1);
	efi.filename = curfile;
	efi.metadata = tag;
	efi.ret = retbuf;
	efi.retlen = sizeof(retbuf)-1;

	if (SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_GET_EXTENDED_FILE_INFO)
		&& lstrlen(retbuf))
	{
		s = (char*)GlobalAlloc(GPTR, 256);
		lstrcpy(s, retbuf);
		return s;
	}
	
	/* Or you can test it with static data:
	
	  if (!lstrcmpi(tag, "title")) lstrcpy(s, "Da funky trakk");
	  if (!lstrcmpi(tag, "artist")) lstrcpy(s, "Saivert");
	  if (!lstrcmpi(tag, "album")) lstrcpy(s, "The lengty EP");
	  if (!lstrcmpi(tag, "tracknumber")) lstrcpy(s, "3");
	  if (!lstrcmpi(tag, "streamtitle")) return 0;
	  
		return s; //Return 0 if not found
	*/
	return 0;
}

void myTagFreeFunc(char * tag,void * p)
{
	GlobalFree((HGLOBAL) tag);
}

char *GetFormattedTitleFromWinamp(char *fmtspec)
{
	char *temp;
	waFormatTitle fmt_title;
	char s[4096];

	fmt_title.spec = fmtspec;
	fmt_title.TAGFUNC = myTagFunc;
	fmt_title.TAGFREEFUNC = myTagFreeFunc;
	fmt_title.out = s;
	fmt_title.out_len = sizeof(s)-1;
	SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&fmt_title, IPC_FORMAT_TITLE);
	temp = (char*)GlobalAlloc(GPTR, 4096);
	lstrcpy(temp, s);
	return temp;
}

BOOL CALLBACK Page6Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int a;
	a = WADlg_handleDialogMsgs(hwnd, message, wParam, lParam);
	if (a)
	{
		SetWindowLong(hwnd, DWL_MSGRESULT, a);
		return a;
	}
	
	static char file[MAX_PATH+1];
	static int songidx;
	
	switch (message)
	{
	case WM_INITDIALOG:
		{
			SetDlgItemText(hwnd, IDC_FMTEDIT, "[%artist% - ]$if2(%title%,$filepart(%filename%))[' ['[%bitrate%kbps][, %srate%Hz]']']$if(%type%,' [video]',)");
			
			songidx = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTPOS);
			
			/* Sending BM_CLICK to simulate a button press doesn't work
			   as expected, so we branch to one of our other message
			   handlers instead. */
			PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_GETMETADATA, 0), 0);
			return TRUE;
		}
	case WM_NOTIFY:
		{
			LPNMHDR pnmhdr = (LPNMHDR) lParam;
			//Break loose if not tp is calling...
			if ((NxS_TabPages*)pnmhdr->hwndFrom != tp) break;
			if (pnmhdr->code == PSN_SETACTIVE)
			{
				// This page needs more seldom WM_TIMER calls
				PostMessage(GetParent(hwnd), WM_NXSINFOMSG, 2000, NXSINFOMSG_SETUPDATEINTERVAL);
			}
			if (pnmhdr->code == PSN_KILLACTIVE)
			{
				// set default interval
				PostMessage(GetParent(hwnd), WM_NXSINFOMSG, 0, NXSINFOMSG_SETUPDATEINTERVAL);
			}
			break;
		}

	case WM_TIMER:
		{
			// donnow what 2 do yet...
			break;
		}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_PREVSONG:
			{
				int pllen;
				pllen = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTLENGTH);
				if (pllen == 0)
					songidx = -1;
				else if (--songidx<0) songidx=pllen-1;
				
				PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_GETMETADATA, 0), 0);
				break;
			}
		case IDC_NEXTSONG:
			{
				int pllen;
				pllen = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTLENGTH);
				if (pllen == 0)
					songidx=-1;
				else if (++songidx > pllen-1)
					songidx=0;
				
				PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_GETMETADATA, BN_CLICKED), 0);
				break;
			}
		case IDC_PLAYBTN:
			SendMessage(plugin.hwndParent, WM_WA_IPC, songidx, IPC_SETPLAYLISTPOS);
			SendMessage(plugin.hwndParent, WM_COMMAND, WINAMP_BUTTON2, 0);
			break;
		case IDC_GETMETADATA:
			{
				char metadata[256];
				extendedFileInfoStruct efi;
				int pllen;
				
				pllen = SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETLISTLENGTH);
				
				if (pllen > 0)
				{
					lstrcpy(file, (LPCTSTR)SendMessage(plugin.hwndParent, WM_WA_IPC, songidx, IPC_GETPLAYLISTFILE));
					
					SetDlgItemText(hwnd, IDC_EDITFILENAME, file);
					
					char s[32];
					wsprintf(s, "%d/%d", songidx+1, pllen);
					SetDlgItemText(hwnd, IDC_SONGIDX, s);
				} else {
					SetDlgItemText(hwnd, IDC_EDITFILENAME, "");
					SetDlgItemText(hwnd, IDC_SONGIDX, "0/0");
					SetDlgItemText(hwnd, IDC_EDITARTIST, "");
					SetDlgItemText(hwnd, IDC_EDITTITLE, "");
					SetDlgItemText(hwnd, IDC_EDITALBUM, "");
					SetDlgItemText(hwnd, IDC_EDITTRACK, "");
					SetDlgItemText(hwnd, IDC_EDITYEAR, "");
					SetDlgItemText(hwnd, IDC_EDITGENRE, "");
					SetDlgItemText(hwnd, IDC_EDITCOMMENT, "");
					lstrcpy(file, "");
				}
				
				
				efi.filename = file;
				efi.ret = metadata;
				efi.retlen = sizeof(metadata);
				
				efi.metadata = "ARTIST";
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_GET_EXTENDED_FILE_INFO);
				SetDlgItemText(hwnd, IDC_EDITARTIST, metadata);
				
				efi.metadata = "TITLE";
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_GET_EXTENDED_FILE_INFO);
				SetDlgItemText(hwnd, IDC_EDITTITLE, metadata);
				
				efi.metadata = "ALBUM";
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_GET_EXTENDED_FILE_INFO);
				SetDlgItemText(hwnd, IDC_EDITALBUM, metadata);
				
				efi.metadata = "TRACK";
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_GET_EXTENDED_FILE_INFO);
				SetDlgItemText(hwnd, IDC_EDITTRACK, metadata);
				
				efi.metadata = "YEAR";
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_GET_EXTENDED_FILE_INFO);
				SetDlgItemText(hwnd, IDC_EDITYEAR, metadata);
				
				efi.metadata = "GENRE";
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_GET_EXTENDED_FILE_INFO);
				SetDlgItemText(hwnd, IDC_EDITGENRE, metadata);
				
				efi.metadata = "COMMENT";
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_GET_EXTENDED_FILE_INFO);
				SetDlgItemText(hwnd, IDC_EDITCOMMENT, metadata);
				
				break;
			}
		case IDC_SETMETADATA:
			{
				char metadata[256];
				extendedFileInfoStruct efi;
				
				efi.filename = file;
				efi.ret = metadata;
				
				efi.metadata = "ARTIST";
				GetDlgItemText(hwnd, IDC_EDITARTIST, metadata, sizeof(metadata)-1);
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_SET_EXTENDED_FILE_INFO);
				
				efi.metadata = "TITLE";
				GetDlgItemText(hwnd, IDC_EDITTITLE, metadata, sizeof(metadata)-1);
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_SET_EXTENDED_FILE_INFO);
				
				efi.metadata = "ALBUM";
				GetDlgItemText(hwnd, IDC_EDITALBUM, metadata, sizeof(metadata)-1);
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_SET_EXTENDED_FILE_INFO);
				
				efi.metadata = "TRACK";
				GetDlgItemText(hwnd, IDC_EDITTRACK, metadata, sizeof(metadata)-1);
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_SET_EXTENDED_FILE_INFO);
				
				efi.metadata = "YEAR";
				GetDlgItemText(hwnd, IDC_EDITYEAR, metadata, sizeof(metadata)-1);
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_SET_EXTENDED_FILE_INFO);
				
				efi.metadata = "GENRE";
				GetDlgItemText(hwnd, IDC_EDITGENRE, metadata, sizeof(metadata)-1);
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_SET_EXTENDED_FILE_INFO);
				
				efi.metadata = "COMMENT";
				GetDlgItemText(hwnd, IDC_EDITCOMMENT, metadata, sizeof(metadata)-1);
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_SET_EXTENDED_FILE_INFO);
				
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&efi, IPC_WRITE_EXTENDED_FILE_INFO);
				
				// Send some messages to inform Winamp we have updated shit
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&file, IPC_FILE_TAG_MAY_HAVE_UPDATED);
				SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_REFRESHPLCACHE);
				
				
				
				break;
			}
		case IDC_INFOBTN:
			{
				infoBoxParam ib;
				int len;
				len = GetWindowTextLength(GetDlgItem(hwnd, IDC_EDITFILENAME))+1;
				ib.parent = hwnd;
				ib.filename = (char*)GlobalAlloc(GPTR, len);
				GetWindowText(GetDlgItem(hwnd, IDC_EDITFILENAME), ib.filename, len);
				SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&ib, IPC_INFOBOX);
				GlobalFree((HGLOBAL)ib.filename);
				PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_GETMETADATA, BN_CLICKED), 0);
				break;
			}			
		case IDC_TESTFMTTITLE:
			{
				char fmt[256];
				char *tmp;
				
				GetDlgItemText(hwnd, IDC_FMTEDIT, fmt, sizeof(fmt)-1);
				tmp = GetFormattedTitleFromWinamp(lstrlen(fmt)?fmt:NULL);
				SetDlgItemText(hwnd, IDC_FMTEDIT2, tmp);
				GlobalFree((HGLOBAL)tmp);
				break;
			}
		}
		break;
		
	case WM_PAINT:
		{
			int tab[12] = {IDC_FRAME1|DCW_SUNKENBORDER, IDC_FRAME2|DCW_SUNKENBORDER,
				IDC_FMTEDIT|DCW_SUNKENBORDER, IDC_FMTEDIT2|DCW_SUNKENBORDER,
				IDC_EDITARTIST|DCW_SUNKENBORDER, IDC_EDITTITLE|DCW_SUNKENBORDER,
				IDC_EDITALBUM|DCW_SUNKENBORDER, IDC_EDITTRACK|DCW_SUNKENBORDER,
				IDC_EDITYEAR|DCW_SUNKENBORDER, IDC_EDITFILENAME|DCW_SUNKENBORDER,
				IDC_EDITGENRE|DCW_SUNKENBORDER, IDC_EDITCOMMENT|DCW_SUNKENBORDER};
			WADlg_DrawChildWindowBorders(hwnd, tab, 12);
			return FALSE;
		}
	}
	
	return FALSE;
}


void config(void)
{
	embedWindow_t embedWindow;
	HWND cfgdlg;
	RECT wr;
	
	// If the window already exists, bring it to top
	if (IsWindow(cfgews.me))
	{
		SetForegroundWindow(cfgews.me);
		return;
	}
	
	
	// Get function pointer
	embedWindow=(embedWindow_t)SendMessage(plugin.hwndParent,WM_WA_IPC,0,IPC_GET_EMBEDIF);
	
	// Center dialog in workarea
	SystemParametersInfo(SPI_GETWORKAREA, 0, &wr, 0);
	wr.right -= wr.left;
	wr.bottom -= wr.top;
	cfgews.r.left = (wr.right-425) / 2;
	cfgews.r.top = (wr.bottom-580) / 2;
	cfgews.r.right = cfgews.r.left + 425;
	cfgews.r.bottom = cfgews.r.top + 590;

	cfgews.flags = 0;
	embedWindow(&cfgews);

	cfgdlg = CreateDialogParam(plugin.hDllInstance, MAKEINTRESOURCE(IDD_CONFIG),
		cfgews.me, ConfigProc, NULL);
	
	ShowWindow(cfgdlg, SW_SHOW);
	SetWindowText(cfgews.me, "NxS Info Tool");
	ShowWindow(cfgews.me, SW_SHOWNA);

}


void quit()
{
	char ini[MAX_PATH+1];
	char *p;
	char s[256];
	
	GetModuleFileName(plugin.hDllInstance, ini, MAX_PATH);
	p = ini+lstrlen(ini);
	while (p >= ini && *p != '\\') p--;
	*++p = 0;
	lstrcat(ini, "plugin.ini");
	
	//	lstrcpy(ini, (char*)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GETINIDIRECTORY));
	
	wsprintf(s, "%d", g_isvisible);
	WritePrivateProfileString(PLUGINSHORTNAME, "visible", s, ini);
	
	wsprintf(s, "%d", g_starttabidx);
	WritePrivateProfileString(PLUGINSHORTNAME, "lastpageidx", s, ini);
	
	wsprintf(s, "%d", g_autorefresh);
	WritePrivateProfileString(PLUGINSHORTNAME, "autorefresh", s, ini);
	
}

int init()
{
	char ini[MAX_PATH+1];
	char *p;
	
	MENUITEMINFO mii;
	HMENU WinampMenu;
	
	//if (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, 0) < 0x5001) return 1;
	
	GetModuleFileName(plugin.hDllInstance, ini, MAX_PATH);
	p = ini+lstrlen(ini);
	while (p >= ini && *p != '\\') p--;
	*++p = 0;
	lstrcat(ini, "plugin.ini");
	
	OldWinampWndProc = (WNDPROC)SetWindowLong(plugin.hwndParent,
		GWL_WNDPROC, (LONG)WinampSubclass);
	
	mii.cbSize		= sizeof(MENUITEMINFO);
	mii.fMask		= MIIM_TYPE|MIIM_ID;
	mii.dwTypeData	= plugin.description;
	mii.fType		= MFT_STRING;
	mii.wID			= MENUID_SHOWMYWND;
	
	/* If we got an old Winamp to work with, then obtain the
	   popup menu the old fashion way. */
	if (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, 0) < 0x4901)
		WinampMenu = GetSubMenu(GetSystemMenu(plugin.hwndParent, FALSE), 0);
	else
		WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 0, IPC_GET_HMENU);
	
	InsertMenuItem(WinampMenu, 40258, FALSE, &mii);
	/* Always remember to adjust "Option" submenu position. */
	SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_ADJUST_OPTIONSMENUPOS);
	
	if (SendMessage(plugin.hwndParent, WM_WA_IPC, 0, 0) >= 0x4901)
	{
		/* Insert menuitem in help menu as well. */
		WinampMenu = (HMENU)SendMessage(plugin.hwndParent, WM_WA_IPC, 5, IPC_GET_HMENU);
		InsertMenuItem(WinampMenu, 2, TRUE, &mii);
		//not needed: SendMessage(plugin.hwndParent, WM_WA_IPC, 1, IPC_ADJUST_FFOPTIONSMENUPOS);
	}
	
	LoadLibrary("riched32.dll");
	
	g_starttabidx = GetPrivateProfileInt(PLUGINSHORTNAME, "lastpageidx", 0, ini);
	g_autorefresh = GetPrivateProfileInt(PLUGINSHORTNAME, "autorefresh", 1, ini);
	
	
	/* 1st we create a page, then we remove it. Why??
	   Well.. that's so we can get hold of a prefsDlgRec structure,
	   so we can iterate the next member of it later. */
	prefsDlgRec _prefs;
	_prefs.hInst = plugin.hDllInstance;
	_prefs.name = "NxS Info Tool";
	_prefs.proc = DefDlgProc;
	_prefs.where = 0;
	_prefs.dlgID = IDD_CONFIG;
	
	SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&_prefs, IPC_ADD_PREFS_DLG);
	prefs = _prefs.next;
	SendMessage(plugin.hwndParent, WM_WA_IPC, (WPARAM)&_prefs, IPC_REMOVE_PREFS_DLG);
	
	/* If visible is 1 in plugin.ini, then show the window. */
	if ( GetPrivateProfileInt(PLUGINSHORTNAME, "visible", 1, ini) ) config();

	return 0;
}



LRESULT WinampSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (LOWORD(wParam)==MENUID_SHOWMYWND && HIWORD(wParam) == 0)
		{
			config();
		}
		break;
	/* Also handle WM_SYSCOMMAND if people are selectng the item through
	   Winamp submenu in system menu. */
	case WM_SYSCOMMAND:
		if (wParam == MENUID_SHOWMYWND)
		{
			config();
		}
		break;
	}
	return CallWindowProc(OldWinampWndProc, hwnd, uMsg, wParam, lParam);
}



extern "C" __declspec(dllexport) winampGeneralPurposePlugin* winampGetGeneralPurposePlugin()
{
	return &plugin;
}

