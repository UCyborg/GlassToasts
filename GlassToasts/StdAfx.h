// StdAfx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

// Change these values to use different versions
#define WINVER			0x0600
#define _WIN32_WINNT	0x0600
#define _WIN32_IE		0x0600
#define _RICHEDIT_VER	0x0100

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>

extern CAppModule _Module;

#if defined _M_IX86
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df'\"")
#elif defined _M_IA64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df'\"")
#elif defined _M_AMD64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df'\"")
#else
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df'\"")
#endif

#ifdef _UNICODE
#define __targv __wargv
#else
#define __targv __argv
#endif

int checkCmdLineParm(TCHAR *);

#include <queue>
#include <math.h>

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#include <psapi.h>
#pragma comment(lib, "psapi.lib")

#include <shellapi.h>

// NOTIFYICONDATA struct with H* members redefined as DWORDs
// we need this to be able to correctly read notification data on 64-bit OS
typedef struct _NOTIFYICONDATAA32 {
	DWORD cbSize;
	DWORD dwWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	DWORD dwIcon;
#if (NTDDI_VERSION < NTDDI_WIN2K)
	CHAR   szTip[64];
#endif
#if (NTDDI_VERSION >= NTDDI_WIN2K)
	CHAR   szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	CHAR   szInfo[256];
	union {
		UINT  uTimeout;
		UINT  uVersion;  // used with NIM_SETVERSION, values 0, 3 and 4
	} DUMMYUNIONNAME;
	CHAR   szInfoTitle[64];
	DWORD dwInfoFlags;
#endif
#if (NTDDI_VERSION >= NTDDI_WINXP)
	GUID guidItem;
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
	DWORD dwBalloonIcon;
#endif
} NOTIFYICONDATAA32, *PNOTIFYICONDATAA32;
typedef struct _NOTIFYICONDATAW32 {
	DWORD cbSize;
	DWORD dwWnd;
	UINT uID;
	UINT uFlags;
	UINT uCallbackMessage;
	DWORD dwIcon;
#if (NTDDI_VERSION < NTDDI_WIN2K)
	WCHAR  szTip[64];
#endif
#if (NTDDI_VERSION >= NTDDI_WIN2K)
	WCHAR  szTip[128];
	DWORD dwState;
	DWORD dwStateMask;
	WCHAR  szInfo[256];
	union {
		UINT  uTimeout;
		UINT  uVersion;  // used with NIM_SETVERSION, values 0, 3 and 4
	} DUMMYUNIONNAME;
	WCHAR  szInfoTitle[64];
	DWORD dwInfoFlags;
#endif
#if (NTDDI_VERSION >= NTDDI_WINXP)
	GUID guidItem;
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
	DWORD dwBalloonIcon;
#endif
} NOTIFYICONDATAW32, *PNOTIFYICONDATAW32;
#ifdef UNICODE
typedef NOTIFYICONDATAW32 NOTIFYICONDATA32;
typedef PNOTIFYICONDATAW32 PNOTIFYICONDATA32;
#else
typedef NOTIFYICONDATAA32 NOTIFYICONDATA32;
typedef PNOTIFYICONDATAA32 PNOTIFYICONDATA32;
#endif // UNICODE
#define NOTIFYICONDATAA32_V2_SIZE     FIELD_OFFSET(NOTIFYICONDATAA32, guidItem)
#define NOTIFYICONDATAW32_V2_SIZE     FIELD_OFFSET(NOTIFYICONDATAW32, guidItem)
#ifdef UNICODE
#define NOTIFYICONDATA32_V2_SIZE      NOTIFYICONDATAW32_V2_SIZE
#else
#define NOTIFYICONDATA32_V2_SIZE      NOTIFYICONDATAA32_V2_SIZE
#endif // UNICODE
// undocumented TRAYNOTIFYDATA struct
// we get pointer to this via WM_COPYDATA message
typedef struct _TRAYNOTIFYDATAA
{
	DWORD dwSignature;
	DWORD dwMessage;
	NOTIFYICONDATAA32 nid;
} TRAYNOTIFYDATAA, *PTRAYNOTIFYDATAA;
typedef struct _TRAYNOTIFYDATAW
{
	DWORD dwSignature;
	DWORD dwMessage;
	NOTIFYICONDATAW32 nid;
} TRAYNOTIFYDATAW, *PTRAYNOTIFYDATAW;
#ifdef UNICODE
typedef TRAYNOTIFYDATAW TRAYNOTIFYDATA;
typedef PTRAYNOTIFYDATAW PTRAYNOTIFYDATA;
#else
typedef TRAYNOTIFYDATAA TRAYNOTIFYDATA;
typedef PTRAYNOTIFYDATAA PTRAYNOTIFYDATA;
#endif // UNICODE

#pragma comment(lib, "shell32.lib")

#include <shlobj.h>
