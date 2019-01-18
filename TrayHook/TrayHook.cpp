// TrayHook.cpp : Defines the entry point for the DLL application.
//


// This DLL injects itself into explorer.exe by hooking
// into the window that makes up the notification area (tray)
// and tries to intercept "balloon popup" calls made to the tray
// and forward them to the glasstoast.exe application.

// Balloon notifications are shown by invoking the Shell_TrayIcon() API.
// This API just packages both parameters of the function call and
// sends them thru the use of WM_COPYDATA to the traywindow.
// This hook will see those WM_COPYDATA messages and pass them
// towards the glasstoast.exe main window. Further processing is then
// done in glasstoast.exe's main window (a different process).

// We actually intercept messages by subclassing the tray on
// first seeing a WM_COPYDATA message. By subclassing, we
// are able to drop WM_COPYDATA messages before they reach
// the tray, thus making the tray not see the requests made
// by Shell_TrayIcon() calls and thus not show balloons (we
// replaced them, after all).

#include "stdafx.h"

// data shared between all instances of this DLL
#pragma data_seg(".AVESHELLHOOK")
HWND tray;			// the notification area window
HMODULE hMod;		// this DLL's module handle
HHOOK hook;			// the hook's handle
HWND owner;			// the owner of this hook; this is were we forward to all WM_COPYDATA messages
BOOL hasSubclassed;	// TRUE if we have subclassed the tray already
UINT unsubclassMsg;		// messages used for notifying that we need to remove the subclass procedure.
#pragma data_seg()
#pragma comment(linker, "/section:.AVESHELLHOOK,rws")

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hModule);
	return TRUE;
}

// Subclass procedure for the tray's window.
LRESULT CALLBACK MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
	// whenever we get a WM_COPYDATA message for the tray,
	// just pass it to the owner window.
	// if the Owner signals we need to drop the data (returning 1), we drop it,
	// otherwise we pass it on to the original procedure.
	if (hwnd == tray && WM_COPYDATA == msg)
	{
		if (owner != NULL)
		{
			LRESULT res = SendMessage(owner, msg, wParam, lParam);
			if (1 == res)
				return 0;
		}
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// hook callback function
LRESULT CALLBACK CallWndProc(int code, WPARAM wParam, LPARAM lParam)
{
	// we examine all messages before they reach their destination window.
	// if the window is the tray, subclass the tray (from within the explorer process, thus!).
	// If we get a "stop subclassing" message, we unsubclass the tray again.
	// It's important this is done from this procedure, since this procedure runs
	// on the same thread as the tray window.
	CWPSTRUCT* cpw = reinterpret_cast<CWPSTRUCT*>(lParam);
	if (cpw != NULL && cpw->hwnd == tray)
	{
		if (unsubclassMsg == cpw->message)
		{
			if (hasSubclassed)
				hasSubclassed = !RemoveWindowSubclass(tray, MsgProc, 1);
		}
		else if (!hasSubclassed)
		{
			hasSubclassed = SetWindowSubclass(tray, MsgProc, 1, 0);
		}
	}

	return CallNextHookEx(hook, code, wParam, lParam);
}

// method to start the hook
BOOL CALLBACK StartHook(HMODULE hMod, HWND hwnd)
{
	if (hook != NULL)
		return TRUE;

	unsubclassMsg = RegisterWindowMessage(TEXT("AveUnSubclassTrayPlease"));

	owner = hwnd;

	tray = FindWindow(TEXT("Shell_TrayWnd"), NULL);
	if (NULL == tray)
		return FALSE;

	DWORD threadid = GetWindowThreadProcessId(tray, 0);
	hook = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, hMod, threadid);
	if (NULL == hook)
		return FALSE;

	return TRUE;
}

// method to stop the hook
BOOL CALLBACK StopHook()
{
	if (NULL == hook)
		return TRUE;

	SendMessage(tray, unsubclassMsg, 0, 0);

	BOOL res = UnhookWindowsHookEx(hook);
	if (res)
	{
		hook = NULL;
	}

	return res;
}
