// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__C8B857C8_EA12_41C6_A335_C7E4B2DD1908__INCLUDED_)
#define AFX_STDAFX_H__C8B857C8_EA12_41C6_A335_C7E4B2DD1908__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define _WIN32_IE 0x600

// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>

// TODO: reference additional headers your program requires here
#include <commctrl.h>
#include <shellapi.h>

#define NO_IVIDEO_DECLARE
#include "wa_ipc.h"
#include "wa_hotkeys.h"
#include "ml.h"


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__C8B857C8_EA12_41C6_A335_C7E4B2DD1908__INCLUDED_)
