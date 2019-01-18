// StdAfx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
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
#define _USE_MATH_DEFINES
#include <cmath>

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
#include "notifyicondata.h"
#pragma comment(lib, "shell32.lib")

#include <shlobj.h>
