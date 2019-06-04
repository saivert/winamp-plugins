// Contains function for "skinning" controls
// Version 0.2
// Written by Saivert
// Homepage: http://inthegray.com/saivert/
// E-Mail: saivert@email.com  or  saivert@gmail.com
// You can easily turn this into a native C++ class if you want to,
// since it basically has the structure of a class.

// This module is the only one to define WA_DLG_IMPLEMENT.
// WARNING: Do *not* define WA_DLG_IMPLEMENT in any other module.
#define WA_DLG_IMPLEMENT

// NOTE: wa_ipc.h and wa_dlg.h is included indirectly via ctrlskin.h
#include "ctrlskin.h"


// Forward declarations (or prototypes)
static LRESULT CALLBACK DialogSubclass(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK TrackBarSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK CheckBoxSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK GroupBoxSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL CALLBACK EnumWndProc(HWND hwnd, LPARAM lParam);
static LRESULT CALLBACK EmbedWndSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static INT_PTR WINAPI CtrlSkin_DialogBoxParam(IN HINSTANCE hInstance, IN LPCSTR lpTemplateName, IN HWND hWndParent, IN DLGPROC lpDialogFunc, IN LPARAM dwInitParam);


static HWND ctrlskin_hwndWinamp=0;

// The strings for our window properties
#define CTRLSKIN_PROP_OLDWNDPROC   "CtrlSkin_oldWndProc"
#define CTRLSKIN_PROP_DATAPTR      "CtrlSkin_DataPointer"
#define CTRLSKIN_PROP_EMBEDDEDWND  "CtrlSkin_EmbeddedWindow"

typedef struct _CtrlSkin_data {
	embedWindowState *ews;
	RECT orgRect;
	DWORD orgStyles;
	DWORD orgExStyles;
	int more[4];
} CtrlSkin_data, * lpCtrlSkin_data;

int CtrlSkin_Init(HWND hwndWinamp) {
	ctrlskin_hwndWinamp = hwndWinamp;
	return IsWindow(ctrlskin_hwndWinamp);
}

BOOL CtrlSkin_SkinControls(HWND hDlg, BOOL fActive) {

	// Fail the call if CtrlSkin is not initialized correctly.
	if (!IsWindow(ctrlskin_hwndWinamp))
		return FALSE;


	WADlg_init(ctrlskin_hwndWinamp);


	if (fActive) {
		// Are we already skinning this dialog?
		if (GetProp(hDlg, CTRLSKIN_PROP_OLDWNDPROC))
			return FALSE; // If so, fail the call.

		// Install the Subclass of the dialog
		SetProp(hDlg, CTRLSKIN_PROP_OLDWNDPROC,
			(HANDLE)SetWindowLongPtr(hDlg, GWLP_WNDPROC, (LONG)DialogSubclass)
		);
		InvalidateRect(hDlg, NULL, TRUE);
	} else {

		// Remove the Subclass of the dialog
		if (GetWindowLongPtr(hDlg, GWLP_WNDPROC) == (LONG)DialogSubclass) {
			SetWindowLongPtr(hDlg, GWLP_WNDPROC, (LONG_PTR)GetProp(hDlg, CTRLSKIN_PROP_OLDWNDPROC));
			RemoveProp(hDlg, CTRLSKIN_PROP_OLDWNDPROC);
			InvalidateRect(hDlg, NULL, TRUE);
		}
	}

	// Subclass all controls in the dialog
	EnumChildWindows(hDlg, EnumWndProc, fActive);

	return TRUE;
}


INT_PTR
WINAPI
CtrlSkin_DialogBoxParam(IN HINSTANCE hInstance, IN LPCSTR lpTemplateName,
						IN HWND hWndParent, IN DLGPROC lpDialogFunc, IN LPARAM dwInitParam) {

	HWND hDlg;
	MSG msg;

	hDlg = CreateDialogParam(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);

	while (IsWindow(hDlg) && GetMessage(&msg, 0, 0, 0)) {
		if (!IsDialogMessage(hDlg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	DestroyWindow(hDlg);

}


embedWindowState* CtrlSkin_EmbedWindow(HWND hDlg, BOOL fActive, UINT flags) {

	HWND (*embedWindow)(embedWindowState *);
	lpCtrlSkin_data data;
	TCHAR szCaption[256];

	// Fail the call if CtrlSkin is not initialized correctly.
	if (!IsWindow(ctrlskin_hwndWinamp))
		return NULL;

	if (fActive) {
		// Is the dialog already embedded?
		if (GetProp(hDlg, CTRLSKIN_PROP_DATAPTR))
			return NULL; // If so, fail the call.

		data = (lpCtrlSkin_data)GlobalAlloc(GPTR, sizeof(CtrlSkin_data));
		if (!data) return NULL;
		data->ews = (embedWindowState*)GlobalAlloc(GPTR, sizeof(embedWindowState));
		if (!data->ews) return NULL;
		data->ews->flags = flags;

		// Calculate the frame's bounding rectangle,
		// following the dialogs size and position.
		GetWindowRect(hDlg, &data->ews->r);
		CopyRect(&data->orgRect, &data->ews->r);
		data->ews->r.bottom = 29*2 + (data->ews->r.bottom & ~29);
		data->ews->r.right = 25*2 + (data->ews->r.right & ~25);

		// Get the pointer to the embedWindow function.
		// Calls SendMessage with IPC_GET_EMBEDIF Winamp message.
		*(void **)&embedWindow=(void*)SendMessage(ctrlskin_hwndWinamp, WM_WA_IPC, 0, IPC_GET_EMBEDIF);

		embedWindow(data->ews);
		data->ews->user_ptr = (void*)SetWindowLongPtr(data->ews->me, GWLP_WNDPROC, (LONG)EmbedWndSubclass);

		// Set a couple of window properties to remember certain things
		SetProp(hDlg, CTRLSKIN_PROP_DATAPTR, (HANDLE)data);
		SetProp(data->ews->me, CTRLSKIN_PROP_EMBEDDEDWND, (HANDLE)hDlg);

		// Set window title of frame
		GetWindowText(hDlg, szCaption, sizeof(szCaption));
		SetWindowText(data->ews->me, szCaption);

		// Get dialogs window styles
		data->orgStyles = (DWORD)GetWindowLong(hDlg, GWL_STYLE);
		data->orgExStyles = (DWORD)GetWindowLong(hDlg, GWL_EXSTYLE);

		// Now put the dialog inside the frame, but remember to put in the right styles or
		// else it just wont work.
		SetWindowLong(hDlg, GWL_STYLE, WS_CHILD|WS_VISIBLE);
		SetWindowLong(hDlg, GWL_EXSTYLE, WS_EX_CONTROLPARENT);
		SetParent(hDlg, data->ews->me);

		ShowWindow(data->ews->me, SW_SHOWNA);

		return data->ews;
	} else {
		// Remove frame

		// Get associated CtrlSkin_data struct
		data = (lpCtrlSkin_data)GetProp(hDlg, CTRLSKIN_PROP_DATAPTR);

		// Is the window embedded?
		if (!data || !data->ews || !IsWindow(data->ews->me))
			return NULL; // If not, say it's ok.

		// Restore frame's original window proc
		SetWindowLongPtr(data->ews->me, GWLP_WNDPROC, (LONG)data->ews->user_ptr);

		// Remove window properties
		RemoveProp(hDlg, CTRLSKIN_PROP_DATAPTR);
		RemoveProp(data->ews->me, CTRLSKIN_PROP_EMBEDDEDWND);

		ShowWindow(data->ews->me, SW_HIDE);

		// Move dialog out of frame's grip
		LockWindowUpdate(hDlg);
		ShowWindow(hDlg, SW_HIDE);
		SetParent(hDlg, NULL);
		SetWindowLong(hDlg, GWL_STYLE, data->orgStyles);
		SetWindowLong(hDlg, GWL_EXSTYLE, data->orgExStyles);
		MoveWindow(hDlg, data->orgRect.left, data->orgRect.top,
			data->orgRect.right-data->orgRect.left, data->orgRect.bottom-data->orgRect.top, TRUE);
		UpdateWindow(hDlg);
		InvalidateRect(hDlg, NULL, TRUE);
		LockWindowUpdate(NULL);
		ShowWindow(hDlg, SW_SHOW);
		

		// Destroy frame window and free our embedWindowStruct
		DestroyWindow(data->ews->me);
		GlobalFree((HGLOBAL)data->ews);
		GlobalFree((HGLOBAL)data);
	}
	
	return NULL;
}

BOOL CtrlSkin_IsEmbedded(HWND hWnd) {
  lpCtrlSkin_data data;

  data = (lpCtrlSkin_data)GetProp(hWnd, CTRLSKIN_PROP_DATAPTR);
  return (data && data->ews && IsWindow(data->ews->me));
}

BOOL CtrlSkin_SetForegroundWindow(HWND hWnd) {
  HWND hWndToUse;
  lpCtrlSkin_data data;

  data = (lpCtrlSkin_data)GetProp(hWnd, CTRLSKIN_PROP_DATAPTR);
  if (data && data->ews && IsWindow(data->ews->me))
	  hWndToUse = data->ews->me;
  else
	  hWndToUse = hWnd;

  return SetForegroundWindow(hWndToUse);
}

// This enumeration procedure is responsible for setting the subclass of all controls
// in the dialog. It stores the original window procedure pointer in GWL_USERDATA field.
static BOOL CALLBACK EnumWndProc(HWND hwnd, LPARAM lParam) {
	TCHAR szClass[64];
	GetClassName(hwnd, szClass, sizeof(szClass));

	if (lParam==1) {

		if ( !lstrcmpi(szClass, TRACKBAR_CLASS) ) {
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)
				SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG)TrackBarSubclass));
		} else if ( !lstrcmp(CharUpper(szClass), "BUTTON") ) {
			DWORD type = ((DWORD)GetWindowLong(hwnd, GWL_STYLE) & BS_TYPEMASK);

			if ((type == BS_CHECKBOX) || (type == BS_RADIOBUTTON) ||
				(type == BS_AUTOCHECKBOX) || (type == BS_AUTORADIOBUTTON)) {
				SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)
					SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG)CheckBoxSubclass));
			} else if (type == BS_GROUPBOX) {
				SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)
					SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG)GroupBoxSubclass));
			} else if (type == BS_PUSHBUTTON || type==BS_DEFPUSHBUTTON) {
				DWORD style;
				
				SetWindowLongPtr(hwnd, GWLP_USERDATA, GetWindowLong(hwnd, GWL_STYLE));

				// Convert all push buttons to owner-draw buttons
				style = (DWORD)GetWindowLong(hwnd, GWL_STYLE) & ~BS_PUSHBUTTON;
				SetWindowLong(hwnd, GWL_STYLE, style|BS_OWNERDRAW);
			} else {
				// Also put a backup of the styles for other button types
				SetWindowLongPtr(hwnd, GWLP_USERDATA, GetWindowLong(hwnd, GWL_STYLE));
			}
		}

	} else if (lParam==0) {

		if ( !lstrcmpi(szClass, TRACKBAR_CLASS) ) {
			if (GetWindowLongPtr(hwnd, GWLP_WNDPROC)==(LONG)TrackBarSubclass)
				SetWindowLongPtr(hwnd, GWLP_WNDPROC, GetWindowLongPtr(hwnd, GWLP_USERDATA));
		} else if ( !lstrcmp(CharUpper(szClass), "BUTTON") ) {
			DWORD type = ((DWORD)GetWindowLong(hwnd, GWL_STYLE) & BS_TYPEMASK);

			if ((type == BS_CHECKBOX) || (type == BS_RADIOBUTTON) ||
				(type == BS_AUTOCHECKBOX) || (type == BS_AUTORADIOBUTTON)) {
				if (GetWindowLongPtr(hwnd, GWLP_WNDPROC)==(LONG)CheckBoxSubclass)
					SetWindowLongPtr(hwnd, GWLP_WNDPROC, GetWindowLongPtr(hwnd, GWLP_USERDATA));
			} else if (type == BS_GROUPBOX) {
				if (GetWindowLongPtr(hwnd, GWLP_WNDPROC)==(LONG)GroupBoxSubclass)
					SetWindowLongPtr(hwnd, GWLP_WNDPROC, GetWindowLongPtr(hwnd, GWLP_USERDATA));
			} else {
				// Restore window styles
				SetWindowLong(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWLP_USERDATA));
			}
		}

	}
	return TRUE;
}


static LRESULT CALLBACK EmbedWndSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	embedWindowState *lpews;

	if (uMsg==WM_CLOSE) {
		PostMessage(GetProp(hWnd, CTRLSKIN_PROP_EMBEDDEDWND),
			WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), 0);
		return 0;
	}

	lpews = (embedWindowState *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	return CallWindowProc((WNDPROC)lpews->user_ptr, hWnd, uMsg, wParam, lParam);
}


// Subclass of dialog skinned
static LRESULT CALLBACK DialogSubclass(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	int a;
		
	a=WADlg_handleDialogMsgs(hDlg,uMsg,wParam,lParam);
	if (a) return a;

	// Winamp sends dialogs a bogus WM_DISPLAYCHANGE message when user switches to another skin.
	if (uMsg==WM_DISPLAYCHANGE && wParam==0 && lParam==0) {
		// User has changed skin: Reinit and redraw.
		WADlg_init(ctrlskin_hwndWinamp);
		InvalidateRect(hDlg, NULL, TRUE);
		return FALSE;
	}

	
#if 0
	if (GetProp(hDlg, CTRLSKIN_PROP_EWSPTR)) {
		// Things to do if we are embedded...
	}
#endif


	return CallWindowProc((WNDPROC)GetProp(hDlg, CTRLSKIN_PROP_OLDWNDPROC), hDlg, uMsg, wParam, lParam);
}


// Rest is subclass procedures for the various control classes skinned

static LRESULT CALLBACK TrackBarSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {


	static BOOL bDrawFocus=FALSE;
	static BOOL bIsDown=FALSE;

	PAINTSTRUCT ps;
	HDC hdc;
	HBRUSH hbr, holdbr;
	COLORREF bgcolor, fgcolor;
	COLORREF itembg, itemfg;
	RECT rc;
	HPEN hpen, holdpen;
	HBITMAP hbm, holdbm;
	HDC memdc;
	int cyBox=58-31;
	int cxBox=56-31;
	RECT r;
	POINT pt;

	if (uMsg==WM_LBUTTONDOWN || uMsg==WM_LBUTTONUP) {
		CallWindowProc((WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA), hwnd, TBM_GETTHUMBRECT, 0, (LPARAM) (LPRECT) &r);
		pt.x = LOWORD(lParam);
		pt.y = HIWORD(lParam);
		if (PtInRect(&r, pt) && uMsg==WM_LBUTTONDOWN) {
			bIsDown=TRUE;
		} else if (uMsg==WM_LBUTTONUP) {
			bIsDown=FALSE;
		}
	}

	// Take over the paintjob
	if (uMsg==WM_PAINT) {
		GetClientRect(hwnd, &rc);
		bgcolor = WADlg_getColor(WADLG_WNDBG);
		fgcolor = WADlg_getColor(WADLG_WNDFG);
		itembg = WADlg_getColor(WADLG_ITEMBG);
		itemfg = WADlg_getColor(WADLG_ITEMFG);
		hdc = BeginPaint(hwnd, &ps);
		SetBkColor(hdc, bgcolor);
		SetTextColor(hdc, fgcolor);
		hbr = CreateSolidBrush(bgcolor);
		FillRect(hdc, &rc, hbr);

		// Create and select pen into dc
		hpen = CreatePen(PS_SOLID, 0, fgcolor);
		holdpen = (HPEN)SelectObject(hdc, hpen);

		// Change brush
		DeleteObject(hbr);
		hbr = CreateSolidBrush(itembg);
		holdbr = (HBRUSH)SelectObject(hdc, hbr);
		
		// Draw trackbar channel
		CallWindowProc((WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA), hwnd, TBM_GETCHANNELRECT, 0, (LPARAM) (LPRECT) &r);
		// Rotate rectangle 90 degrees clock-wise if we are vertical
		if (GetWindowLong(hwnd, GWL_STYLE) & TBS_VERT) {
			RECT tmpr;
			tmpr = r;
			r.left = tmpr.bottom;
			r.bottom = tmpr.left;
			r.top = tmpr.right;
			r.right = tmpr.top;
		}
		Rectangle(hdc, r.left, r.top, r.right, r.bottom);

		// Draw trackbar thumb

		// Create a memory dc and select a bitmap into it
		hbm = WADlg_getBitmap();
		memdc = CreateCompatibleDC(hdc);
		holdbm = (HBITMAP)SelectObject(memdc, hbm);

		CallWindowProc((WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA), hwnd, TBM_GETTHUMBRECT, 0, (LPARAM) (LPRECT) &r);

		//SetStretchBltMode(hdc, COLORONCOLOR);
		SetStretchBltMode(hdc, HALFTONE);

		if (GetWindowLong(hwnd, GWL_STYLE) & TBS_VERT) {
			if (bIsDown) {
				StretchBlt(hdc, r.left, r.top, r.right-r.left, r.bottom-r.top, memdc, 84, 45, 28, 14, SRCCOPY);
			} else {
				StretchBlt(hdc, r.left, r.top, r.right-r.left, r.bottom-r.top, memdc, 84, 31, 28, 14, SRCCOPY);
			}
		} else {
			if (bIsDown) {
				StretchBlt(hdc, r.left, r.top, r.right-r.left, r.bottom-r.top, memdc, 70, 31, 14, 28, SRCCOPY);
			} else {
				StretchBlt(hdc, r.left, r.top, r.right-r.left, r.bottom-r.top, memdc, 56, 31, 14, 28, SRCCOPY);
			}
		}

		SelectObject(memdc, holdbm);
		DeleteDC(memdc);


		// Restore and delete pen
		SelectObject(hdc, holdpen);
		DeleteObject(hpen);
		// Restore and delete brush
		SelectObject(hdc, holdbr);
		DeleteObject(hbr);


		if ( GetFocus()==hwnd && bDrawFocus) {
			InflateRect(&rc, -1, -1);
			DrawFocusRect(hdc, &rc);
		}

		EndPaint(hwnd, &ps);
		return 0;
	}
 
	
	return CallWindowProc((WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA), hwnd, uMsg, wParam, lParam);
}


static LRESULT CALLBACK CheckBoxSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	static BOOL bDrawFocus=FALSE;

	PAINTSTRUCT ps;
	HDC hdc;
	HBRUSH hbr, holdbr;
	COLORREF bgcolor, fgcolor;
	COLORREF itembg, itemfg;
	RECT rc;
	HPEN hpen, holdpen;
	int cxBox, cyBox;
	TCHAR szLabel[256];
	LOGFONT lf;
	HFONT hf, oldhf;
	HBITMAP hbm;
	HDC memdc;
	RECT r;
	DWORD dwType;
	BOOL bIsRadioBtn;

	// Take over the paintjob
	if (uMsg==WM_PAINT) {
		dwType = ((DWORD)GetWindowLong(hwnd, GWL_STYLE) & BS_TYPEMASK);
		bIsRadioBtn = ((dwType==BS_RADIOBUTTON) || (dwType==BS_AUTORADIOBUTTON));
		GetWindowText(hwnd, szLabel, sizeof(szLabel));
		GetClientRect(hwnd, &rc);
		bgcolor = WADlg_getColor(WADLG_WNDBG);
		fgcolor = WADlg_getColor(WADLG_WNDFG);
		itembg = WADlg_getColor(WADLG_ITEMBG);
		itemfg = WADlg_getColor(WADLG_ITEMFG);
		hdc = BeginPaint(hwnd, &ps);
		hbr = CreateSolidBrush(bgcolor);
		FillRect(hdc, &rc, hbr);
		DeleteObject(hbr);

		// Draw checkbox rectangle
		cxBox = GetSystemMetrics(SM_CXMENUCHECK);
		cyBox = GetSystemMetrics(SM_CYMENUCHECK);

		hpen = (HPEN)CreatePen(PS_SOLID, 0, fgcolor);
		holdpen = (HPEN)SelectObject(hdc, hpen);

		r.left = 1;
		r.top = (rc.bottom-cyBox)/2;
		r.right = cxBox+1;
		r.bottom = cyBox+((rc.bottom-cyBox)/2);
		if (bIsRadioBtn) {
			// Set new brush for the Ellipse
			hbr = CreateSolidBrush(itembg);
			holdbr = (HBRUSH)SelectObject(hdc, hbr);
			Ellipse(hdc, r.left, r.top, r.right, r.bottom);
		} else {
			// Set new brush
			DeleteObject(hbr);
			hbr = CreateSolidBrush(itembg);
			FillRect(hdc, &r, hbr);
		}
		
		// Draw checkmark if checked
		if (SendMessage(hwnd, BM_GETCHECK, 0, 0)==BST_CHECKED) {
			if (bIsRadioBtn) {
				InflateRect(&r, -3, -3);
				// Set new pen for Ellipse
				DeleteObject(hpen);
				hpen = CreatePen(PS_SOLID, 0, itemfg);
				holdpen = (HPEN)SelectObject(hdc, hpen);
				// Set new brush for the Ellipse
				hbr = CreateSolidBrush(itemfg);
				holdbr = (HBRUSH)SelectObject(hdc, hbr);
				Ellipse(hdc, r.left, r.top, r.right, r.bottom);
			} else {
				// Set text and background colors for the checkmark
				SetBkColor(hdc, itembg);
				SetTextColor(hdc, itemfg);

				hbm = LoadBitmap(NULL, (LPCTSTR)32760); //OBM_CHECK
				memdc = CreateCompatibleDC(hdc);
				SelectObject(memdc, hbm);

				BitBlt(hdc, 2, (rc.bottom-cyBox)/2, cxBox-1, cyBox+((rc.bottom-cyBox)/2), memdc, 0, 0, SRCCOPY);

				DeleteObject(hbm);
				DeleteDC(memdc);
			}
		}

		if (!bIsRadioBtn) {
			// Set new brush
			DeleteObject(hbr);
			hbr = CreateSolidBrush(itemfg);
			FrameRect(hdc, &r, hbr); // Draw frame around checkmark
		}

		// Draw checkbox label
		ZeroMemory(&lf, sizeof(LOGFONT));
		lstrcpyn(lf.lfFaceName,
			(char*)SendMessage(ctrlskin_hwndWinamp, WM_WA_IPC, 1, IPC_GET_GENSKINBITMAP), LF_FACESIZE);
		lf.lfHeight = -SendMessage(ctrlskin_hwndWinamp, WM_WA_IPC, 3, IPC_GET_GENSKINBITMAP);
		lf.lfCharSet = (unsigned char)SendMessage(ctrlskin_hwndWinamp, WM_WA_IPC, 2, IPC_GET_GENSKINBITMAP);
		hf = CreateFontIndirect(&lf);

		oldhf = (HFONT)SelectObject(hdc, hf);
		SetBkColor(hdc, bgcolor);
		SetTextColor(hdc, fgcolor);
		TextOut(hdc, cxBox+2+3, 0, szLabel, lstrlen(szLabel));

		// Calculate focus rectangle
		r = rc;
		DrawText(hdc, szLabel, -1, &r, DT_SINGLELINE|DT_LEFT|DT_CALCRECT);

		// Restore and delete font
		SelectObject(hdc, oldhf);
		DeleteObject(hf);
		// Restore and delete pen
		SelectObject(hdc, holdpen);
		DeleteObject(hpen);
		// Restore and delete brush
		SelectObject(hdc, holdbr);
		DeleteObject(hbr);

		if (GetFocus()==hwnd && bDrawFocus) {
			OffsetRect(&r, cxBox+2+3, 0);
			DrawFocusRect(hdc, &r);
		}

		EndPaint(hwnd, &ps);
		return 0;
	}

	return CallWindowProc((WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA), hwnd, uMsg, wParam, lParam);
}


static LRESULT CALLBACK GroupBoxSubclass(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	PAINTSTRUCT ps;
	HDC hdc;
	HBRUSH hbr, holdbr;
	COLORREF bgcolor, fgcolor;
	COLORREF itembg, itemfg;
	RECT rc;
	HPEN hpen, holdpen;
	TCHAR szLabel[256];
	LOGFONT lf;
	HFONT hf, oldhf;
	RECT r;

	// Take over the paintjob
	if (uMsg==WM_PAINT) {
		GetWindowText(hwnd, szLabel, sizeof(szLabel));
		GetClientRect(hwnd, &rc);
		bgcolor = WADlg_getColor(WADLG_WNDBG);
		fgcolor = WADlg_getColor(WADLG_WNDFG);
		itembg = WADlg_getColor(WADLG_ITEMBG);
		itemfg = WADlg_getColor(WADLG_ITEMFG);
		hdc = BeginPaint(hwnd, &ps);
		hbr = CreateSolidBrush(bgcolor);
		FillRect(hdc, &rc, hbr);
		DeleteObject(hbr);

		// Set brush and pen
		hbr = CreateSolidBrush(itemfg);
		holdbr = SelectObject(hdc, hbr);
		
		hpen = (HPEN)CreatePen(PS_SOLID, 0, fgcolor);
		holdpen = (HPEN)SelectObject(hdc, hpen);

		// Draw groupbox rectangle
		r = rc;
		InflateRect(&r, -2, -2);
		r.top += 5;
		FrameRect(hdc, &r, hbr);
		
		// Draw checkbox label
		ZeroMemory(&lf, sizeof(LOGFONT));
		lstrcpyn(lf.lfFaceName,
			(char*)SendMessage(ctrlskin_hwndWinamp, WM_WA_IPC, 1, IPC_GET_GENSKINBITMAP), LF_FACESIZE);
		lf.lfHeight = -SendMessage(ctrlskin_hwndWinamp, WM_WA_IPC, 3, IPC_GET_GENSKINBITMAP);
		lf.lfCharSet = (unsigned char)SendMessage(ctrlskin_hwndWinamp, WM_WA_IPC, 2, IPC_GET_GENSKINBITMAP);
		hf = CreateFontIndirect(&lf);

		oldhf = (HFONT)SelectObject(hdc, hf);
		SetBkColor(hdc, bgcolor);
		SetTextColor(hdc, fgcolor);
		TextOut(hdc, r.left+5, 0, szLabel, lstrlen(szLabel));

		// Restore and delete font
		SelectObject(hdc, oldhf);
		DeleteObject(hf);
		// Restore and delete pen
		SelectObject(hdc, holdpen);
		DeleteObject(hpen);
		// Restore and delete brush
		SelectObject(hdc, holdbr);
		DeleteObject(hbr);

		EndPaint(hwnd, &ps);
		return 0;
	}

	return CallWindowProc((WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA), hwnd, uMsg, wParam, lParam);
}
