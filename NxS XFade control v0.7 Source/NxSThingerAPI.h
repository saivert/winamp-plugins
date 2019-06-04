/*
NxS Thinger API Header file
Note: Please read the readme file before attempting to use this API.
Written by Saivert
http://inthegray.com/saivert/
*/

#ifndef _NXSTHINGERAPI_H
#define _NXSTHINGERAPI_H 1

#ifdef __cplusplus
extern "C" {
#endif


/* Flags to use for the dwFlags member of NxSThingerIconStruct */
#define NTIS_ADD          1
#define NTIS_MODIFY       2
#define NTIS_DELETE       4
#define NTIS_NOMSG        8
#define NTIS_NOICON       16
#define NTIS_NODESC       32
#define NTIS_HIDDEN       64
#define NTIS_BITMAP       128

typedef struct _NxSThingerIconStruct {
  DWORD dwFlags; /* NTIS_* flags */
  UINT uIconId; /* Only used for NTIS_MODIFY and NTIS_DELETE flags */
  LPTSTR lpszDesc;
  /* These are HBITMAP if the NTIS_BITMAP flag is used. */
  union {
	HICON hIcon;
	HBITMAP hBitmap;
  };
  union {
	HICON hIconHighlight;
	HBITMAP hBitmapHighlight;
  };

  /* Following is the message to send when icon is clicked */
  HWND hWnd; /* Set to NULL to send to Winamp */
  UINT uMsg;
  WPARAM wParam;
  LPARAM lParam;
} NxSThingerIconStruct, * lpNxSThingerIconStruct;

#ifdef __cplusplus
}
#endif

#endif /* _NXSTHINGERAPI_H */
