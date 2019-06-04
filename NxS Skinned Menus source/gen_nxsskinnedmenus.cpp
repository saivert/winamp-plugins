// gen_template.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "gen_nxsskinnedmenus.h"

#include "ConfigDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#define SHOW_MESSAGE_BOXES

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// gen_template

#include "../../winamp mfc template/gen.h"
#include "../../winamp mfc template/wa_ipc.h"
#define WA_DLG_IMPLEMENT
#include "../../winamp mfc template/wa_dlg.h"


winampGeneralPurposePlugin plugin = {GPPHDR_VER, // version of gen.h header
									 "",
									 gen_nxsskinnedmenus::Load,
									 gen_nxsskinnedmenus::Config,
									 gen_nxsskinnedmenus::Unload,};

CWinampWnd g_wawnd;

//-------------------------------------------------------------------------
// winampGetGeneralPurposePlugin
//
// Returns the pointer to the entry point functions of this plug-in to Winamp
//-------------------------------------------------------------------------
winampGeneralPurposePlugin*
winampGetGeneralPurposePlugin()
{
	return &plugin;
}


BEGIN_MESSAGE_MAP(gen_nxsskinnedmenus, CWinApp)
	//{{AFX_MSG_MAP(gen_nxsskinnedmenus)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// gen_template construction

gen_nxsskinnedmenus::gen_nxsskinnedmenus()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only gen_template object

gen_nxsskinnedmenus theApp;

//-------------------------------------------------------------------------
// Load
//
// This is a static function and you should not put any functionality
// for the plug-in in here. Put it in the called function.
//-------------------------------------------------------------------------
int gen_nxsskinnedmenus::Load()
{
	// This call is very important for maintaining
	// the state of the plug-in. See the comments at
	// the top of the file
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	return(theApp.Initialize());
}

//-------------------------------------------------------------------------
// Config
//
// This is a static function and you should not put any functionality
// for the plug-in in here. Put it in the called function.
//-------------------------------------------------------------------------
void gen_nxsskinnedmenus::Config()
{
	// This call is very important for maintaining
	// the state of the plug-in. See the comments at
	// the top of the file
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	theApp.Configure();
}

//-------------------------------------------------------------------------
// Unload
//
// This is a static function and you should not put any functionality
// for the plug-in in here. Put it in the called function.
//-------------------------------------------------------------------------
void gen_nxsskinnedmenus::Unload()
{
	// This call is very important for maintaining
	// the state of the plug-in. See the comments at
	// the top of the file
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	theApp.Deinitialize();
}

//-------------------------------------------------------------------------
// Initialize
//-------------------------------------------------------------------------
int gen_nxsskinnedmenus::Initialize()
{
#if defined(SHOW_MESSAGE_BOXES)
	AfxMessageBox(_T("gen_nxsskinnedmenus Initialize()"));
#endif

	static CString s;
	s = "NxS Skinned Menus v0.1 (using MFC)";

	plugin.description = s.GetBuffer();

	AfxEnableControlContainer();

	WADlg_init(plugin.hwndParent);


	CSkinMenuMgr::Initialize(SKMS_FLAT, 8, FALSE);

	CSkinMenuMgr::SetColor(COLOR_MENU, WADlg_getColor(WADLG_ITEMBG));
	CSkinMenuMgr::SetColor(COLOR_WINDOWTEXT, WADlg_getColor(WADLG_ITEMFG));
	CSkinMenuMgr::SetColor(COLOR_HIGHLIGHTTEXT, WADlg_getColor(WADLG_ITEMFG));
	CSkinMenuMgr::SetColor(COLOR_3DHIGHLIGHT, WADlg_getColor(WADLG_SELCOLOR));

	CSkinMenu::SetRenderer(this);

	g_wawnd.HookWindow(plugin.hwndParent);
	

	return(0);// return zero for no error
}

//-------------------------------------------------------------------------
// Configure
//-------------------------------------------------------------------------
void gen_nxsskinnedmenus::Configure()
{
	CConfigDlg dlg;

	if(IDOK == dlg.DoModal())
	{
	}

}

//-------------------------------------------------------------------------
// Deinitialize
//-------------------------------------------------------------------------
void gen_nxsskinnedmenus::Deinitialize()
{
#if defined(SHOW_MESSAGE_BOXES)
	AfxMessageBox(_T("gen_nxsskinnedmenus Deinitialize()"));
#endif

	g_wawnd.HookWindow(NULL);
}

BOOL gen_nxsskinnedmenus::DrawMenuClientBkgnd(CDC* pDC, LPRECT pRect, LPRECT pClip)
{
	COLORREF crFrom = CSkinMenuMgr::GetColor(COLOR_MENU);
	COLORREF crTo = CSkinMenuMgr::GetColor(COLOR_3DHIGHLIGHT);

	if (pClip)
	{
		// ensure that pClip is at least 100 pixels high else the 
		// gradient has artifacts
		CRect rClip(pClip), rRect(pRect);

		if (rClip.Height() < 100 && rRect.Height() > 100)
		{
			rClip.InflateRect(0, (min(rRect.Height(), 100) - rClip.Height()) / 2);

			if (rClip.top < rRect.top)
				rClip.OffsetRect(0, rRect.top - rClip.top);

			else if (rClip.bottom > rRect.bottom)
				rClip.OffsetRect(0, rRect.bottom - rClip.bottom);
		}

		float fHeight = (float)rRect.Height();

		float fFromFactor = (pRect->bottom - rClip.top) / fHeight;
		float fToFactor = (pRect->bottom - rClip.bottom) / fHeight;

		crFrom = CSkinBase::BlendColors(crFrom, crTo, fFromFactor);
		crTo = CSkinBase::BlendColors(crFrom, crTo, fToFactor);

		CSkinBase::GradientFill(pDC, rClip, crFrom, crTo, FALSE);
	}
	else
		CSkinBase::GradientFill(pDC, pRect, crFrom, crTo, FALSE);

	return TRUE;
}


LRESULT CWinampWnd::WindowProc(HWND hRealWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	LRESULT res = CSubclassWnd::WindowProc(hRealWnd, msg, wp, lp);
	if (msg==WM_DISPLAYCHANGE && wp==0 && lp==0) {
		// Time for a skin refresh. Update colors
		WADlg_init(plugin.hwndParent);
		//CSkinMenuMgr::Initialize(SKMS_FLAT, 8, FALSE);
		CSkinMenuMgr::SetColor(COLOR_MENU, WADlg_getColor(WADLG_ITEMBG));
		CSkinMenuMgr::SetColor(COLOR_WINDOWTEXT, WADlg_getColor(WADLG_ITEMFG));
		CSkinMenuMgr::SetColor(COLOR_HIGHLIGHTTEXT, WADlg_getColor(WADLG_ITEMFG));
		CSkinMenuMgr::SetColor(COLOR_3DHIGHLIGHT, WADlg_getColor(WADLG_SELCOLOR));
		
	}
	return res;
}
