/* NxS Tabpages - written by Saivert */
//#include "StdAfx.h"
#include "nxstabpages.h"

NxS_TabPages::~NxS_TabPages()
{
	Clear();
}

HWND NxS_TabPages::SetTabCtrl(HWND NewTabCtrl)
{
	HWND tmp;
	tmp = m_hwTab;
	if (IsWindow(NewTabCtrl))
	{
		if (GetCount()>0)
			TabCtrl_DeleteAllItems(m_hwTab);
		m_hwTab = NewTabCtrl;
		if (GetCount()>0)
			TabCtrl_DeleteAllItems(m_hwTab);
		TabCtrl_SetItemExtra(m_hwTab, NXSTABPAGES_TABITEMSIZE);
	}
	return tmp;
}

bool NxS_TabPages::DelPage(int index)
{
	TABITEM item;

	if (TabCtrl_GetItem(m_hwTab, index, &item))
	{
		if (item.dlghandle)
		{
			NMHDR hdr;
			hdr.code = PSN_RESET;
			hdr.hwndFrom = (HWND)this;
			hdr.idFrom = 0;
			if (SendMessage(item.dlghandle, WM_NOTIFY, 0, (LPARAM)&hdr))
			{
				if (TRUE==GetWindowLong(item.dlghandle, DWL_MSGRESULT))
					return false;
			}

			DestroyWindow(item.dlghandle);
		}
		TabCtrl_DeleteItem(m_hwTab, index);
		return true;
	}
	return false;
}

bool NxS_TabPages::GetPage(int index, TABITEM &tp)
{
	return TabCtrl_GetItem(m_hwTab, index, &tp)>0;
}

NxS_TabPages::TABITEM& NxS_TabPages::Pages(int index)
{
	TabCtrl_GetItem(m_hwTab, index, &m_tptemp);
	return m_tptemp;
}

bool NxS_TabPages::SendApply()
{
	bool res=true;
	TABITEM item;

	//First we check if the active page wants to loose activation
	if (m_curwnd)
	{
		NMHDR hdr;
		hdr.code = PSN_KILLACTIVE;
		hdr.hwndFrom = (HWND)this;
		hdr.idFrom = 0;
		if (SendMessage(m_curwnd, WM_NOTIFY, 0, (LPARAM)&hdr))
		{
			if (TRUE==GetWindowLong(m_curwnd, DWL_MSGRESULT))
				return false;
		}
	}

	//If the active page said OK, we send a PSN_APPLY notification message to
	//all the pages. Any one of the pages can return PSNRET_INVALID_NOCHANGEPAGE
	//to make SendApply() return false. This is a clue to keep the host dialog open.
	for (int i=0; i<GetCount(); i++)
	{
		if (TabCtrl_GetItem(m_hwTab, i, &item))
		{
			if (item.dlghandle)
			{
				NMHDR hdr;
				hdr.code = PSN_APPLY;
				hdr.hwndFrom = (HWND)this;
				hdr.idFrom = 0;
				if (SendMessage(item.dlghandle, WM_NOTIFY, 0, (LPARAM)&hdr))
				{
					if (PSNRET_INVALID_NOCHANGEPAGE==GetWindowLong(item.dlghandle, DWL_MSGRESULT))
						res=false;
				}
			}
		}
	}
	return res;
}

bool NxS_TabPages::Clear(void)
{
	int i;
	while ((i = GetCount()) > 0)
	{
		if (!DelPage(i-1)) return false;
	}
	m_curwnd=NULL; //There's no longer an active dialog now...
	return true;
}

bool NxS_TabPages::HandleNotifications(WPARAM wParam, LPARAM lParam)
{
	int idTabCtl = (int) LOWORD(wParam);
	LPNMHDR lpnmhdr = (LPNMHDR) lParam;

	if (lpnmhdr->hwndFrom == m_hwTab)
	{
		if (lpnmhdr->code == TCN_SELCHANGING)
		{
			NMHDR hdr;
			hdr.code = PSN_KILLACTIVE;
			hdr.hwndFrom = (HWND)this;
			hdr.idFrom = 0;
			if (SendMessage(m_curwnd, WM_NOTIFY, 0, (LPARAM)&hdr))
			{
				if (TRUE==GetWindowLong(m_curwnd, DWL_MSGRESULT))
					return true;
			}
		}

		if (lpnmhdr->code == TCN_SELCHANGE)
		{
			return SelectPage(GetSelPage());
		}

	}
	if (lpnmhdr->code == TTN_NEEDTEXT)
	{
		LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT) lParam;
		TABITEM item;

		//Make sure it's our tab control's tooltip calling in...
		if (lpnmhdr->hwndFrom != TabCtrl_GetToolTips(m_hwTab)) return false;

		TabCtrl_GetItem(m_hwTab, lpttt->hdr.idFrom, &item);

		lpttt->hinst = IS_INTRESOURCE(item.tip)?m_hinst:NULL;
		lpttt->lpszText = item.tip;
	}
	return false;
}

int NxS_TabPages::AddPage(char* lpszTitle, char* lpszDlgRes, DLGPROC lpfnDlgProc, char* lpszTip)
{
	TABITEM i;

	if (!lpszDlgRes) return 0;

	if (IS_INTRESOURCE(lpszDlgRes))
		i.dialog = lpszDlgRes;
	else
		i.SetDlgRes(lpszDlgRes);

	if (IS_INTRESOURCE(lpszTip))
		i.tip = lpszTip;
	else
		i.SetTip(lpszTip);

	if (!lpszTitle)
	{
		int len;
		HWND hwTemp;
		hwTemp = CreateDialog(m_hinst,(LPCTSTR)lpszDlgRes,
			GetDesktopWindow(), lpfnDlgProc);

		len = GetWindowTextLength(hwTemp)+1;
		i.title = new char[len];
		GetWindowText(hwTemp, i.title, len);
		DestroyWindow(hwTemp);
	}
	else
		i.SetTitle(lpszTitle);

	i.dlgproc = lpfnDlgProc;
	i.dlghandle = NULL; //Dialog not created yet...

	i.itemhdr.mask = TCIF_TEXT|TCIF_PARAM;
	i.itemhdr.pszText = i.title;

	return TabCtrl_InsertItem(m_hwTab, GetCount(), &i);
}


bool NxS_TabPages::SelectPage(int index)
{
	HWND hwndDlg;
	TABITEM item;
	hwndDlg = GetParent(m_hwTab);
	if (index >= 0)
	{
		if (m_curwnd)
		{
			NMHDR hdr;
			hdr.code = PSN_KILLACTIVE;
			hdr.hwndFrom = (HWND)this;
			hdr.idFrom = 0;

			if (SendMessage(m_curwnd, WM_NOTIFY, 0, (LPARAM)&hdr))
			{
				if (TRUE==GetWindowLong(m_curwnd, DWL_MSGRESULT))
					return false;
			}


			ShowWindow(m_curwnd, SW_HIDE);
		}

		if (!TabCtrl_GetItem(m_hwTab, index, &item)) return false;

		if (NULL==item.dlghandle)
		{
			item.SetDlgHandle(
				CreateDialogParam(m_hinst, item.dialog,
					hwndDlg, item.dlgproc, LPARAM(&item))
			);

			SetWindowLong(item.dlghandle, GWL_STYLE, WS_CHILD);
			SetWindowLong(item.dlghandle, GWL_EXSTYLE, WS_EX_CONTROLPARENT);
			SetParent(item.dlghandle, hwndDlg);
			EnableThemeDlgTexture(item.dlghandle,
				m_UseDlgTexture?ETDT_ENABLETAB:ETDT_DISABLE);

			//Update item.dlghandle (application data item)
			TabCtrl_SetItem(m_hwTab, index, &item);		
		}

		{
			NMHDR hdr;
			hdr.code = PSN_SETACTIVE;
			hdr.hwndFrom = (HWND)this;
			hdr.idFrom = 0;

			if (SendMessage(item.dlghandle, WM_NOTIFY, 0, (LPARAM)&hdr))
			{
				if (0==GetWindowLong(item.dlghandle, DWL_MSGRESULT)) return false;
			}
		}

		if (m_curwnd=item.dlghandle)
		{
			RECT r;
			GetWindowRect(m_hwTab,&r);
			TabCtrl_AdjustRect(m_hwTab, FALSE, &r);

			MapWindowPoints(HWND_DESKTOP, hwndDlg, LPPOINT(&r), 2);
			//Make right & bottom be the width & height
			r.right -= r.left;
			r.bottom -= r.top;

			SetWindowPos(item.dlghandle, 0, r.left, r.top, r.right, r.bottom,
				SWP_NOACTIVATE|SWP_NOZORDER);
			
			ShowWindow(item.dlghandle,SW_SHOWNA);
		}
		TabCtrl_SetCurSel(m_hwTab, index);
	}
	return true;
}

void NxS_TabPages::AdjustPageSize(void)
{
	RECT r;
	GetWindowRect(m_hwTab,&r);
	TabCtrl_AdjustRect(m_hwTab, FALSE, &r);

	MapWindowPoints(HWND_DESKTOP, GetParent(m_hwTab), LPPOINT(&r), 2);
	//Make right & bottom be the width & height
	r.right -= r.left;
	r.bottom -= r.top;

	SetWindowPos(m_curwnd, 0, r.left, r.top, r.right, r.bottom,
		SWP_NOACTIVATE|SWP_NOZORDER);
}

bool NxS_TabPages::SetUseThemedDlgTexture(bool use)
{
	bool prev=m_UseDlgTexture;
	// If there are already some pages added,
	// change the state for these pages' dialogs.
	if (int count=GetCount())
	{
		for (int i=0;i<count;i++)
		{
			if (HWND hwndDlg=Pages(i).dlghandle)
				EnableThemeDlgTexture(hwndDlg, use?ETDT_ENABLETAB:ETDT_DISABLE);
		}
	}
	return prev;
}


// A wrapper for the "EnableThemeDialogTexture" function located in
// UXTHEME.DLL. When you call this with the handle of a dialog, the
// dialog's background get a texture that matches the current theme.
// This only works on a Windows XP machine.
HRESULT NxS_TabPages::EnableThemeDlgTexture(HWND hwnd, DWORD flags)
{
	typedef HRESULT (WINAPI * ENABLETHEMEDIALOGTEXTURE)(HWND, DWORD);

	ENABLETHEMEDIALOGTEXTURE pfnETDT;
	HINSTANCE hDll;
	HRESULT res=-1;

	if (NULL != (hDll = LoadLibrary(TEXT("uxtheme.dll"))))
	{
		if (NULL != (pfnETDT = (ENABLETHEMEDIALOGTEXTURE)GetProcAddress(hDll, "EnableThemeDialogTexture")))
		{
			res = pfnETDT(hwnd, ETDT_ENABLETAB);
		}
		FreeLibrary(hDll);
	}
	return res;
}

//#EOF