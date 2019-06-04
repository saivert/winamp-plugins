/* misc.c: This source file contains a lot of utility functions
** some of them essential (but not all) used by NxS Balloon Tip.
**
** Included:  char *GetFormattedTitleFromWinamp(char *fmtspec)
** This is a very *nice* function that utilizes Winamp's own
** title formatter to format our own stuff. Now we finally got
** "$abbr()", "$if()" and "[]" support baby!
*/

#include "stdafx.h"
#include <commctrl.h>
#include "resource.h"
#include "misc.h"
#include "ml.h"
#include <time.h>

#define MAKE_UPPER(c) (char)LOWORD(CharUpper((LPTSTR) MAKELONG(c, 0)))
#define MAKE_LOWER(c) (char)LOWORD(CharLower((LPTSTR) MAKELONG(c, 0)))

/* All these declared in nxsballoontip.cpp */
extern int g_volchanged;
extern HWND g_hwndML;
extern int config_useml;

/* This function parses the common C/C++ string escapes */
int parse_escapes(char *in, char *out) {
	const char *p;
	unsigned int i=0;
	p=in;

	while (p && (*p != 0)) {
		if (*p=='\\') {
			switch (*(p+1)) {
			case '\\': out[i++] = '\\'; break; /* backslash */
			case 'r':  out[i++] = '\r'; break; /* return */
			case 'n':  out[i++] = '\n'; break; /* newline */
			case 't':  out[i++] = '\t'; break; /* tab */
			case 'b': if (i > 0) out[--i] = 0; break; /* backspace */
			default: out[i++] = *p; out[i++] = *(p+1);
			}
			++p;
		} else {
			/* All other characters, just copy verbatim to out */
			out[i++] = *p;
		}
		++p; /* Goto next */
	}
	out[i]=0; /* terminate string */
	return lstrlen(out);
}

char* myTagFunc(char *tag, void *p) {
	char curfile[MAX_PATH];
	char *file;
	char *retbuf;
	extendedFileInfoStruct efi;
	char num[64]; /* Used for simple itoa() */
	UINT plpos;

	/* Get currently playing file */
	plpos = SendMessage((HWND)p, WM_WA_IPC, 0, IPC_GETLISTPOS);
	file = (char*)SendMessage((HWND)p, WM_WA_IPC, plpos, IPC_GETPLAYLISTFILE);
	/* Check for existing filename */
	if (file) {
		lstrcpyn(curfile, file, MAX_PATH);
	} else {
		/* I'm using the VIDEO_MAKETYPE here because it's a quick way
		   to turn four chars into a DWORD. */
		(*(LPDWORD)curfile) = VIDEO_MAKETYPE('N','O','N','E');
		curfile[4] = '\0';
	}

	retbuf=(char*)GlobalAlloc(GPTR, 1024);
	retbuf[0]=0;
	
	if (!lstrcmpi(tag, "filename")) {
		lstrcpy(retbuf, curfile);
	} else if (!lstrcmpi(tag, "filedir")) {
		char *p=curfile+lstrlen(curfile);
		while (p && *p != '\\') p--;
		if (p > curfile) *p=0;
		lstrcpy(retbuf, curfile);
	} else if (!lstrcmpi(tag, "pllen")) {
		wsprintf(num, "%d", SendMessage((HWND)p, WM_WA_IPC, 0, IPC_GETLISTLENGTH));
		lstrcpy(retbuf, num);
	} else if (!lstrcmpi(tag, "plpos")) {
		wsprintf(num, "%d", 1+plpos);
		lstrcpy(retbuf, num);
	} else if (!lstrcmpi(tag, "plzeroidx")) {
		wsprintf(num, "%d", plpos);
		lstrcpy(retbuf, num);
	} else if (!lstrcmpi(tag, "length")) {
		char szLen[16];
		int s, m, h, length;

		length = SendMessage((HWND)p, WM_WA_IPC, 1,IPC_GETOUTPUTTIME);
		s = length % 60;
		m = (length / 60) % 60;
		h = (length / 60) / 60;
		if (h > 0) wsprintf(szLen, "%.2d:%.2d:%.2d", h, m, s);
		else wsprintf(szLen, "%.2d:%.2d", m, s);
		lstrcpy(retbuf, szLen);
	} else if (!lstrcmpi(tag, "bitrate")) {
		wsprintf(num, "%d", SendMessage((HWND)p, WM_WA_IPC, 1, IPC_GETINFO));
		lstrcpy(retbuf, num);
	} else if (!lstrcmpi(tag, "channelnum")) {
		wsprintf(num, "%d", SendMessage((HWND)p, WM_WA_IPC, 2, IPC_GETINFO));
		lstrcpy(retbuf, num);
	} else if (!lstrcmpi(tag, "channels")) {
		lstrcpy(retbuf, SendMessage((HWND)p, WM_WA_IPC, 2, IPC_GETINFO)-1?"stereo":"mono");
	} else if (!lstrcmpi(tag, "srate")) {
		wsprintf(num, "%d", SendMessage((HWND)p, WM_WA_IPC, 0, IPC_GETINFO));
		lstrcpy(retbuf, num);
	} else if (!lstrcmpi(tag, "videosize")) {
		LRESULT lRes;
		lRes = SendMessage((HWND)p, WM_WA_IPC, 3, IPC_GETINFO);
		wsprintf(num, "%dx%d", LOWORD(lRes), HIWORD(lRes));
		lstrcpyn(retbuf, num, sizeof(num)-1);
	} else if (!lstrcmpi(tag, "videowidth")) {
		LRESULT lRes;
		lRes = SendMessage((HWND)p, WM_WA_IPC, 3, IPC_GETINFO);
		wsprintf(num, "%d", LOWORD(lRes));
		lstrcpyn(retbuf, num, sizeof(num)-1);
	} else if (!lstrcmpi(tag, "videoheight")) {
		LRESULT lRes;
		lRes = SendMessage((HWND)p, WM_WA_IPC, 3, IPC_GETINFO);
		wsprintf(num, "%d", HIWORD(lRes));
		lstrcpyn(retbuf, num, sizeof(num)-1);
	} else if (!lstrcmpi(tag, "videoinfo")) {
		LRESULT lRes;
		lRes = SendMessage((HWND)p, WM_WA_IPC, 4, IPC_GETINFO);
		if (lRes > 65536)
			lstrcpyn(retbuf, (char*)lRes, 1024);
	} else if (!config_useml && (!lstrcmpi(tag, "year2d") || !lstrcmpi(tag, "twodigityear"))) {
		ZeroMemory(retbuf, sizeof(retbuf)-1);
		efi.filename = curfile;
		efi.metadata = "year";
		efi.ret = retbuf;
		efi.retlen = 1024;
		if (SendMessage((HWND)p, WM_WA_IPC, (WPARAM)&efi, IPC_GET_EXTENDED_FILE_INFO_HOOKABLE) && (lstrlen(retbuf) >= 2))
			lstrcpyn(retbuf, (retbuf+lstrlen(retbuf))-2, 1024);
	} else if (!lstrcmpi(tag, "pbstate")) {
		/* 0 = stopped, 1 = playing & 2 = paused */
		int pbstate=SendMessage((HWND)p, WM_WA_IPC, 0, IPC_ISPLAYING);
		if (pbstate==3) pbstate = 2; /* 3 --> 2 */
		wsprintf(retbuf, "%d", pbstate);
	} else if (!lstrcmpi(tag, "watitle")) {
		/* Experimental!! */
		waHookTitleStruct hooktitle;
		hooktitle.filename = curfile;
		hooktitle.title = (char*)GlobalAlloc(GPTR, 2048);
		if (SendMessage((HWND)p, WM_WA_IPC, (WPARAM)&hooktitle, IPC_HOOK_TITLES)) {
			lstrcpyn(retbuf, hooktitle.title, 1024);
		} else {
			lstrcpyn(retbuf, (char*)SendMessage((HWND)p, WM_WA_IPC, plpos, IPC_GETPLAYLISTTITLE), 1024);
		}
		GlobalFree((HGLOBAL)hooktitle.title);
	} else if (!lstrcmpi(tag, "rating")) {
		int rating;

		if (config_useml) {
			rating=SendMessage(g_hwndML, WM_ML_IPC, 0, ML_IPC_GETRATING);
		} else {
			rating=SendMessage((HWND)p, WM_WA_IPC, 0, IPC_GETRATING);
		}

		wsprintf(retbuf, "%d", rating);
	} else if (!lstrcmpi(tag, "ratingstars")) {
		int rating;
		int i;
		
		if (config_useml) {
			rating=SendMessage(g_hwndML, WM_ML_IPC, 0, ML_IPC_GETRATING);
		} else {
			rating=SendMessage((HWND)p, WM_WA_IPC, 0, IPC_GETRATING);
		}

		for (i=0; i < rating; i++) {
			retbuf[i]='*';
		}
		retbuf[rating+1]=0;
	} else if (!lstrcmpi(tag, "volume")) {
		int vol=SendMessage((HWND)p, WM_WA_IPC, -666, IPC_SETVOLUME);
		wsprintf(retbuf, "%d", (vol * 100) / 255);
	} else if (!lstrcmpi(tag, "panning")) {
		int pan=SendMessage((HWND)p, WM_WA_IPC, -666, IPC_SETPANNING);
		wsprintf(retbuf, "%s%d", pan>0?"+":"", (pan * 100) / 127);
	} else if (!lstrcmpi(tag, "volchanged")) {
		/* Set volchanged to 1 to indicate volume/balance just changed.
		   Otherwise make it a blank string.*/
		if (g_volchanged)
		{
			(*(LPDWORD)retbuf) = VIDEO_MAKETYPE('1','\0','\0','\0');
		}
	} else if (!lstrcmpi(tag, "nextsong")) {
		basicFileInfoStruct bfi;
		int pos=SendMessage((HWND)p, WM_WA_IPC, 0, IPC_GET_NEXT_PLITEM);
		int length=SendMessage((HWND)p, WM_WA_IPC, 0, IPC_GETLISTLENGTH);
		if (pos < 0) pos=SendMessage((HWND)p, WM_WA_IPC, 0, IPC_GETLISTPOS)+1;
		if (pos >= 0 && pos < length)
		{
			bfi.filename = (char*)SendMessage((HWND)p, WM_WA_IPC, pos, IPC_GETPLAYLISTFILE);
			bfi.quickCheck = 0;
			bfi.title = retbuf;
			bfi.titlelen = 1024;
			SendMessage((HWND)p, WM_WA_IPC, (WPARAM)&bfi, IPC_GET_BASIC_FILE_INFO);
		}
	} else if (!lstrcmpi(tag, "prevsong")) {
		basicFileInfoStruct bfi;
		int pos=SendMessage((HWND)p, WM_WA_IPC, 0, IPC_GET_PREVIOUS_PLITEM);
		int length=SendMessage((HWND)p, WM_WA_IPC, 0, IPC_GETLISTLENGTH);
		if (pos < 0) pos=SendMessage((HWND)p, WM_WA_IPC, 0, IPC_GETLISTPOS)-1;
		if (pos >= 0 && pos < length)
		{
			bfi.filename = (char*)SendMessage((HWND)p, WM_WA_IPC, pos, IPC_GETPLAYLISTFILE);
			bfi.quickCheck = 0;
			bfi.title = retbuf;
			bfi.titlelen = 1024;
			SendMessage((HWND)p, WM_WA_IPC, (WPARAM)&bfi, IPC_GET_BASIC_FILE_INFO);
		}
	} else if (!lstrcmpi(tag, "ishttp")) {
		retbuf[0] = VIDEO_MAKETYPE('\0','\0','\0','\0');
		if (strstr(curfile, "http://")) {
			retbuf[0] = '1';
		}
	} else {

		if (config_useml) {
			/* Try getting info from Media Library */
			mlQueryStruct mlQ;
			mlQ.query=curfile;
			mlQ.max_results=1;
			mlQ.results.Alloc=0;
			mlQ.results.Items=NULL;
			mlQ.results.Size=0;

			if (SendMessage(g_hwndML, WM_ML_IPC, WPARAM(&mlQ), ML_IPC_DB_RUNQUERY_FILENAME) == 1) {
				if (!lstrcmpi(tag, "artist")) {
					lstrcpyn(retbuf, mlQ.results.Items[0].artist, 1024);
				} else if (!lstrcmpi(tag, "title")) {
					lstrcpyn(retbuf, mlQ.results.Items[0].title, 1024);
				} else if (!lstrcmpi(tag, "filename")) {
					lstrcpyn(retbuf, mlQ.results.Items[0].filename, 1024);
				} else if (!lstrcmpi(tag, "album")) {
					lstrcpyn(retbuf, mlQ.results.Items[0].album, 1024);
				} else if (!lstrcmpi(tag, "comment")) {
					lstrcpyn(retbuf, mlQ.results.Items[0].comment, 1024);
				} else if (!lstrcmpi(tag, "genre")) {
					lstrcpyn(retbuf, mlQ.results.Items[0].genre, 1024);
				} else if (!lstrcmpi(tag, "trackno")) {
					wsprintf(retbuf, "%d", mlQ.results.Items[0].track);
				} else if (!lstrcmpi(tag, "year")) {
					if (mlQ.results.Items[0].year >= 0)
						wsprintf(retbuf, "%d", mlQ.results.Items[0].year);
				} else if (!lstrcmpi(tag, "tracknumber")) {
					wsprintf(retbuf, "%.2d", mlQ.results.Items[0].track);
				} else if (!lstrcmpi(tag, "trackno")) {
					wsprintf(retbuf, "%d", mlQ.results.Items[0].track);
				} else if (!lstrcmpi(tag, "lastplay") || !lstrcmpi(tag, "lastupd") || !lstrcmpi(tag, "filetime")) {
					/* These items is in string format and represents a time_t value.
					   Convert to actual integer/time_t and format as human-readable string. */
					char *pszLP = getRecordExtendedItem(&mlQ.results.Items[0], tag);
					if (pszLP) {
						time_t tt = (time_t)myatoi(pszLP);
						lstrcpyn(retbuf, ctime(&tt), 1024);
						/* ctime() appends a newline character for some reason, so remove it */
						retbuf[lstrlen(retbuf)-1]=0;
					}
				} else {
					/* Try to get data from extended_info field */
					char *tmp = getRecordExtendedItem(&mlQ.results.Items[0], tag);
					if (tmp != NULL && lstrlen(tmp) != 0)
						lstrcpyn(retbuf, tmp, 1024);
				}
			}
			SendMessage(g_hwndML, WM_ML_IPC, WPARAM(&mlQ), ML_IPC_DB_FREEQUERYRESULTS);
		}

		if (lstrlen(retbuf)) {
			return retbuf;
		} else {
			/* Not in Media Library, try metadata from input plugin */
			ZeroMemory(retbuf, sizeof(retbuf)-1);
			efi.filename = curfile;
			efi.metadata = tag;
			efi.ret = retbuf;
			efi.retlen = 1024;
			SendMessage(HWND(p), WM_WA_IPC, WPARAM(&efi), IPC_GET_EXTENDED_FILE_INFO_HOOKABLE);
		}
	}

	if (lstrlen(retbuf)) {
		return retbuf;
	}

	GlobalFree((HGLOBAL)retbuf);
	return 0;
}

void myTagFreeFunc(char *tag, void *p)
{
	GlobalFree((HGLOBAL)tag);
}


/* O T H E R   U T I L I T Y   F U N C T I O N S */

/* Copies a string representing the brand, version and build
   of the current OS (Windows) into out and returns length
   of string. It may look like this:
   "Microsoft Windows XP 5.1 (Build 2600: Service Pack 1)"
   or
   "Microsoft Windows 95 4.0 (Build 1111)"

   If out is NULL and/or outsize is smaller than resulting
   string length, no string is copied, but the length of
   the resulting string is still returned.
*/
int GetOSString(char *out, int outsize)
{
	char OSPlatform[256]={0,};
	int BuildNumber;
	char Result[256] = "Unknown Windows Version";
	OSVERSIONINFO osvi;
	
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	BuildNumber = osvi.dwBuildNumber;
	
	
	switch (osvi.dwPlatformId)
	{
    case VER_PLATFORM_WIN32_WINDOWS:
		{
			BuildNumber = osvi.dwBuildNumber & 0x0000FFFF;
			if (osvi.dwMinorVersion >= 0 && osvi.dwMinorVersion <= 9)
			{
				if (osvi.szCSDVersion[0] == 'B')
					lstrcpy(OSPlatform, "95 OSR2");
				else
					lstrcpy(OSPlatform, "95");
			}
			if (osvi.dwMinorVersion >= 10 && osvi.dwMinorVersion <= 89)
			{
				if (osvi.szCSDVersion[0] == 'A')
					lstrcpy(OSPlatform, "98");
				else
					lstrcpy(OSPlatform, "98 SE");
			}
			if (osvi.dwMinorVersion == 90)
				lstrcpy(OSPlatform, "Millennium");
		}
		break;
    case VER_PLATFORM_WIN32_NT:
		{
			if (osvi.dwMajorVersion == 3 || osvi.dwMajorVersion == 4)
				lstrcpy(OSPlatform, "NT");
			else if (osvi.dwMajorVersion == 5)
			{
				switch (osvi.dwMinorVersion)
				{
				case 0: lstrcpy(OSPlatform, "2000"); break;
				case 1: lstrcpy(OSPlatform, "XP");
				}
			}
			BuildNumber = osvi.dwBuildNumber;
		}
		break;
    case VER_PLATFORM_WIN32s:
		{
			lstrcpy(OSPlatform, "Win32s");
			BuildNumber = osvi.dwBuildNumber;
		}
	}
	if ((osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) ||
		(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT))
	{
		if (lstrlen(osvi.szCSDVersion) == 0)
			wsprintf(Result, "Windows %s %d.%d (Build %d)", OSPlatform, osvi.dwMajorVersion,
			osvi.dwMinorVersion, BuildNumber);
		else
			wsprintf(Result, "Windows %s %d.%d (Build %d: %s)", OSPlatform, osvi.dwMajorVersion,
			osvi.dwMinorVersion, BuildNumber, osvi.szCSDVersion);
	}
	else
		wsprintf(Result, "Windows %s %d.%d", OSPlatform, osvi.dwMajorVersion, osvi.dwMinorVersion);
	
	if (out && lstrlen(Result)<outsize)
		lstrcpy(out, Result);
	
	return lstrlen(Result);
}


/* We all love this one, right!? */
int myatoi(char *s)
{
  unsigned int v=0;
  if (*s == '0' && (s[1] == 'x' || s[1] == 'X'))
  {
    s+=2;
    for (;;)
    {
      int c=*s++;
      if (c >= '0' && c <= '9') c-='0';
      else if (c >= 'a' && c <= 'f') c-='a'-10;
      else if (c >= 'A' && c <= 'F') c-='A'-10;
      else break;
      v<<=4;
      v+=c;
    }
  }
  else if (*s == '0' && s[1] <= '7' && s[1] >= '0')
  {
    s++;
    for (;;)
    {
      int c=*s++;
      if (c >= '0' && c <= '7') c-='0';
      else break;
      v<<=3;
      v+=c;
    }
  }
  else
  {
    int sign=0;
    if (*s == '-') { s++; sign++; }
    for (;;)
    {
      int c=*s++ - '0';
      if (c < 0 || c > 9) break;
      v*=10;
      v+=c;
    }
    if (sign) return -(int) v;
  }
  return (int)v;
}
