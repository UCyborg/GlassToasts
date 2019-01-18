#include "StdAfx.h"
#include "TrayIconFinder.h"

TrayIconFinder::TrayIconFinder(void)
{
}

TrayIconFinder::~TrayIconFinder(void)
{
}

POINT TrayIconFinder::GetTrayIconPosition(HWND hwnd, UINT id)
{
	RECT rc;
	FindOutPositionOfIconDirectly(hwnd, id, rc);
	POINT pt = { rc.left, rc.top };
	return pt;
}

BOOL CALLBACK TrayIconFinder::FindTrayWnd(HWND hwnd, LPARAM lParam)
{
	TCHAR szClassName[256] = { 0 };
	int i = GetClassName(hwnd, szClassName, 255);    // Did we find the Main System Tray? If so, then get its size and quit
	if (i && !_tcscmp(szClassName, _T("TrayNotifyWnd")))
	{
		HWND* pWnd = (HWND*)lParam;
		*pWnd = hwnd;
		return FALSE;
	}

	return TRUE;
}

BOOL CALLBACK TrayIconFinder::FindToolBarInTrayWnd(HWND hwnd, LPARAM lParam)
{
	TCHAR szClassName[256] = { 0 };
	GetClassName(hwnd, szClassName, 255);    // Did we find the Main System Tray? If so, then get its size and quit
	if (!_tcscmp(szClassName, _T("ToolbarWindow32")))
	{
		HWND* pWnd = (HWND*)lParam;
		*pWnd = hwnd;
		return FALSE;
	}
	return TRUE;
}

HWND TrayIconFinder::GetTrayNotifyWnd(BOOL a_bSeekForEmbedToolbar)
{
	HWND hWndShellTrayWnd = FindWindow(_T("Shell_TrayWnd"), NULL);
	if (hWndShellTrayWnd)
	{
		HWND hWndTrayNotifyWnd = NULL;
		EnumChildWindows(hWndShellTrayWnd, TrayIconFinder::FindTrayWnd, (LPARAM)&hWndTrayNotifyWnd);

		if (hWndTrayNotifyWnd && IsWindow(hWndTrayNotifyWnd))
		{
			HWND hWndToolBarWnd = NULL;
			EnumChildWindows(hWndTrayNotifyWnd, TrayIconFinder::FindToolBarInTrayWnd, (LPARAM)&hWndToolBarWnd);
			if (hWndToolBarWnd)
			{
				return hWndToolBarWnd;
			}
		}

		return hWndTrayNotifyWnd;
	}

	return hWndShellTrayWnd;
}

RECT TrayIconFinder::GetTrayWndRect()
{
	HWND hWndTrayWnd = GetTrayNotifyWnd(FALSE);
	if (hWndTrayWnd)
	{
		RECT rect;
		GetWindowRect(hWndTrayWnd, &rect);
		return rect;
	}

	int nWidth = GetSystemMetrics(SM_CXSCREEN);
	int nHeight = GetSystemMetrics(SM_CYSCREEN);
	RECT rc = { nWidth - 40, nHeight - 20, nWidth, nHeight };

	return rc;
}

BOOL TrayIconFinder::FindOutPositionOfIconDirectly(const HWND a_hWndOwner, const int a_iButtonID, RECT& a_rcIcon)
{
	//first of all let's find a Tool bar control embed in Tray window
	HWND hWndTray = GetTrayNotifyWnd(TRUE);
	if (!hWndTray)
	{
		return FALSE;
	}

	//now we have to get an ID of the parent process for system tray
	DWORD dwTrayProcessID = 0;
	GetWindowThreadProcessId(hWndTray, &dwTrayProcessID);
	if (dwTrayProcessID == 0)
	{
		return FALSE;
	}

	HANDLE hTrayProc = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ, FALSE, dwTrayProcessID);
	if (!hTrayProc)
	{
		return FALSE;
	}

	//now we check how many buttons is there - should be more than 0
	int iButtonsCount = (int)SendMessage(hWndTray, TB_BUTTONCOUNT, 0, 0);

	//We want to get data from another process - it's not possible to just send messages like TB_GETBUTTON with a locally
	//allocated buffer for return data. Pointer to locally allocated data has no useful meaning in a context of another
	//process (since Win95) - so we need to allocate some memory inside Tray process.
	//We allocate sizeof(TBBUTTON) bytes of memory - because TBBUTTON is the biggest structure we will fetch. But this buffer
	//will be also used to get smaller pieces of data like RECT structures.
	LPVOID lpData;
	if (iButtonsCount > 0)
	{
		lpData = VirtualAllocEx(hTrayProc, NULL, sizeof(TBBUTTON), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

		if (!lpData)
		{
			CloseHandle(hTrayProc);
			return FALSE;
		}
	}
	else
	{
		CloseHandle(hTrayProc);
		return FALSE;
	}

	BOOL bIconFound = FALSE;

	for (int iButton = 0; iButton<iButtonsCount; iButton++)
	{
		//first let's read TBUTTON information about each button in a task bar of tray

		TBBUTTON buttonData;
		SendMessage(hWndTray, TB_GETBUTTON, iButton, (LPARAM)lpData);
		if (!ReadProcessMemory(hTrayProc, lpData, &buttonData, sizeof(TBBUTTON), NULL))
		{
			continue;
		}

		//now let's read extra data associated with each button: there will be a HWND of the window that created an icon and icon ID
		DWORD dwExtraData[2];
		if (!ReadProcessMemory(hTrayProc, (LPVOID)buttonData.dwData, dwExtraData, sizeof(dwExtraData), NULL))
		{
			continue;
		}

		HWND hWndOfIconOwner = (HWND)dwExtraData[0];
		int  iIconId = (int)dwExtraData[1];

		if (hWndOfIconOwner != a_hWndOwner || iIconId != a_iButtonID)
		{
			continue;
		}

		//we found our icon - in WinXP it could be hidden - let's check it:
		if (buttonData.fsState & TBSTATE_HIDDEN)
		{
			break;
		}

		//now just ask a tool bar of rectangle of our icon
		SendMessage(hWndTray, TB_GETITEMRECT, iButton, (LPARAM)lpData);

		RECT rcPosition;
		if (!ReadProcessMemory(hTrayProc, lpData, &rcPosition, sizeof(RECT), NULL))
		{
			continue;
		}

		MapWindowPoints(hWndTray, NULL, (LPPOINT)&rcPosition, 2);
		a_rcIcon = rcPosition;

		bIconFound = TRUE;
		break;
	}

	if (!bIconFound)
	{
		a_rcIcon = GetTrayWndRect(); //we failed to detect position of icon - let's return fail safe coordinates of system tray
	}

	VirtualFreeEx(hTrayProc, lpData, 0, MEM_RELEASE);
	CloseHandle(hTrayProc);

	return bIconFound;
}
