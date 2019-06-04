/* NxS Tabpages 
 *
 * How to use (the long and winding road...)
 * -----------------------------------------
 * Put a Tab Control ("SysTabControl32" class) on your dialog and create
 * several new dialogs for the pages and add them to your resource script.
 *
 * Instantiate a NxS_TabPages class. You may do it in WM_INITDIALOG like this:
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
 *   that the "hwndFrom" member of the NMHDR structure in the WM_NOTIFY message
 *   is a pointer to the NxS_TabPages class that is calling in.
 *   The "idFrom" member of NMHDR structure is not used.
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
 *   Feel free to guard your code by putting it in a "if (g_tp) {...}" block, like:
 *     #define g_tp() if (g_tp) g_tp
 *
 * Please read through the source in nxstabpages.cpp for more in-depth information.
 * It contains a lot of comments that might be helpful.
 *
 * Written by Saivert
 * Homepage http://inthegray.com/saivert/
 */


#if !defined(__NXSTABPAGES_H)
#define __NXSTABPAGES_H

#if _MSC_VER > 1000
#pragma once
#endif

#include <windows.h>
#include <commctrl.h>

#define NXSTABPAGES_TABITEMSIZE ( sizeof(TABITEMINFO)-sizeof(TC_ITEMHEADER) )



class CNxSTabPages
{
public:

	//Flags used by EnableThemeDlgTexture
	enum _ETDT_ENUM {
		ETDT_DISABLE       = 0x01,
		ETDT_ENABLE        = 0x02,
		ETDT_USETABTEXTURE = 0x04,
		ETDT_ENABLETAB     = (ETDT_ENABLE|ETDT_USETABTEXTURE)
	} ETDT_ENUM;

	//TABITEM: Used like a struct, but declared as a class so I have a centralized
	//property modifier kinda-thing...
	class TABITEMINFO
	{
	public:
		TABITEMINFO(): title(NULL), tip(NULL), dlgres(NULL), dlgproc(NULL), dlghandle(NULL)
		{
			itemhdr.mask=TCIF_PARAM;
		}
		char* GetDlgRes(void) const { return dlgres; }
		char* GetTitle(void) const { return title; }
		char* GetTip(void) const { return tip; }
		HWND GetDlgHandle(void) const { return dlghandle; }
		DLGPROC GetDlgProc(void) const { return dlgproc; }

	//This is the struct part, used when adding tabs (items) to the Tab Control.
	protected:
		TC_ITEMHEADER itemhdr;
		char *dlgres;
		DLGPROC dlgproc;
		HWND dlghandle;
		char *title;
		char *tip;
	};

	class TABITEM : public TABITEMINFO
	{
	public:
		TABITEM(CNxSTabPages *ref)
		{
			m_ref = ref;
		}

		char* SetDlgRes(char *lpszDlgRes) {
			char *t=dlgres;
			if (IS_INTRESOURCE(lpszDlgRes))
				dlgres=lpszDlgRes;
			else
				dlgres=strdup(lpszDlgRes);
			return t;
		}

		char* SetTitle(char *lpszTitle) {
			char *t=title;
			title=strdup(lpszTitle);
			itemhdr.mask = TCIF_TEXT|TCIF_PARAM;
			itemhdr.pszText = title;
			return title;
		}

		char* SetTip(char *lpszTip) {
			char *t=tip;
			if (IS_INTRESOURCE(lpszTip))
				tip=lpszTip;
			else
				tip=strdup(lpszTip);
			return t;
		}

		HWND SetDlgHandle(HWND hw) { HWND t=dlghandle; dlghandle = hw; return t; }
		DLGPROC SetDlgProc(DLGPROC newproc) { DLGPROC t=dlgproc; dlgproc = newproc; return t; }
	protected:
		CNxSTabPages *m_ref;
	};

public:
	CNxSTabPages(HWND TabCtrl=NULL, HINSTANCE hinst=NULL):
	  m_hinst(hinst), m_curwnd(0), m_UseDlgTexture(true), m_UseTabClientArea(false)
	{
		if (!hinst) m_hinst = GetModuleHandle(NULL);
		SetTabCtrl(TabCtrl);
	}

	~CNxSTabPages();

	//When you set a new Tab Control, all the tabs on the old and the new tab control
	//are removed and you must call AddPage() to add new pages.
	//This method is called mostly when you specify NULL for TabCtrl in the constructor.
	HWND SetTabCtrl(HWND NewTabCtrl);
	HWND GetTabCtrl(void) { return m_hwTab; }

	//AddPage: Returns the index of the new tab if successful or  -1 otherwise.
	int AddPage(char* lpszTitle, char* lpszDlgRes, DLGPROC lpfnDlgProc, char* lpszTip);
	bool DelPage(int index);
	bool GetPage(int index, TABITEMINFO &tp); // Fills in a TABITEM you supply
	TABITEMINFO& Pages(int index); //Array style function
	HWND GetCurrentPageHWND(void) { return m_curwnd; }
	bool Clear(void);

	bool SelectPage(int index);
	bool HandleNotifications(WPARAM wParam, LPARAM lParam);

	int GetCount(void) { return TabCtrl_GetItemCount(m_hwTab); }
	int GetSelPage(void) { return TabCtrl_GetCurSel(m_hwTab); }

	//Sends PSN_KILLACTIVE to selected page's dialog and then
	//sends the PSN_APPLY message to all page dialogs.
	//Returns true if it's okey to close the outer dialog.
	bool SendApply();

	//Resizes the current page's dialog to fit the Tab Control.
	//You must resize the Tab Control yourself before calling this.
	//Tip: GetTabCtrl() returns the assigned Tab Control.
	void AdjustPageSize(void);

	//When you call this with with true as param, the dialog's
	//background get a texture that matches the current theme.
	//This only works on a Windows XP machine.
	bool SetUseThemedDlgTexture(bool use);
	bool GetUseThemedDlgTexture(void) { return m_UseDlgTexture; }

	//Call this with true as param to make the page dialogs fill
	//the entire client area of the Tab Control. This will
	//also update the current page.
	bool SetUseTabClientArea(bool use);
	bool GetUseTabClientArea(void) { return m_UseTabClientArea; }

	static HRESULT EnableThemeDlgTexture(HWND hwnd, DWORD flags);

protected:
	bool SelectPage_Internal(int index);

	HWND m_hwTab;
	HWND m_curwnd;
	HINSTANCE m_hinst;
	TABITEMINFO m_tptemp; //Used by Pages() only
	bool m_UseDlgTexture; //Used by SetUseThemedDlgTexture() only
	bool m_UseTabClientArea; //Used by SetUseTabClientArea
};

#endif //!defined(__NXSTABPAGES_H)
