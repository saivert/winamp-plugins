/* NxS Tabpages - written by Saivert */
#include "StdAfx.h"
#include "nxstabpages.h"

CNxSTabPages::~CNxSTabPages()
{
	Clear();
}

HWND CNxSTabPages::SetTabCtrl(HWND NewTabCtrl)
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

bool CNxSTabPages::DelPage(int index)
{
	TABITEM item(this);

	if (TabCtrl_GetItem(m_hwTab, index, &item))
	{
		if (item.GetDlgHandle())
		{
			NMHDR hdr;
			hdr.code = PSN_RESET;
			hdr.hwndFrom = (HWND)this;
			hdr.idFrom = 0;
			if (TRUE==SendMessage(item.GetDlgHandle(), WM_NOTIFY, 0, (LPARAM)&hdr))
				return false;

			DestroyWindow(item.GetDlgHandle());
		}
		TabCtrl_DeleteItem(m_hwTab, index);
		return true;
	}
	return false;
}

bool CNxSTabPages::GetPage(int index, TABITEMINFO &tp)
{
	return TabCtrl_GetItem(m_hwTab, index, &tp)>0;
}

CNxSTabPages::TABITEMINFO& CNxSTabPages::Pages(int index)
{
	GetPage(index, m_tptemp);
	return m_tptemp;
}

bool CNxSTabPages::SendApply()
{
	bool res=true;
	TABITEM item(this);

	//First we check if the active page wants to loose activation
	if (m_curwnd)
	{
		NMHDR hdr;
		hdr.code = PSN_KILLACTIVE;
		hdr.hwndFrom = (HWND)this;
		hdr.idFrom = 0;

		if (TRUE==SendMessage(m_curwnd, WM_NOTIFY, 0, (LPARAM)&hdr))
			return false;
	}

	//If the active page said OK, we send a PSN_APPLY notification message to
	//all the pages. Any one of the pages can return PSNRET_INVALID_NOCHANGEPAGE
	//to make SendApply() return false. This is a clue to keep the host dialog open.
	for (int i=0; i<GetCount(); i++)
	{
		if (TabCtrl_GetItem(m_hwTab, i, &item))
		{
			if (item.GetDlgHandle())
			{
				NMHDR hdr;
				hdr.code = PSN_APPLY;
				hdr.hwndFrom = (HWND)this;
				hdr.idFrom = 0;

				if (PSNRET_INVALID_NOCHANGEPAGE==
					SendMessage(item.GetDlgHandle(), WM_NOTIFY, 0, (LPARAM)&hdr))
				{
					res=false;
				}
			}
		}
	}
	return res;
}

bool CNxSTabPages::Clear(void)
{
	int i;
	while ((i = GetCount()) > 0)
	{
		if (!DelPage(i-1)) return false;
	}
	m_curwnd=NULL; //There's no longer an active dialog now...
	return true;
}

bool CNxSTabPages::HandleNotifications(WPARAM wParam, LPARAM lParam)
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

			return SendMessage(m_curwnd, WM_NOTIFY, 0, (LPARAM)&hdr)>0;
		}

		if (lpnmhdr->code == TCN_SELCHANGE)
		{
			return SelectPage_Internal(GetSelPage());
		}

	}
	if (lpnmhdr->code == TTN_NEEDTEXT)
	{
		LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT) lParam;
		TABITEM item(this);

		//Make sure it's our tab control's tooltip calling in...
		if (lpnmhdr->hwndFrom != TabCtrl_GetToolTips(m_hwTab)) return false;

		TabCtrl_GetItem(m_hwTab, lpttt->hdr.idFrom, &item);

		lpttt->hinst = IS_INTRESOURCE(item.GetTip())?m_hinst:NULL;
		lpttt->lpszText = item.GetTip();
	}
	return false;
}

int CNxSTabPages::AddPage(char* lpszTitle, char* lpszDlgRes, DLGPROC lpfnDlgProc, char* lpszTip)
{
	TABITEM i(this);

	if (!lpszDlgRes) return 0;

	i.SetDlgRes(lpszDlgRes);
	i.SetTip(lpszTip);
	i.SetDlgProc(lpfnDlgProc);

	if (!lpszTitle)
	{
		int len;
		char *t;
		HWND hwTemp;
		hwTemp = CreateDialog(m_hinst,(LPCTSTR)lpszDlgRes,
			GetDesktopWindow(), lpfnDlgProc);

		len = GetWindowTextLength(hwTemp)+1;
		t = new char[len];
		GetWindowText(hwTemp, t, len);
		i.SetTitle(t);
		DestroyWindow(hwTemp);
	}
	else
		i.SetTitle(lpszTitle);

	return TabCtrl_InsertItem(m_hwTab, GetCount(), &i);
}


bool CNxSTabPages::SelectPage(int index)
{
	if (m_curwnd)
	{
		NMHDR hdr;
		hdr.code = PSN_KILLACTIVE;
		hdr.hwndFrom = (HWND)this;
		hdr.idFrom = 0;

		if (TRUE==SendMessage(m_curwnd, WM_NOTIFY, 0, (LPARAM)&hdr))
			return false;
	}
	
	if (SelectPage_Internal(index))
	{
		TabCtrl_SetCurSel(m_hwTab, index);
		return true;
	}
	return false;
}

bool CNxSTabPages::SelectPage_Internal(int index)
{
	HWND hwndDlg;
	TABITEM item(this);
	hwndDlg = GetParent(m_hwTab);
	if (index >= 0)
	{
		if (m_curwnd) ShowWindow(m_curwnd, SW_HIDE);

		if (!TabCtrl_GetItem(m_hwTab, index, &item)) return false;

		if (NULL==item.GetDlgHandle())
		{
			item.SetDlgHandle(
				CreateDialogParam(m_hinst, item.GetDlgRes(),
					hwndDlg, item.GetDlgProc(), LPARAM(&item))
			);

			SetWindowLong(item.GetDlgHandle(), GWL_STYLE, WS_CHILD);
			SetWindowLong(item.GetDlgHandle(), GWL_EXSTYLE, WS_EX_CONTROLPARENT);
			SetParent(item.GetDlgHandle(), hwndDlg);
			EnableThemeDlgTexture(item.GetDlgHandle(),
				m_UseDlgTexture?ETDT_ENABLETAB:ETDT_DISABLE);

			//Update item.dlghandle (application data item)
			TabCtrl_SetItem(m_hwTab, index, &item);
		}

		{
			NMHDR hdr;
			hdr.code = PSN_SETACTIVE;
			hdr.hwndFrom = (HWND)this;
			hdr.idFrom = 0;

			if (1==SendMessage(item.GetDlgHandle(), WM_NOTIFY, 0, (LPARAM)&hdr))
				return false;
		}

		if (m_curwnd=item.GetDlgHandle())
		{
			RECT r;
			GetWindowRect(m_hwTab,&r);

			//Use client are if it says so...
			if (!m_UseTabClientArea)
				TabCtrl_AdjustRect(m_hwTab, FALSE, &r);

			MapWindowPoints(HWND_DESKTOP, hwndDlg, LPPOINT(&r), 2);
			//Make right & bottom be the width & height
			r.right -= r.left;
			r.bottom -= r.top;

			SetWindowPos(item.GetDlgHandle(), 0, r.left, r.top, r.right, r.bottom,
				SWP_NOACTIVATE|SWP_NOZORDER);
			
			ShowWindow(item.GetDlgHandle(),SW_SHOWNA);
		}
	}
	return true;
}

void CNxSTabPages::AdjustPageSize(void)
{
	RECT r;
	GetWindowRect(m_hwTab,&r);
	//Use client are if it says so...
	if (!m_UseTabClientArea)
		TabCtrl_AdjustRect(m_hwTab, FALSE, &r);

	MapWindowPoints(HWND_DESKTOP, GetParent(m_hwTab), LPPOINT(&r), 2);
	//Make right & bottom be the width & height
	r.right -= r.left;
	r.bottom -= r.top;

	SetWindowPos(m_curwnd, 0, r.left, r.top, r.right, r.bottom,
		SWP_NOACTIVATE|SWP_NOZORDER);
}

bool CNxSTabPages::SetUseThemedDlgTexture(bool use)
{
	bool prev=m_UseDlgTexture;
	m_UseDlgTexture=use;
	// If there are already some pages added,
	// change the state for these pages' dialogs.
	if (int count=GetCount())
	{
		for (int i=0;i<count;i++)
		{
			if (HWND hwndDlg=Pages(i).GetDlgHandle())
				EnableThemeDlgTexture(hwndDlg, use?ETDT_ENABLETAB:ETDT_DISABLE);
		}
	}
	return prev;
}

bool CNxSTabPages::SetUseTabClientArea(bool use)
{
	bool prev=m_UseTabClientArea;
	m_UseTabClientArea=use;
	// Update currently selected page.
	int i = GetSelPage();
	if (i >= 0) AdjustPageSize();
	return prev;
}

// A wrapper for the "EnableThemeDialogTexture" function located in
// UXTHEME.DLL. When you call this with the handle of a dialog, the
// dialog's background get a texture that matches the current theme.
// This only works on a Windows XP machine.
HRESULT CNxSTabPages::EnableThemeDlgTexture(HWND hwnd, DWORD flags)
{
	typedef HRESULT (WINAPI * ENABLETHEMEDIALOGTEXTURE)(HWND, DWORD);

	ENABLETHEMEDIALOGTEXTURE pfnETDT;
	HINSTANCE hDll;
	HRESULT res=-1;

	if (NULL != (hDll = LoadLibrary(TEXT("uxtheme.dll"))))
	{
		if (NULL != (pfnETDT = (ENABLETHEMEDIALOGTEXTURE)GetProcAddress(hDll, "EnableThemeDialogTexture")))
		{
			res = pfnETDT(hwnd, flags);
		}
		FreeLibrary(hDll);
	}
	return res;
}

//#EOF