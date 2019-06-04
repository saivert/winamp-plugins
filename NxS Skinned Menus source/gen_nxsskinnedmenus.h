// gen_nxsskinnedmenus.h : main header file for the GEN_TEMPLATE DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

#include "..\skinwindows\subclass.h"
#include "..\skinwindows\skinmenumgr.h"
#include "..\skinwindows\subclass.h"

/////////////////////////////////////////////////////////////////////////////
// gen_template
// See gen_template.cpp for the implementation of this class
//

class gen_nxsskinnedmenus : public CWinApp, protected ISkinMenuRender
{
public:
	static void Unload();
	static void Config();
	static int Load();
	gen_nxsskinnedmenus();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(gen_nxsskinnedmenus)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(gen_template)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
protected:
	void Deinitialize();
	void Configure();
	int Initialize();

	BOOL DrawMenuClientBkgnd(CDC* pDC, LPRECT pRect, LPRECT pClip);

};


class CWinampWnd : public CSubclassWnd
{
protected:
	LRESULT WindowProc(HWND hRealWnd, UINT msg, WPARAM wp, LPARAM lp);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

