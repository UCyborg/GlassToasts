// MainDlg.h : interface of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#include "resource.h"
#include "..\TrayHook\TrayHook.h"

void GetFilePath(TCHAR* buf, const TCHAR* name, BOOL ignoreSkin = FALSE);
ATOM MyRegisterClass(HINSTANCE);

class CMainDlg : public CDialogImpl<CMainDlg>
{
public:
	enum { IDD = IDD_MAINDLG };

	std::queue<NOTIFYICONDATA32*> queuedBalloons;
	ATOM balloonAtom;
	HWND balloonHwnd;

	BOOL showStartPopup;

	static UINT WM_AVE_ACTION;

	BEGIN_MSG_MAP(CMainDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		MESSAGE_HANDLER(WM_TRAYCALLBACK, OnTrayCallback)
		MESSAGE_HANDLER(WM_COPYDATA, OnCopyData)
		MESSAGE_HANDLER(WM_AVE_ACTION, OnAveAction)
		MESSAGE_HANDLER(WM_APP_BYE, OnBalloonFinished)
		MESSAGE_HANDLER(WM_TRAYCRASHED, OnTrayCrashed)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_ENDSESSION, OnEndSession)
		COMMAND_HANDLER(IDC_STARTHOOK, BN_CLICKED, OnBnClickedStarthook)
		COMMAND_HANDLER(IDC_STOPHOOK, BN_CLICKED, OnBnClickedStophook)
		COMMAND_HANDLER(IDC_SHOWBALLOON, BN_CLICKED, OnBnClickedShowballoon)
		COMMAND_HANDLER(IDC_REFRESH, BN_CLICKED, OnBnClickedRefresh)
		COMMAND_HANDLER(IDC_HIDEDLG, BN_CLICKED, OnBnClickedHidedlg)
		COMMAND_HANDLER(IDC_SKINS, CBN_SELCHANGE, OnCbnSelchangeSkins)
	END_MSG_MAP()

	BOOL			isHookRunning;
	BOOL			isTimerActive;

	void ShowBalloonTip(NOTIFYICONDATA32*);
	void LoadSkins();

	// Handler prototypes (uncomment arguments if needed):
	//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
	//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
	//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnWindowsPosChanged(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnTrayCallback(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnCopyData(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnAveAction(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnBalloonFinished(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnTrayCrashed(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnTimer(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&);
	LRESULT OnEndSession(UINT, WPARAM, LPARAM, BOOL&);

	LRESULT OnBnClickedStarthook(WORD, WORD, HWND, BOOL&);
	LRESULT OnBnClickedStophook(WORD, WORD, HWND, BOOL&);
	LRESULT OnBnClickedShowballoon(WORD, WORD, HWND, BOOL&);
	LRESULT OnBnClickedRefresh(WORD, WORD, HWND, BOOL&);
	LRESULT OnBnClickedHidedlg(WORD, WORD, HWND, BOOL&);
	LRESULT OnCbnSelchangeSkins(WORD, WORD, HWND, BOOL&);
};
