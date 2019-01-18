// GlassToasts.cpp : main source file for GlassToasts.exe
//

#include "StdAfx.h"
#include "MainDlg.h"

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO); // GetNativeSystemInfo
CAppModule _Module;

int checkCmdLineParm(TCHAR *parm)
{
	for (int i = 1; i < __argc; i++)
		if (!_tcsicmp(parm, __targv[i]))
			return i;

	return 0;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int nCmdShow)
{
	SYSTEM_INFO sysInfo;
	SYSTEM_INFO nativeSysInfo;
	PGNSI PGetNativeSystemInfo;

	GetSystemInfo(&sysInfo);
	PGetNativeSystemInfo = (PGNSI)GetProcAddress(GetModuleHandle(_T("kernel32.dll")), "GetNativeSystemInfo");
	if (PGetNativeSystemInfo)
		PGetNativeSystemInfo(&nativeSysInfo);
	else
		nativeSysInfo.wProcessorArchitecture = sysInfo.wProcessorArchitecture;

	if (sysInfo.wProcessorArchitecture != nativeSysInfo.wProcessorArchitecture)
	{
		MessageBox(NULL, _T("Process is running under emulation. You need a version of GlassToasts compiled for your processor's architecture."), NULL, MB_OK | MB_ICONERROR);
		return EXIT_FAILURE;
	}

	CMainDlg::WM_AVE_ACTION =
		RegisterWindowMessage(_T("AvePleaseDoThisForMeOkay"));

	if (checkCmdLineParm(_T("-stop")))
	{
		PostMessage(HWND_BROADCAST, CMainDlg::WM_AVE_ACTION, 1, 0);
		return 0;
	}

	if (checkCmdLineParm(_T("-kill")))
	{
		PostMessage(HWND_BROADCAST, CMainDlg::WM_AVE_ACTION, 2, 0);
		return 0;
	}

	HANDLE mutex = CreateMutex(NULL, TRUE, _T("AveGlassToastsMutexForRunningOnlyOneApp"));
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
	if (FAILED(hRes))
	{
		GdiplusShutdown(token);
		CloseHandle(mutex);
		return int(hRes);
	}

	CMainDlg dlgMain;
	dlgMain.showStartPopup = checkCmdLineParm(_T("-silence")) == 0;

	if (!dlgMain.Create(NULL))
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		GdiplusShutdown(token);
		CloseHandle(mutex);
		return EXIT_FAILURE;
	}

	if (!checkCmdLineParm(_T("-noauto")))
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
