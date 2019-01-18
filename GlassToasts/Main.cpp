// HookTester.cpp : main source file for HookTester.exe
//

#include "StdAfx.h"
#include "MainDlg.h"

CAppModule _Module;

int checkCmdLineParm(TCHAR *parm)
{
	for (int i = 1; i < __argc; i++)
		if (_tcsicmp(parm, __targv[i]) == 0)
			return i;
	return 0;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	CMainDlg::WM_AVE_ACTION =
		::RegisterWindowMessage(_T("AvePleaseDoThisForMeOkay"));

	if (checkCmdLineParm(_T("-stop")) != 0)
	{
		PostMessage(HWND_BROADCAST, CMainDlg::WM_AVE_ACTION, 1, 0);
		return 0;
	}

	if (checkCmdLineParm(_T("-kill")) != 0)
	{
		PostMessage(HWND_BROADCAST, CMainDlg::WM_AVE_ACTION, 2, 0);
		return 0;
	}

	if (checkCmdLineParm(_T("-show")) != 0)
	{
		PostMessage(HWND_BROADCAST, CMainDlg::WM_AVE_ACTION, 3, 0);
		return 0;
	}

	HANDLE mutex = CreateMutex(NULL, TRUE, _T("AveGlassToastMutexForRunningOnlyOneApp"));
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		PostMessage(HWND_BROADCAST, CMainDlg::WM_AVE_ACTION, 3, 0);
		CloseHandle(mutex);
		return 0;
	}

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR token;
	int nRet = GdiplusStartup(&token, &gdiplusStartupInput, NULL);
	if (nRet != Ok)
	{
		ATLTRACE(_T("GdiplusStartup failed!\n"));
		CloseHandle(mutex);
		return nRet;
	}

	HRESULT hRes = _Module.Init(NULL, hInstance);
	if (hRes != S_OK)
	{
		GdiplusShutdown(token);
		CloseHandle(mutex);
		return int(hRes);
	}

	CMainDlg dlgMain;
	dlgMain.showStartPopup = checkCmdLineParm(_T("-silence")) == 0;

	if (dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		GdiplusShutdown(token);
		CloseHandle(mutex);
		return EXIT_FAILURE;
	}

	if (checkCmdLineParm(_T("-noauto")) == 0)
	{
		BOOL res = FALSE;
		dlgMain.OnBnClickedStarthook(0, 0, 0, res);
	}
	else
		dlgMain.ShowWindow(nCmdShow);

	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	_Module.Term();
	GdiplusShutdown(token);
	CloseHandle(mutex);

	return nRet;
}
