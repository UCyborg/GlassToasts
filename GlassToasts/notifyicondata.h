#pragma once

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
