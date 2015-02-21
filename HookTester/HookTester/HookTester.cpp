// HookTester.cpp : main source file for HookTester.exe
//

#include "stdafx.h"

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>

#include "resource.h"

#include "MainDlg.h"

CAppModule _Module;

int Run(LPTSTR lpstrCmdLine = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMainDlg::WM_AVE_ACTION = 
        ::RegisterWindowMessage(_T("AvePleaseDoThisForMeOkay"));

	HANDLE mutex = CreateMutex(NULL, TRUE, L"AveGlassToastMutexForRunningOnlyOneApp");
	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		SendMessage(HWND_BROADCAST, CMainDlg::WM_AVE_ACTION, 3, 0);
		CloseHandle(mutex);
		return 0;
	}

	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);
	


	if(wcsstr(lpstrCmdLine, L"-stop") != 0)
	{
		SendMessage(HWND_BROADCAST, CMainDlg::WM_AVE_ACTION, 1, 0);
		return 0;
	}

	if(wcsstr(lpstrCmdLine, L"-kill") != 0)
	{
		SendMessage(HWND_BROADCAST, CMainDlg::WM_AVE_ACTION, 2, 0);
		return 0;
	}

	if(wcsstr(lpstrCmdLine, L"-show") != 0)
	{
		SendMessage(HWND_BROADCAST, CMainDlg::WM_AVE_ACTION, 3, 0);
		return 0;
	}

	CMainDlg dlgMain;
	dlgMain.showStartPopup = wcsstr(lpstrCmdLine, L"-silence") == 0;

	if(dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		return 0;
	}
	int show = nCmdShow;

	if(wcsstr(lpstrCmdLine, L"-noauto") == 0)
		show = 0;

	dlgMain.ShowWindow(show);
	if(0 == show)
	{
		BOOL res = FALSE;
		dlgMain.OnBnClickedStarthook(0,0,0,res);
	}

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();

	CloseHandle(mutex);

	if(dlgMain.hMod && dlgMain.StopHook)
		dlgMain.StopHook();

	if(::IsWindow(dlgMain.m_hWnd))
		dlgMain.DestroyWindow();

	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR pGdiToken;
	GdiplusStartup(&pGdiToken,&gdiplusStartupInput,NULL); 

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
