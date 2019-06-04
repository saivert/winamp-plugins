/*
** Written by Saivert
**
** License:
** This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held 
** liable for any damages arising from the use of this software. 
**
** Permission is granted to anyone to use this software for any purpose, including commercial applications, and to 
** alter it and redistribute it freely, subject to the following restrictions:
**
**   1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. 
**      If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
**
**   2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
**
**   3. This notice may not be removed or altered from any source distribution.
**
*/

#pragma once

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#define NXSTOOLTIP_MAXLOADSTRING 512

class CNxSToolTip
{
  public:
	//- ctor & dtor -
	CNxSToolTip(HWND hwndOwner); //Uses GetModuleHandle(NULL) for hinst.
                                     //Tip: You can do "CNxSToolTip g_tooltip(0);" and
                                     //call "g_tooltip.SetOwnerWnd(myhwnd);" later on.

	CNxSToolTip(HWND hwndOwner, HINSTANCE hinst);//Supply hinst yourself.		
	~CNxSToolTip();

	//- methods -
	void Activate(BOOL fActivate) { SendMessage(m_hwnd, TTM_ACTIVATE, (WPARAM)fActivate, 0); }
	BOOL AddTool(UINT uFlags, UINT uId, LPRECT prect, LPTSTR lpszText);
	BOOL AddTool(UINT uFlags, UINT uId, LPRECT prect, UINT strId);

	BOOL SetToolInfo(UINT uFlags, UINT uId, LPRECT prect, LPTSTR lpszText);
	BOOL SetToolInfo(UINT uFlags, UINT uId, LPRECT prect, UINT strId);

	void DelTool(UINT uId);
	long GetCount(void) { return SendMessage(m_hwnd, TTM_GETTOOLCOUNT, 0, 0); }
	char* GetText(UINT uId);
	BOOL GetInfo(UINT uId, LPRECT prect, char **lpszText);
	BOOL HitTest(POINT point, LPRECT prect, char **lpszText);

	void SetDelayTime(UINT uAutoPop, UINT uInitial, UINT uReshow);
	void SetDelayTime(UINT uAutomatic);

	void SetMaxTipWidth(int width) { SendMessage(m_hwnd, TTM_SETMAXTIPWIDTH, 0, width); }

	void UpdateTipText(UINT uId, char* lpszText);
	void UpdateTipText(UINT uId, UINT strId);

	void RelayEvent(LPMSG msg);

	/* This AllInOne method adds the controls in the owner window to the
	   tooltip control and uses the IDs of the controls as the ID of a
	   string in the string list.
	   If you got a control with the ID constant IDC_BUTTON1, then you can
	   add a string with the same ID to the string list resource of your
	   project, and then call AddDialogControls() in your WM_CREATE/
	   WM_INITDIALOG message handler. Can it *be* more simple??? */
	void AddDialogControls(bool fSubClass=false);

	/* If you don't have direct access to the message loop, you can
	   call this to make CNxSToolTip install it's own hook to intercept
	   messages for the Tooltip control. */
	void InstallHook(void);
	void UninstallHook(void); //Remove the hook installed by InstallHook


	//- properties -
	HWND GetTooltipWnd(void) { return m_hwnd; } //Returns hwnd of Tooltip control.
	LONG SetStyle(LONG style); //TTS_*; returns old style
	LONG GetStyle(void);
	HWND GetOwnerWnd(void) { return m_hwndOwner; }
	HWND SetOwnerWnd(HWND hwndNewOwner); //Returns previous owner wnd. Used in
                                             //subsequent calls to AddTool, DelTool, etc...
                                             //Note: Uses IsWindow() to validate hwndNewOwner.

  protected:
	HWND m_hwnd;
	HWND m_hwndOwner;
	HINSTANCE m_hinst;
	bool m_fSubClass; //Used by AddDialogControls()

  private:
	HWND m_hwndDlg;

	// These functions simplify making over-loaded methods.
	BOOL _AddTool(UINT uFlags, UINT uId, LPRECT prect, LPTSTR lpszText, HINSTANCE hinst);
	BOOL _SetToolInfo(UINT uFlags, UINT uId, LPRECT prect, LPTSTR lpszText, HINSTANCE hinst);
	void _UpdateTipText(UINT uId, char* lpszText, HINSTANCE hinst);

	static BOOL WINAPI AddToolInfo_EnumChild(HWND hwndCtrl, LPARAM lParam);
	//This static function is used by InstallHook
	static LRESULT CALLBACK CNxSToolTip::GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);

};
