/* NxS Thinger API Header file
** Note: Please read the readme file before attempting to use this API.
**
** New in v0.512:
**   Bitmaps are now drawn transparently. All areas of the bitmaps that has the color
**   RGB(255,0,255) or Fuchsia will be transparent.
**
** New in v0.512:
**  - Better explanation of the API in this header file.
**  - Constant for the message string in this header file.
**
** Written by Saivert
** http://inthegray.com/saivert/
*/

#ifndef _NXSTHINGERAPI_H
#define _NXSTHINGERAPI_H 1

#ifdef __cplusplus
extern "C" {
#endif

#define NXSTHINGER_MSGSTR "NxSThingerIPCMsg"

/* Flags to use for the dwFlags member of NxSThingerIconStruct */
#define NTIS_ADD          1
#define NTIS_MODIFY       2
#define NTIS_DELETE       4
#define NTIS_NOMSG        8
#define NTIS_NOICON       16
#define NTIS_NODESC       32
#define NTIS_HIDDEN       64
#define NTIS_BITMAP       128

/* This is the structure you will be using to add your own icon to NxS Thinger control.
** You will be setting the dwFlags member with different flags depending on what you
** want to do:
** - Include the NTIS_ADD flag to add an icon. The NTIS_NO* flags are not valid here.
**   You can include the NTIS_BITMAP flags however to use a bitmap instead of an icon.
**   The parts of the bitmap that has the color RGB(255,0,255) (Fuchsia) will be transparent.
** - Include the NTIS_MODIFY flag to modify an icon. Here the NTIS_NO* flags are valid.
**   You include the NTIS_NODESC flag if you are not going to modify the description.
**   Include the NTIS_NOICON to not modify the icon/bitmap in use. The NTIS_BITMAP flag
**   is also used here to specify that you're using bitmap (HBITMAP) instead.
** - OR in the NTIS_NOMSG flag to leave the message (hWnd, uMsg, wParam, lParam) as it is.
** - Use the NTIS_HIDDEN flag (together with NTIS_ADD or NTIS_MODIFY) to hide the icon.
**   To show the icon, just don't include this flag when modifying the icon.
** - To delete an icon use the NTIS_DELETE flag.
*/
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
  HWND hWnd; /* Set to NULL to send to Winamp. From v0.51 on, this actually works. ;-) */
  UINT uMsg;
  WPARAM wParam;
  LPARAM lParam;
} NxSThingerIconStruct, * lpNxSThingerIconStruct;

#ifdef __cplusplus
}
#endif

#endif /* _NXSTHINGERAPI_H */
