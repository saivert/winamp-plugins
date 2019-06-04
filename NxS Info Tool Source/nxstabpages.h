/* NxS Tabpages 
 *
 * How to use (the long and winding road...)
 * ----------------------------------------
 * Put a Tab Control ("SysTabControl32" class) on your dialog and create
 * several new dialogs for the pages and add them to your resource script.
 *
 * Instantiate NxS_TabPages class. You may do it in WM_INITDIALOG like this:
 * NxS_TabPages *g_tp; //Global variable
 *   case WM_INITDIALOG:
 *   {
 *     g_tp = new NxS_TabPages(GetDlgItem(hwndDlg, IDC_TAB1), g_hInstance);
 *     g_tp->AddPage(NULL, MAKEINTRESOURCE(IDD_DIALOG1), DummyProc, "A tooltip");
 *     g_tp->AddPage("A new one", MAKEINTRESOURCE(IDD_DIALOG1), DummyProc, NULL);
 *     g_tp->SelectPage(0); //select first page
 *   }
 *
 * Notification messages:
 *   The page dialogs will receive the same notification messages as page dialogs
 *   in a Property Sheet does (see Win32 docs; keyword PropertySheet), except
 *   that the hwndFrom and idFrom members of NMHDR structure is not used.
 *   I have simply duplicated the message system in this class and I think this makes
 *   more sense to do as you don't have to learn yet another message system.
 *
 *   To make the tab control actually switch between pages when you click the tabs
 *   you must call HandleNotifications() method in your WM_NOTIFY handler.
 *
 *   In a dialog procedure do:
 *     int i=g_tp->HandleNotifications(wParam, lParam);
 *     if (i)
 *     {
 *       SetWindowLong(hwnd, DWL_MSGRESULT, s);
 *       return TRUE;
 *     }
 *
 *   In a simple window procedure do:
 *     int i=g_tp->HandleNotifications(wParam, lParam);
 *     if (i) return i;
 *
 *   Feel free to guard your code by putting it in a "if (g_tp) {...}" block.
 *
 * Please read through the source in nxstabpages.cpp for more in-depth information.
 * It contains a lot of comments that might be helpful.
 *
 * Written by Saivert
 * Homepage http://members.tripod.com/files_saivert/
 */


#if !defined(__NXSTABPAGES_H)
#define __NXSTABPAGES_H

#if _MSC_VER > 1000
#pragma once
#endif

#include <windows.h>
#include <commctrl.h>

#define NXSTABPAGES_TABITEMSIZE ( sizeof(TABITEM)-sizeof(TC_ITEMHEADER) )



class NxS_TabPages
{
public:

	//Flags used by EnableThemeDlgTexture
	enum _ETDT_ENUM {
		ETDT_DISABLE       = 0x01,
		ETDT_ENABLE        = 0x02,
		ETDT_USETABTEXTURE = 0x04,
		ETDT_ENABLETAB     = (ETDT_ENABLE|ETDT_USETABTEXTURE)
	} ETDT_ENUM;

	//TABITEM: Used like a struct, but declared as a class so I can initialize
	//the members to NULL automatically and set the members using neat functions.
	class TABITEM
	{
	public:
		TABITEM(): title(NULL), tip(NULL), dialog(NULL), dlgproc(NULL), dlghandle(NULL)
		{
			itemhdr.mask=TCIF_PARAM;
		}
		const char* SetDlgRes(const char *lpszDlgRes) { return dialog=strdup(lpszDlgRes); }
		const char* SetTitle(const char *lpszTitle) { return title=strdup(lpszTitle); }
		const char* SetTip(const char *lpszTip) { return tip=strdup(lpszTip); }
		void SetDlgHandle(HWND hw) { dlghandle = hw; }
	public:
		TC_ITEMHEADER itemhdr;
		char *dialog;
		DLGPROC dlgproc;
		HWND dlghandle;
		char *title;
		char *tip;
	};

public:
	NxS_TabPages(HWND TabCtrl=NULL, HINSTANCE hinst=NULL):
	  m_hinst(hinst), m_curwnd(0), m_UseDlgTexture(true)
	{
		if (!hinst) m_hinst = GetModuleHandle(NULL);
		SetTabCtrl(TabCtrl);
	}

	~NxS_TabPages();

	//When you set a new Tab Control, all the tabs on the old and the new tab control
	//are removed and you must call AddPage() to add new pages.
	//This method is called mostly when you specify NULL for TabCtrl in the constructor.
	HWND SetTabCtrl(HWND NewTabCtrl);
	HWND GetTabCtrl(void) { return m_hwTab; }

	//AddPage: Returns the index of the new tab if successful or  -1 otherwise.
	int AddPage(char* lpszTitle, char* lpszDlgRes, DLGPROC lpfnDlgProc, char* lpszTip);
	bool DelPage(int index);
	bool GetPage(int index, TABITEM &tp); // Fills in a TABITEM you supply
	TABITEM& Pages(int index); //Array style function
	HWND GetCurrentPageHWND(void) { return m_curwnd; }
	bool Clear(void);

	bool SelectPage(int index);
	bool HandleNotifications(WPARAM wParam, LPARAM lParam);

	int GetCount(void) { return TabCtrl_GetItemCount(m_hwTab); }
	int GetSelPage(void) { return TabCtrl_GetCurSel(m_hwTab); }

	bool SendApply(); //Returns true if it's okey to close dialog
	void AdjustPageSize(void);
	bool SetUseThemedDlgTexture(bool use);

	static HRESULT EnableThemeDlgTexture(HWND hwnd, DWORD flags);

protected:
	HWND m_hwTab;
	HWND m_curwnd;
	HINSTANCE m_hinst;
	TABITEM m_tptemp; //Used by Pages() only
	bool m_UseDlgTexture; //Used by SetUseThemedDlgTexture() only
};

#endif //!defined(__NXSTABPAGES_H)
