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

#include "StdAfx.h"
#include "TrayHook.h"

HMODULE hMod;				// this DLL's module handle
HANDLE hMonitorEvent;
HANDLE hMonitorThread;
HHOOK hook;					// the hook's handle

// data shared between all instances of this DLL
#pragma data_seg(".AVETRAY")
HWND tray = NULL;			// the notification area window
HWND owner = NULL;			// the owner of this hook; this is were we forward to all WM_COPYDATA messages
BOOL hasSubclassed = FALSE;	// TRUE if we have subclassed the tray already
UINT unsubclassMsg = 0;		// messages used for notifying that we need to remove the subclass procedure.
BOOL inSendMessage = FALSE;
#pragma data_seg()
#pragma comment(linker, "/section:.AVETRAY,rws")

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		hMod = hModule;
		DisableThreadLibraryCalls(hModule);
	}
	return TRUE;
}

// Subclass procedure for the tray's window.
LRESULT CALLBACK MsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	// whenever we get a WM_COPYDATA message for the tray,
	// just pass it to the owner window.
	// if the Owner signals we need to drop the data (returning 1), we drop it,
	// otherwise we pass it on to the original procedure.
	if (hWnd == tray && WM_COPYDATA == uMsg && !inSendMessage)
	{
		if (owner)
		{
			LRESULT res = SendMessage(owner, uMsg, wParam, lParam);
			if (res == 1)
				return 0;
			if (res == 2)
			{
				COPYDATASTRUCT cds;
				TRAYNOTIFYDATA tnd;
				COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)lParam;
				TRAYNOTIFYDATA* ptnd = (TRAYNOTIFYDATA*)pcds->lpData;
				NOTIFYICONDATA32* pnid = (NOTIFYICONDATA32*)&ptnd->nid;
				cds.dwData = pcds->dwData;
				cds.cbData = pcds->cbData;
				cds.lpData = &tnd;
				tnd.dwMessage = ptnd->dwMessage;
				tnd.dwSignature = ptnd->dwSignature;
				memcpy(&tnd.nid, pnid, pnid->cbSize);
				tnd.nid.uFlags &= ~(NIF_INFO | NIF_REALTIME);
				inSendMessage = TRUE;
				SendMessage(hWnd, uMsg, (WPARAM)(HWND)wParam, (LPARAM)(LPVOID)&cds);
				inSendMessage = FALSE;
				return 0;
			}
		}
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// hook callback function
LRESULT CALLBACK CallWndProc(int code, WPARAM wParam, LPARAM lParam)
{
	// we examine all messages before they reach their destination window.
	// if the window is the tray, subclass the tray (from within the explorer process, thus!).
	// If we get a "stop subclassing" message, we unsubclass the tray again.
	// It's important this is done from this procedure, since this procedure runs
	// on the same thread as the tray window.
	CWPSTRUCT* cpw = (CWPSTRUCT*)lParam;
	if (cpw && cpw->hwnd == tray)
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

DWORD WINAPI TrayMonitor(LPVOID lpParameter)
{
	HANDLE hHandles[2];
	DWORD dwExitCode;

	hHandles[1] = OpenThread(SYNCHRONIZE, FALSE, (DWORD)lpParameter);
	if (!hHandles[1])
		return -1;
	hHandles[0] = hMonitorEvent;

	dwExitCode = WaitForMultipleObjects(_countof(hHandles), hHandles, FALSE, INFINITE);
	if (dwExitCode == 1)
	{
		hasSubclassed = FALSE;
		hook = NULL;
		PostMessage(owner, WM_TRAYCRASHED, 0, 0);
	}

	CloseHandle(hHandles[1]);
	return dwExitCode;
}

// method to start the hook
// returns whether the hook is currently running
__declspec(dllexport) BOOL APIENTRY StartHook(HWND hwnd)
{
	DWORD threadId;

	if (hMonitorThread)
	{
		if (WaitForSingleObject(hMonitorThread, 0) == WAIT_TIMEOUT)
			return TRUE;
		else
			StopHook();
	}

	if (unsubclassMsg == 0)
		unsubclassMsg = RegisterWindowMessage(_T("AveUnSubclassTrayPlease"));

	owner = hwnd;

	tray = FindWindow(_T("Shell_TrayWnd"), NULL);
	if (!tray)
		return FALSE;

	threadId = GetWindowThreadProcessId(tray, NULL);
	hook = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, hMod, threadId);
	if (hook)
	{
		hMonitorEvent = CreateEvent(NULL, FALSE, FALSE, _T("AveTrayMonitor"));
		if (!hMonitorEvent)
		{
			StopHook();
			return FALSE;
		}
		hMonitorThread = CreateThread(NULL, 0, TrayMonitor, (LPVOID)threadId, 0, NULL);
		if (!hMonitorThread)
		{
			StopHook();
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

// method to stop the hook
// whether the hook is currently stopped
__declspec(dllexport) void APIENTRY StopHook()
{
	if (hMonitorThread)
	{
		DWORD dwExitCode;
		SetEvent(hMonitorEvent);
		WaitForSingleObject(hMonitorThread, INFINITE);
		GetExitCodeThread(hMonitorThread, &dwExitCode);

		if (dwExitCode == 0)
		{
			SendMessage(tray, unsubclassMsg, 0, 0);
			UnhookWindowsHookEx(hook);
			hook = NULL;
		}

		CloseHandle(hMonitorThread);
		hMonitorThread = NULL;
		CloseHandle(hMonitorEvent);
		hMonitorEvent = NULL;
	}
	else
	{
		if (hMonitorEvent)
		{
			CloseHandle(hMonitorEvent);
			hMonitorEvent = NULL;
		}
		if (hook)
		{
			SendMessage(tray, unsubclassMsg, 0, 0);
			UnhookWindowsHookEx(hook);
			hook = NULL;
		}
	}
}
