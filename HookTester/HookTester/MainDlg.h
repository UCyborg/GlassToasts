// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"

typedef BOOL (CALLBACK* PStartHook)(HMODULE, HWND);
typedef BOOL (CALLBACK* PStopHook)();
typedef BOOL (CALLBACK* PIsHookRunning)();

void GetFilePath(WCHAR* buf, const WCHAR* name, BOOL ignoreSkin=FALSE);

#define WM_APP_BYE WM_APP + 709
#define WM_TRAYCALLBACK WM_APP + 1

class CMainDlg : public CDialogImpl<CMainDlg>
{
public:
	enum { IDD = IDD_MAINDLG };

	std::queue<NOTIFYICONDATA*> queuedBalloons;
	BOOL balloonIsActive;
	HWND balloonHwnd;

	BOOL showStartPopup;

	static UINT WM_AVE_ACTION;
  

	BEGIN_MSG_MAP(CMainDlg)
		if(uMsg == WM_AVE_ACTION)
			OnAveAction(uMsg, wParam, lParam, bHandled);
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowsPosChanged)
		MESSAGE_HANDLER(WM_TRAYCALLBACK, OnTrayCallback)
		MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
		MESSAGE_HANDLER(WM_APP_BYE, OnBalloonFinished)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)	
		COMMAND_HANDLER(IDC_STARTHOOK, BN_CLICKED, OnBnClickedStarthook)
		COMMAND_HANDLER(IDC_STOPHOOK, BN_CLICKED, OnBnClickedStophook)
		COMMAND_HANDLER(IDC_SHOWBALLOON, BN_CLICKED, OnBnClickedShowballoon)
		COMMAND_HANDLER(IDC_HIDEDLG, BN_CLICKED, OnBnClickedHidedlg)
		COMMAND_HANDLER(IDC_SKINS, CBN_SELCHANGE, OnCbnSelchangeSkins)
	END_MSG_MAP()

	PStartHook     StartHook;
	PStopHook      StopHook;
	PIsHookRunning IsHookRunning;
	HMODULE hMod;

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnWindowsPosChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnTrayCallback(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnAveAction(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnBalloonFinished(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCopyData(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	{
		balloonIsActive = FALSE;
		balloonHwnd = NULL;
		// center the dialog on the screen
		CenterWindow();

		// set icons
		
		HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICON2), 
			IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
		SetIcon(hIcon, TRUE);
		HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICON2), 
			IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		SetIcon(hIconSmall, FALSE);
		
		StartHook = NULL;
		StopHook = NULL;
		IsHookRunning = NULL;


		WCHAR hookPath[MAX_PATH] = {0};
		GetFilePath(hookPath, L"trayhook.dll", TRUE);
		hMod = LoadLibrary(hookPath);
		if(hMod != NULL)
		{
			StartHook     = (PStartHook)GetProcAddress(hMod, "StartHook");
			StopHook      = (PStopHook)GetProcAddress(hMod, "StopHook");
			IsHookRunning = (PIsHookRunning)GetProcAddress(hMod, "IsHookRunning");
		}

		BOOL isRunning = IsHookRunning && IsHookRunning();
		::EnableWindow(GetDlgItem(IDC_STARTHOOK),!isRunning);
		::EnableWindow(GetDlgItem(IDC_STOPHOOK),isRunning);

		return TRUE;
	}

	void ShowBalloonTip(NOTIFYICONDATA* data);
	void LoadSkins();

public:
	LRESULT OnBnClickedStarthook(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
public:
	LRESULT OnBnClickedStophook(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
public:
	LRESULT OnBnClickedShowballoon(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
public:
	LRESULT OnBnClickedHidedlg(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
public:
	LRESULT OnCbnSelchangeSkins(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};
