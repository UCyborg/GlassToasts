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
	RECT rc = {0};
	FindOutPositionOfIconDirectly(hwnd, id, rc);
	POINT pt = {rc.left, rc.top};
	return pt;
}


BOOL CALLBACK TrayIconFinder::FindTrayWnd(HWND hwnd, LPARAM lParam)
{    
	TCHAR szClassName[256];
    GetClassName(hwnd, szClassName, 255);    // Did we find the Main System Tray? If so, then get its size and quit
	if (_tcscmp(szClassName, _T("TrayNotifyWnd")) == 0)    
	{        
		HWND* pWnd = (HWND*)lParam;
		*pWnd = hwnd;
        return FALSE;    
	}    
	
	return TRUE;
}

BOOL CALLBACK TrayIconFinder::FindToolBarInTrayWnd(HWND hwnd, LPARAM lParam)
{    
	TCHAR szClassName[256];
    GetClassName(hwnd, szClassName, 255);    // Did we find the Main System Tray? If so, then get its size and quit
	if (_tcscmp(szClassName, _T("ToolbarWindow32")) == 0)    
	{        
		HWND* pWnd = (HWND*)lParam;
		*pWnd = hwnd;
        return FALSE;    
	}    
	return TRUE;
}



HWND TrayIconFinder::GetTrayNotifyWnd(BOOL a_bSeekForEmbedToolbar)
{
	HWND hWndTrayNotifyWnd = NULL;
	
    HWND hWndShellTrayWnd = FindWindow(_T("Shell_TrayWnd"), NULL);
    if (hWndShellTrayWnd)    
	{        
		EnumChildWindows(hWndShellTrayWnd, TrayIconFinder::FindTrayWnd, (LPARAM)&hWndTrayNotifyWnd);   
		
		if(hWndTrayNotifyWnd && IsWindow(hWndTrayNotifyWnd))
		{
			HWND hWndToolBarWnd = NULL;
			EnumChildWindows(hWndTrayNotifyWnd, TrayIconFinder::FindToolBarInTrayWnd, (LPARAM)&hWndToolBarWnd);   
			if(hWndToolBarWnd)
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
	RECT rect = {0};
	
	HWND hWndTrayWnd = GetTrayNotifyWnd(FALSE);
	if(hWndTrayWnd)
	{
		GetWindowRect(hWndTrayWnd, &rect);
		return rect;
	}

	int nWidth  = GetSystemMetrics(SM_CXSCREEN);
    int nHeight = GetSystemMetrics(SM_CYSCREEN);
	RECT rc = {nWidth-40, nHeight-20, nWidth, nHeight};

	return rc;
}


BOOL TrayIconFinder::FindOutPositionOfIconDirectly(const HWND a_hWndOwner, const int a_iButtonID, RECT& a_rcIcon)
{
	//first of all let's find a Tool bar control embed in Tray window
	HWND hWndTray = GetTrayNotifyWnd(TRUE);
    if (hWndTray == NULL)    
	{
		return FALSE;
	}

	//now we have to get an ID of the parent process for system tray
	DWORD dwTrayProcessID = -1;
	GetWindowThreadProcessId(hWndTray, &dwTrayProcessID);
	if(dwTrayProcessID <= 0)
	{
		return FALSE;
	}

	HANDLE hTrayProc = OpenProcess(PROCESS_ALL_ACCESS, 0, dwTrayProcessID);
	if(hTrayProc == NULL)
	{
		return FALSE;
	}
 
	//now we check how many buttons is there - should be more than 0
	int iButtonsCount = SendMessage(hWndTray, TB_BUTTONCOUNT, 0, 0);

	//We want to get data from another process - it's not possible to just send messages like TB_GETBUTTON with a localy
	//allocated buffer for return data. Pointer to localy allocated data has no usefull meaning in a context of another
	//process (since Win95) - so we need to allocate some memory inside Tray process.
	//We allocate sizeof(TBBUTTON) bytes of memory - because TBBUTTON is the biggest structure we will fetch. But this buffer
	//will be also used to get smaller pieces of data like RECT structures.
	LPVOID lpData = VirtualAllocEx(hTrayProc, NULL, sizeof(TBBUTTON), MEM_COMMIT, PAGE_READWRITE);
	if( lpData == NULL || iButtonsCount < 1 )
	{
		CloseHandle(hTrayProc);
		return FALSE;
	}

	BOOL bIconFound = FALSE;

	for(int iButton=0; iButton<iButtonsCount; iButton++)
	{
		//first let's read TBUTTON information about each button in a task bar of tray

		DWORD dwBytesRead = -1;
		TBBUTTON buttonData;
		SendMessage(hWndTray, TB_GETBUTTON, iButton, (LPARAM)lpData);
		ReadProcessMemory(hTrayProc, lpData, &buttonData, sizeof(TBBUTTON), &dwBytesRead);
		if(dwBytesRead < sizeof(TBBUTTON))
		{
			continue;
		}

		//now let's read extra data associated with each button: there will be a HWND of the window that created an icon and icon ID
		DWORD dwExtraData[2] = { 0,0 };
		ReadProcessMemory(hTrayProc, (LPVOID)buttonData.dwData, dwExtraData, sizeof(dwExtraData), &dwBytesRead);
		if(dwBytesRead < sizeof(dwExtraData))
		{
			continue;
		}

		HWND hWndOfIconOwner = (HWND) dwExtraData[0];
		int  iIconId		 = (int)  dwExtraData[1];
		
		if(hWndOfIconOwner != a_hWndOwner || iIconId != a_iButtonID)
		{
			continue;
		}
		
		//we found our icon - in WinXP it could be hidden - let's check it:
		if( buttonData.fsState & TBSTATE_HIDDEN )
		{
			break;
		}

		//now just ask a tool bar of rectangle of our icon
		RECT rcPosition = {0,0};
		SendMessage(hWndTray, TB_GETITEMRECT, iButton, (LPARAM)lpData);
		ReadProcessMemory(hTrayProc, lpData, &rcPosition, sizeof(RECT), &dwBytesRead);

		if(dwBytesRead < sizeof(RECT))
		{
			continue;
		}

		MapWindowPoints(hWndTray, NULL, (LPPOINT)&rcPosition, 2);
		a_rcIcon = rcPosition;
		
		bIconFound = TRUE;
		break;
	}

	if(bIconFound == FALSE)
	{
		a_rcIcon = GetTrayWndRect(); //we failed to detect position of icon - let's return fail safe cooridinates of system tray
	}

	VirtualFreeEx(hTrayProc, lpData, NULL, MEM_RELEASE);
	CloseHandle(hTrayProc);

	return bIconFound;	
}
