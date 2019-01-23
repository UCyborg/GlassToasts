#include "StdAfx.h"
#include "MainDlg.h"
#include "TrayIconFinder.h"

UINT CMainDlg::WM_AVE_ACTION;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

BOOL dwmLib = TRUE;

void getBalloonIcon(NOTIFYICONDATA32* data);

BOOL HasDWM()
{
	BOOL bEnabled = FALSE;
	if (dwmLib)
	{
		__try
		{
			DwmIsCompositionEnabled(&bEnabled);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			dwmLib = FALSE;
		}
	}
	return bEnabled;
}

struct BalloonOptions
{
	int prevCloseState;
	int prevOptionsState;
	int alpha;
	BOOL fadingIn;
	BOOL mouseIsOn;
	BOOL ncmouseIsOn;
	TRACKMOUSEEVENT tme;
	BOOL startedPressOnClose;
	BOOL startedPressOnOptions;
	BOOL isPressed;
};

void GetSkinName(TCHAR* skinName, DWORD nSize)
{
	TCHAR defaultPath[MAX_PATH];
	GetFilePath(defaultPath, _T("defaults.ini"), TRUE);

	TCHAR defaultSkinName[MAX_PATH];
	GetPrivateProfileString(_T("settings"), _T("skin"), _T("Beta1"), defaultSkinName, MAX_PATH, defaultPath);

	TCHAR appDataPath[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath);
	PathAppend(appDataPath, _T("avetoasts.ini"));
	GetPrivateProfileString(_T("settings"), _T("skin"), defaultSkinName, skinName, MAX_PATH, appDataPath);
}

void WriteSkinName(const TCHAR* skinName)
{
	TCHAR appDataPath[MAX_PATH];
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath);
	PathAppend(appDataPath, _T("avetoasts.ini"));
	WritePrivateProfileString(_T("settings"), _T("skin"), skinName, appDataPath);
}

void GetFilePath(TCHAR* buf, const TCHAR* name, BOOL ignoreSkin)
{
	TCHAR skinName[MAX_PATH];
	if (!ignoreSkin)
	{
		GetSkinName(skinName, MAX_PATH);
	}

	GetModuleFileName(NULL, buf, MAX_PATH);
	PathRemoveFileSpec(buf);
	if (!ignoreSkin)
	{
		PathAppend(buf, _T("skins"));
		PathAppend(buf, skinName);
	}
	PathAppend(buf, name);
}

INT GetOffsetValue(const TCHAR* name, const TCHAR* key, INT def)
{
	TCHAR path[MAX_PATH];
	GetFilePath(path, _T("look.ini"));

	return GetPrivateProfileInt(name, key, def, path);
}

void GetStringValue(const TCHAR* name, const TCHAR* key, const TCHAR* def, TCHAR* val, DWORD nSize)
{
	TCHAR path[MAX_PATH];
	GetFilePath(path, _T("look.ini"));

	GetPrivateProfileString(name, key, def, val, nSize, path);
}

COLORREF GetColor(const TCHAR* name, const TCHAR* key, COLORREF def)
{
	TCHAR color[MAX_PATH];
	GetStringValue(name, key, _T(""), color, MAX_PATH);
	if (_tcslen(color) < 7)
		return def;

	const TCHAR* ptr = color;
	ptr++; // skip #

	TCHAR val[3] = { 0 };
	val[0] = *ptr++;
	val[1] = *ptr++;

	long r = _tcstol(val, NULL, 16);

	val[0] = *ptr++;
	val[1] = *ptr++;

	long g = _tcstol(val, NULL, 16);

	val[0] = *ptr++;
	val[1] = *ptr++;

	long b = _tcstol(val, NULL, 16);

	return RGB(r, g, b);
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = 0;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;//MAKEINTRESOURCE(IDC_GLASSTOAST);
	wcex.lpszClassName = _T("AveGlassToolTips");
	wcex.hIconSm = NULL;//LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

void CMainDlg::LoadSkins()
{
	TCHAR path[MAX_PATH];
	TCHAR searchPath[MAX_PATH];
	GetFilePath(path, _T("Skins\\"), TRUE);
	GetFilePath(searchPath, _T("Skins\\*.*"), TRUE);

	WIN32_FIND_DATA data;
	HANDLE h = FindFirstFile(searchPath, &data);
	if (h == INVALID_HANDLE_VALUE)
		return;

	CComboBox box = GetDlgItem(IDC_SKINS);
	box.ResetContent();

	do
	{
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
			_tcscmp(data.cFileName, _T(".")) && _tcscmp(data.cFileName, _T("..")))
		{
			box.AddString(data.cFileName);
		}

	} while (FindNextFile(h, &data));

	FindClose(h);

	TCHAR skinName[MAX_PATH];
	GetSkinName(skinName, MAX_PATH);
	box.SelectString(0, skinName);
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// set icons
	HICON hIcon = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = (HICON)::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	SetIcon(hIconSmall, FALSE);

	LoadSkins();

	balloonAtom = MyRegisterClass(_Module.GetModuleInstance());
	balloonHwnd = NULL;

	return TRUE;
}

LRESULT CMainDlg::OnTrayCallback(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	if (lParam == NIN_BALLOONUSERCLICK)
	{
		ShowWindow(SW_SHOW);
		SetForegroundWindow(m_hWnd);
	}

	return 0;
}

LRESULT CMainDlg::OnCopyData(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	COPYDATASTRUCT* cds = reinterpret_cast<COPYDATASTRUCT*>(lParam);
	if (!cds)
		return 0;

	if (cds->dwData != 1 || cds->cbData < NOTIFYICONDATA32_V2_SIZE + 8)
		return 0;

	TRAYNOTIFYDATA* data = reinterpret_cast<TRAYNOTIFYDATA*>(cds->lpData);
	if (data && data->dwSignature == 0x34753423)
	{
		if (data->dwMessage <= NIM_MODIFY)
		{
			if (data->nid.uFlags & NIF_INFO)
			{
				if (!data->nid.szInfo || !_tcslen(data->nid.szInfo))
				{
					NOTIFYICONDATA32*curData = (NOTIFYICONDATA32*)GetProp(balloonHwnd, _T("data"));
					if (curData && curData->dwWnd == data->nid.dwWnd && curData->uID == data->nid.uID)
					{
						::PostMessage(balloonHwnd, WM_CLOSE, 0, 0);
					}
				}
				else
				{
					//::MessageBox(0, _T("we got a balloon.. part 2"), 0, 0);
					NOTIFYICONDATA32 *buf = (NOTIFYICONDATA32*)LocalAlloc(LPTR, sizeof(NOTIFYICONDATA32));
					if (buf)
					{
						memcpy(buf, &data->nid, data->nid.cbSize <= sizeof(NOTIFYICONDATA32) ? data->nid.cbSize : sizeof(NOTIFYICONDATA32));
						getBalloonIcon(buf);
						queuedBalloons.push(buf);
						if (!balloonHwnd)
							PostMessage(WM_APP_BYE);
					}
				}
				DWORD test = data->nid.uFlags &= ~(NIF_INFO | NIF_REALTIME);

				if (test)
					return 2;
				return 1;
			}
		}

		//::MessageBox(0, _T("balloon entered queue"), 0,0);
	}

	return 0;
}

LRESULT CMainDlg::OnAveAction(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
	if (wParam == 1)
		OnBnClickedStophook(0, 0, NULL, bHandled);
	else if (wParam == 2)
		DestroyWindow();
	else if (wParam == 3)
	{
		ShowWindow(SW_SHOW);
		SetForegroundWindow(m_hWnd);
	}
	return 0;
}

LRESULT CMainDlg::OnBalloonFinished(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	balloonHwnd = NULL;
	if (!queuedBalloons.empty())
	{
		NOTIFYICONDATA32* buf = queuedBalloons.front();
		queuedBalloons.pop();
		ShowBalloonTip(buf);
		if (buf->dwBalloonIcon && !(buf->dwInfoFlags & (NIIF_INFO | NIIF_WARNING | NIIF_ERROR)))
			DestroyIcon((HICON)buf->dwBalloonIcon);
		LocalFree(buf);
	}
	return 0;
}

LRESULT CMainDlg::OnTrayCrashed(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	isTimerActive = (BOOL)SetTimer(1, 2500, NULL);
	isHookRunning = FALSE;
	::EnableWindow(GetDlgItem(IDC_STARTHOOK), !isHookRunning);
	::EnableWindow(GetDlgItem(IDC_STOPHOOK), isHookRunning);
	::EnableWindow(GetDlgItem(IDC_SHOWBALLOON), isHookRunning);

	return 0;
}

LRESULT CMainDlg::OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if (!isHookRunning)
	{
		isHookRunning = StartHook(m_hWnd);
		if (isHookRunning)
		{
			KillTimer(1);
			isTimerActive = FALSE;
			::EnableWindow(GetDlgItem(IDC_STARTHOOK), !isHookRunning);
			::EnableWindow(GetDlgItem(IDC_STOPHOOK), isHookRunning);
			::EnableWindow(GetDlgItem(IDC_SHOWBALLOON), isHookRunning);
		}
	}

	return 0;
}

LRESULT CMainDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	ShowWindow(SW_HIDE);

	return 0;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	StopHook();

	if (balloonHwnd)
		::DestroyWindow(balloonHwnd);
	if (balloonAtom)
		UnregisterClass((LPCTSTR)balloonAtom, _Module.GetModuleInstance());

	PostQuitMessage(EXIT_SUCCESS);

	return 0;
}

LRESULT CMainDlg::OnEndSession(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if (wParam)
		DestroyWindow();

	return 0;
}

void GlassToastThread(void* lpParameter)
{
	NOTIFYICONDATA data = { 0 };
	data.cbSize = NOTIFYICONDATA_V2_SIZE;
	data.uFlags = NIF_INFO;
	data.hWnd = (HWND)lpParameter;
	data.uCallbackMessage = WM_TRAYCALLBACK;
	_tcscpy_s(data.szInfoTitle, 64, _T("Ave's Glass Toasts"));
	_tcscpy_s(data.szInfo, 256, _T("Ave's Glass Toasts has been started.\nClick here for additional options for Ave's Glass Toasts."));
	Shell_NotifyIcon(NIM_MODIFY, &data);
}

LRESULT CMainDlg::OnBnClickedStarthook(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if (isTimerActive)
	{
		KillTimer(1);
		isTimerActive = FALSE;
	}

	isHookRunning = StartHook(m_hWnd);
	if (isHookRunning)
	{
		::EnableWindow(GetDlgItem(IDC_STARTHOOK), !isHookRunning);
		::EnableWindow(GetDlgItem(IDC_STOPHOOK), isHookRunning);
		::EnableWindow(GetDlgItem(IDC_SHOWBALLOON), isHookRunning);
	}

	if (isHookRunning && showStartPopup)
		_beginthread(GlassToastThread, 0, (void*)m_hWnd);

	return 0;
}

LRESULT CMainDlg::OnBnClickedStophook(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	StopHook();
	isHookRunning = FALSE;
	::EnableWindow(GetDlgItem(IDC_STARTHOOK), !isHookRunning);
	::EnableWindow(GetDlgItem(IDC_STOPHOOK), isHookRunning);
	::EnableWindow(GetDlgItem(IDC_SHOWBALLOON), isHookRunning);

	return 0;
}

void TestThread(void* lpParameter)
{
	TCHAR name[MAX_PATH];
	GetStringValue(_T("info"), _T("author"), _T("unknown"), name, MAX_PATH);
	NOTIFYICONDATA data = { 0 };
	data.cbSize = NOTIFYICONDATA_V2_SIZE;
	data.uFlags = NIF_INFO;
	data.dwInfoFlags = NIIF_INFO;
	data.hWnd = (HWND)lpParameter;
	data.uCallbackMessage = WM_TRAYCALLBACK;
	_tcscpy_s(data.szInfoTitle, 64, _T("Ave's Glass Toasts"));
	_tcscpy_s(data.szInfo, 256, _T("Thank you for choosing Ave's Glass Toasts.\nCode by Andreas Verhoeven,\nGraphics by "));
	_tcscat_s(data.szInfo, 256, name);
	_tcscat_s(data.szInfo, 256, _T("."));
	Shell_NotifyIcon(NIM_MODIFY, &data);
}

LRESULT CMainDlg::OnBnClickedShowballoon(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	_beginthread(TestThread, 0, (void*)m_hWnd);

	return 0;
}

LRESULT CMainDlg::OnCbnSelchangeSkins(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR newSkinName[MAX_PATH];
	CComboBox box = GetDlgItem(IDC_SKINS);
	int sel = box.GetCurSel();
	box.GetLBText(sel, newSkinName);

	WriteSkinName(newSkinName);

	if (balloonHwnd)
		::DestroyWindow(balloonHwnd);

	_beginthread(TestThread, 0, (void*)m_hWnd);

	return 0;
}

LRESULT CMainDlg::OnBnClickedRefresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	LoadSkins();

	return 0;
}

LRESULT CMainDlg::OnBnClickedHidedlg(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	DestroyWindow();

	return 0;
}

BOOL SetLayeredAlpha(HWND hwnd, BYTE alpha)
{
	if (!hwnd)
		return FALSE;

	float progress = float(alpha) / 255.0f;
	float cur = sinf(progress * M_PI / 2.0f);
	BYTE correctedAlpha = (BYTE)(cur * 255.0f);

	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = correctedAlpha;
	return UpdateLayeredWindow(hwnd, 0, 0, 0, 0, 0, 0, &bf, ULW_ALPHA);
}

BOOL SetLayeredWindow(HWND hwnd, Bitmap* bmp, BYTE alpha = 255)
{
	if (!hwnd || !bmp)
		return FALSE;

	HBITMAP hbmp = NULL;
	bmp->GetHBITMAP(NULL, &hbmp);
	if (!hbmp)
		return FALSE;

	SIZE s = { bmp->GetWidth(), bmp->GetHeight() };
	POINT pt = { 0, 0 };

	HDC dc = CreateCompatibleDC(0);
	HBITMAP oldbmp = (HBITMAP)SelectObject(dc, hbmp);

	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = alpha;
	BOOL res = UpdateLayeredWindow(hwnd, 0, 0, &s, dc, &pt, 0, &bf, ULW_ALPHA);

	SelectObject(dc, oldbmp);
	DeleteDC(dc);
	DeleteObject(hbmp);

	return res;
}

BOOL SetLayeredWindowHBitmap(HWND hwnd, HBITMAP hbmp, SIZE s, BYTE alpha = 255)
{
	if (!hwnd || !hbmp)
		return FALSE;

	POINT pt = { 0, 0 };

	HDC dc = CreateCompatibleDC(0);
	HBITMAP oldbmp = (HBITMAP)SelectObject(dc, hbmp);

	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = alpha;
	BOOL res = UpdateLayeredWindow(hwnd, 0, 0, &s, dc, &pt, 0, &bf, ULW_ALPHA);

	SelectObject(dc, oldbmp);
	DeleteDC(dc);

	return res;
}

HRESULT EnableBlurBehindWindow(HWND window,
	bool enable = true,
	HRGN region = NULL,
	bool transitionOnMaximized = false)
{
	if (!HasDWM())
		return S_FALSE;

	DWM_BLURBEHIND blurBehind;

	blurBehind.dwFlags = DWM_BB_ENABLE | DWM_BB_TRANSITIONONMAXIMIZED;
	blurBehind.fEnable = enable;
	blurBehind.fTransitionOnMaximized = transitionOnMaximized;

	if (enable && region)
	{
		blurBehind.dwFlags |= DWM_BB_BLURREGION;
		blurBehind.hRgnBlur = region;
	}
	else
	{
		blurBehind.hRgnBlur = NULL;
	}

	return DwmEnableBlurBehindWindow(window,
		&blurBehind);
}

void balloonEvent(HWND hwnd, LPARAM msg)
{
	DWORD uid = (DWORD)GetProp(hwnd, _T("uid"));
	UINT message = (UINT)GetProp(hwnd, _T("msg"));
	HWND win = (HWND)GetProp(hwnd, _T("hwnd"));

	PostMessage(win, message, (WPARAM)uid, msg);
}

const int MOUSE_OTHER = 0;
const int MOUSE_ON = 1;
const int MOUSE_PRESSED = 2;
const int OPTIONS_ON = 4;
const int OPTIONS_PRESSED = 8;
const int OPTIONS_HIDDEN = 16;

HBITMAP paintBalloon(NOTIFYICONDATA32* data, SIZE* size, Rect* closeRc, Rect* optionsRc, int states, HWND hwnd)
{
	TCHAR bgPath[MAX_PATH];
	TCHAR insetPath[MAX_PATH];
	TCHAR closePath[MAX_PATH];
	GetFilePath(bgPath, _T("bgshadow.png"));
	GetFilePath(insetPath, _T("inset.png"));
	if (states & 1)
		GetFilePath(closePath, _T("closehover.png"));
	else if (states & 2)
		GetFilePath(closePath, _T("closepress.png"));
	else
		GetFilePath(closePath, _T("close.png"));


	Bitmap bmp(bgPath);
	Bitmap bmp2(insetPath);
	Bitmap bmp3(closePath);

	SIZE s = { bmp.GetWidth(), bmp.GetHeight() };

	INT minWidth = GetOffsetValue(_T("global"), _T("minwidth"), 205);
	INT minHeight = GetOffsetValue(_T("global"), _T("minheight"), 100);
	INT maxWidth = GetOffsetValue(_T("global"), _T("maxwidth"), 300);
	INT maxHeight = GetOffsetValue(_T("global"), _T("maxheight"), 100);

	INT extraWidth = GetOffsetValue(_T("global"), _T("extrawidth"), 70);
	INT extraHeight = GetOffsetValue(_T("global"), _T("extraheight"), 20);


	HDC dc = CreateCompatibleDC(0);

	TCHAR textFont[LF_FACESIZE];
	GetStringValue(_T("text"), _T("facename"), _T("Segoe UI"), textFont, LF_FACESIZE);

	LOGFONT lf = { -12, 0, 0, 0, 400, 0, 0, 0, 0, 3, 2, 1, 34, 0 };
	_tcscpy_s(lf.lfFaceName, LF_FACESIZE, textFont);
	HFONT hf = CreateFontIndirect(&lf);
	HFONT oldFont = (HFONT)SelectObject(dc, hf);

	RECT rcMeasure = { 0, 0, maxWidth - extraWidth, maxHeight - extraHeight };
	if (HasDWM())
	{
		DrawText(dc, data->szInfo, -1, &rcMeasure, DT_WORDBREAK | DT_CALCRECT);
	}
	else
	{
		Graphics g(dc);
		Font font(textFont, 10);
		RectF layoutRect(0, 0, (REAL)(maxWidth - extraWidth), (REAL)(maxHeight - extraHeight));
		RectF boundingBox(0, 0, 0, 0);
		g.MeasureString(data->szInfo, -1, &font, layoutRect, &boundingBox);
		rcMeasure.right = (INT)boundingBox.Width;
		rcMeasure.bottom = (INT)boundingBox.Height;
	}

	INT width = rcMeasure.right - rcMeasure.left;
	INT height = rcMeasure.bottom - rcMeasure.top;

	s.cx = max(minWidth - extraWidth, width) + extraWidth;
	s.cy = max(minHeight - extraHeight, height) + extraHeight;

	int xcor = GetOffsetValue(_T("global"), _T("xcorrection"), 20);
	int ycor = GetOffsetValue(_T("global"), _T("ycorrection"), 50);

	int fixX = 0;
	if (xcor != 0)
		fixX = (s.cx - minWidth) / xcor;
	int fixY = 0;
	if (ycor != 0)
		fixY = (s.cy - minHeight) / ycor;


	int textl = GetOffsetValue(_T("text"), _T("left"), 54);
	int textt = GetOffsetValue(_T("text"), _T("top"), 39);
	RECT rcText = { textl - fixX, textt, textl + width - fixX, height + textt - fixY };
	//rcText = r;

	if (size)
		*size = s;


	BITMAPINFO dib = { 0 };
	dib.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	dib.bmiHeader.biWidth = s.cx;
	dib.bmiHeader.biHeight = -s.cy;
	dib.bmiHeader.biPlanes = 1;
	dib.bmiHeader.biBitCount = 32;
	dib.bmiHeader.biCompression = BI_RGB;

	HBITMAP hbmp = CreateDIBSection(dc, &dib, DIB_RGB_COLORS, NULL, NULL, 0);


	HBITMAP old = (HBITMAP)SelectObject(dc, hbmp);


	Graphics g(dc);
	g.SetSmoothingMode(SmoothingModeAntiAlias);
	g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
	g.DrawImage(&bmp, Rect(0, 0, s.cx, s.cy),
		0, 0, bmp.GetWidth(), bmp.GetHeight(),
		UnitPixel, 0, 0, 0);

	int intsetl = GetOffsetValue(_T("inset"), _T("left"), 46);
	int intsett = GetOffsetValue(_T("inset"), _T("top"), 23);
	int intsetr = GetOffsetValue(_T("inset"), _T("right"), 57);
	int intsetb = GetOffsetValue(_T("inset"), _T("bottom"), 29);

	g.DrawImage(&bmp2, Rect(intsetl, intsett, s.cx - intsetr - fixX, s.cy - intsetb - fixY),
		0, 0, bmp2.GetWidth(), bmp2.GetHeight(), UnitPixel, 0, 0, 0);

	int closelo = GetOffsetValue(_T("close"), _T("leftoffset"), 15);
	int closet = GetOffsetValue(_T("close"), _T("top"), 7);

	Rect rcClose(s.cx - bmp3.GetWidth() - closelo - fixX, closet - fixY, bmp3.GetWidth(), bmp3.GetHeight());
	g.DrawImage(&bmp3, rcClose,
		0, 0, bmp3.GetWidth(), bmp3.GetHeight(), UnitPixel, 0, 0, 0);

	if (closeRc)
		*closeRc = rcClose;

	int headerl = GetOffsetValue(_T("header"), _T("left"), 54);
	int headert = GetOffsetValue(_T("header"), _T("top"), 24);
	int headerr = GetOffsetValue(_T("header"), _T("right"), 24);
	int headerb = GetOffsetValue(_T("header"), _T("bottom"), 45);

	RECT rcHeaderText = { headerl - fixX, headert - fixY, s.cx - headerr - fixX, headerb - fixY };

	COLORREF textColor = GetColor(_T("text"), _T("color"), RGB(99, 99, 99));
	COLORREF headerColor = GetColor(_T("header"), _T("color"), RGB(0, 100, 120));

	TCHAR headerFont[LF_FACESIZE];
	GetStringValue(_T("header"), _T("facename"), _T("Segoe UI"), headerFont, LF_FACESIZE);
	if (HasDWM())
	{
		HTHEME hTheme = OpenThemeData(hwnd, L"globals");
		DTTOPTS opts = { 0 };
		opts.dwSize = sizeof(opts);
		opts.fApplyOverlay = TRUE;
		opts.crText = textColor;
		opts.dwFlags = DTT_COMPOSITED | DTT_TEXTCOLOR;
		DrawThemeTextEx(hTheme, dc, 0, 0, data->szInfo, -1, DT_WORDBREAK, &rcText, &opts);

		LOGFONT lf2 = { -16, 0, 0, 0, 400, 0, 0, 0, 0, 3, 2, 1, 34, 0 };
		_tcscpy_s(lf2.lfFaceName, LF_FACESIZE, headerFont);
		HFONT hf2 = CreateFontIndirect(&lf2);
		SelectObject(dc, hf2);


		//RECT rcText = {44, 39, s.cx - 4, s.cy  - 4};
		opts.dwFlags = DTT_COMPOSITED | DTT_TEXTCOLOR | DTT_SHADOWTYPE | DTT_GLOWSIZE | DTT_SHADOWOFFSET | DTT_SHADOWCOLOR;
		opts.crText = headerColor;
		opts.fApplyOverlay = FALSE;
		opts.iTextShadowType = 1;
		opts.iGlowSize = 10;
		opts.ptShadowOffset.x = 1;
		opts.ptShadowOffset.y = 1;
		DrawThemeTextEx(hTheme, dc, 0, 0, data->szInfoTitle, -1, DT_SINGLELINE | DT_WORD_ELLIPSIS, &rcHeaderText, &opts);
		CloseThemeData(hTheme);
		DeleteObject(hf2);


		SelectObject(dc, oldFont);
		DeleteObject(hf);
	}
	else
	{
		g.SetTextRenderingHint(TextRenderingHintSystemDefault);
		g.SetSmoothingMode(SmoothingModeAntiAlias);
		Color b;
		b.SetFromCOLORREF(textColor | 0xFF000000);
		Color b2;
		b2.SetFromCOLORREF(headerColor | 0xFF000000);
		SolidBrush black(b);
		SolidBrush blue(b2);
		//FontFamily fontFamily(L"Segoe UI");
		Font font(textFont, 9);
		Font font2(headerFont, 10);
		StringFormat format;
		RectF rcHeader((REAL)rcHeaderText.left, (REAL)rcHeaderText.top, (REAL)(rcHeaderText.right - rcHeaderText.left), (REAL)(rcHeaderText.bottom - rcHeaderText.top));
		g.DrawString(data->szInfoTitle, -1, &font2, rcHeader, &format, &blue);

		RectF textOutput((REAL)rcText.left, (REAL)rcText.top, (REAL)(rcText.right - rcText.left), (REAL)(rcText.bottom - rcText.top));
		g.DrawString(data->szInfo, -1, &font, textOutput, &format, &black);
	}

	TCHAR optionsPath[MAX_PATH];
	if (states & OPTIONS_ON)
		GetFilePath(optionsPath, _T("optionshover.png"));
	else if (states & OPTIONS_PRESSED)
		GetFilePath(optionsPath, _T("optionspress.png"));
	else
		GetFilePath(optionsPath, _T("options.png"));
	Bitmap bmpOptions(optionsPath);

	int optionslo = GetOffsetValue(_T("options"), _T("leftoffset"), 17);
	int optionst = GetOffsetValue(_T("options"), _T("top"), 4);

	Rect rcOptions(s.cx - bmp3.GetWidth() - optionslo - bmpOptions.GetWidth() - fixX, optionst - fixY, bmpOptions.GetWidth(), bmpOptions.GetHeight());
	if (optionsRc)
		*optionsRc = rcOptions;

	BalloonOptions* opts = (BalloonOptions*)GetProp(hwnd, _T("options"));
	if (opts && (opts->mouseIsOn || opts->ncmouseIsOn) || GetProp(hwnd, _T("menuactive")))
		g.DrawImage(&bmpOptions, rcOptions, 0, 0, bmpOptions.GetWidth(), bmpOptions.GetHeight(), UnitPixel, 0, 0, 0);

	int iconl = GetOffsetValue(_T("icon"), _T("left"), 14);
	int icont = GetOffsetValue(_T("icon"), _T("top"), -1);
	if (icont == -1)
		icont = s.cy / 2 - 16;

	if (data->dwBalloonIcon)
	{
		DrawIcon(dc, iconl, icont, (HICON)data->dwBalloonIcon);

		if (GetOffsetValue(_T("icon"), _T("usereflection"), 1)) // useReflection
		{
			int reflectionpixeltreshold = 32 - GetOffsetValue(_T("icon"), _T("reflectionpixeltreshold"), 24);
			int reflectionalpha = GetOffsetValue(_T("icon"), _T("reflectionalpha"), 150);
			HDC iconDC = CreateCompatibleDC(0);
			BITMAPINFO dib = { 0 };
			dib.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			dib.bmiHeader.biWidth = 32;
			dib.bmiHeader.biHeight = 32;
			dib.bmiHeader.biPlanes = 1;
			dib.bmiHeader.biBitCount = 32;
			dib.bmiHeader.biCompression = BI_RGB;

			HBITMAP iconBmp = CreateDIBSection(dc, &dib, DIB_RGB_COLORS, NULL, NULL, 0);
			HBITMAP old = (HBITMAP)SelectObject(iconDC, iconBmp);
			DrawIcon(iconDC, 0, 0, (HICON)data->dwBalloonIcon);

			BYTE data[32 * 32 * 4];
			LPBYTE ptr = data;
			BYTE data2[32 * 32 * 4];
			LPBYTE ptr2 = data2;
			LPBYTE optr = (&ptr2[0]) + 32 * 32 * 4;
			LPBYTE endptr = (&ptr2[0]) + 32 * 32 * 4;
			GetDIBits(iconDC, iconBmp, 0, 32, (LPVOID)ptr, &dib, DIB_RGB_COLORS);
			int alphaCalc = 0;
			for (int y = 0; y < 32; ++y)
			{
				optr = endptr - 32 * 4 * (y + 1);
				if (y > reflectionpixeltreshold)
				{
					alphaCalc = reflectionalpha / (32 - reflectionpixeltreshold) * (y - reflectionpixeltreshold);
					if (alphaCalc > 255)
						alphaCalc = 255;
				}
				else
				{
					alphaCalc = 0;
				}
				for (int x = 0; x < 32; ++x)
				{
					int a = *ptr++;
					int r = *ptr++;
					int g = *ptr++;
					int b = *ptr++;

					//a = 255;

					a = a * alphaCalc / 255;


					r = r*a / 255;
					g = g*a / 255;
					b = b*a / 255;


					*optr = a;
					*(optr + 1) = r;
					*(optr + 2) = g;
					*(optr + 3) = b;

					optr += 4;
				}
			}

			SetDIBits(iconDC, iconBmp, 0, 32, (LPVOID)ptr2, &dib, DIB_RGB_COLORS);

			BLENDFUNCTION bf;
			bf.BlendOp = AC_SRC_OVER;
			bf.AlphaFormat = AC_SRC_ALPHA;
			bf.BlendFlags = 0;
			bf.SourceConstantAlpha = reflectionalpha;
			AlphaBlend(dc, iconl, icont + 32, 32, 32, iconDC, 0, 0, 32, 32, bf);

			SelectObject(iconDC, old);
			DeleteObject(iconBmp);
			DeleteDC(iconDC);
		}
	}


	SelectObject(dc, old);
	DeleteDC(dc);

	return hbmp;
}

HRGN CreateRegionFromMask(SIZE s)
{
	if (!HasDWM())
		return NULL;

	TCHAR maskPath[MAX_PATH];
	GetFilePath(maskPath, _T("mask.bmp"));

	Bitmap bmpFile(maskPath);
	Bitmap bmp(s.cx, s.cy);
	Graphics g(&bmp);
	//g.Clear(Color(255,255,255,255));
	g.SetSmoothingMode(SmoothingModeNone);
	g.SetInterpolationMode(InterpolationModeLowQuality);
	g.DrawImage(&bmpFile, Rect(0, 0, s.cx, s.cy),
		0, 0, bmpFile.GetWidth(), bmpFile.GetHeight(),
		UnitPixel, 0, 0, 0);
	//BitmapData data = {0};
	//Rect rc(0, 0, bmp.GetWidth(), bmp.GetHeight());
	//bmp.LockBits(&rc, ImageLockModeRead,PixelFormat32bppARGB, &data);

	HRGN region = CreateRectRgn(-1, -1, 0, 0);

	for (size_t y = 0; y < bmp.GetHeight(); ++y)
	{
		//BYTE* ptr = ((BYTE*)(data.Scan0)+data.Stride*y);
		for (size_t x = 0; x < bmp.GetWidth(); ++x)
		{
			//*ptr++;
			//BOOL black = TRUE;
			//black &= *ptr++ == 0;
			//black &= *ptr++ == 0;
			//black &= *ptr++ == 0;
			Color c;
			bmp.GetPixel((INT)x, (INT)y, &c);
			BOOL black = c.GetB() == 0 && c.GetR() == 0 && c.GetG() == 0;

			if (black)
			{
				HRGN rcRegion = CreateRectRgn((INT)x, (INT)y, (INT)x + 1, (INT)y + 1);
				CombineRgn(region, rcRegion, region, RGN_OR);

				DeleteObject(rcRegion);
			}
		}
	}

	//bmp.UnlockBits(&data);

	return region;
}

HICON WINAPI _GetWindowIcon(HWND hwnd)
{
	if (!IsWindow(hwnd))
		return LoadIcon(NULL, IDI_WINLOGO);

	HICON hIcon = NULL;
	SendMessageTimeout(hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 100, (PDWORD_PTR)&hIcon);
	if (!hIcon) hIcon = (HICON)GetClassLongPtr(hwnd, GCLP_HICON);
	return hIcon;
}

void getBalloonIcon(NOTIFYICONDATA32* data)
{
	if (data->dwBalloonIcon)
		data->dwBalloonIcon = (DWORD)CopyIcon((HICON)data->dwBalloonIcon);
	else
	{
		if ((data->dwInfoFlags & NIIF_ERROR) == NIIF_ERROR)
			data->dwBalloonIcon = (DWORD)LoadIcon(NULL, IDI_ERROR);
		else if ((data->dwInfoFlags & NIIF_INFO) == NIIF_INFO)
			data->dwBalloonIcon = (DWORD)LoadIcon(NULL, IDI_INFORMATION);
		else if ((data->dwInfoFlags & NIIF_WARNING) == NIIF_WARNING)
			data->dwBalloonIcon = (DWORD)LoadIcon(NULL, IDI_WARNING);
		else if ((data->dwInfoFlags & NIIF_USER) == NIIF_USER)
			data->dwBalloonIcon = (DWORD)CopyIcon((HICON)data->dwIcon);
	}

	if (!data->dwBalloonIcon)
	{
		HICON hIcon = _GetWindowIcon((HWND)data->dwWnd);
		if (hIcon)
			data->dwBalloonIcon = (DWORD)CopyIcon(hIcon);
	}

	if (!data->dwBalloonIcon)
	{
		DWORD pid = 0;
		GetWindowThreadProcessId((HWND)data->dwWnd, &pid);
		if (pid != 0)
		{
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
				FALSE, pid);

			if (hProcess)
			{
				TCHAR exePath[MAX_PATH];
				if (GetModuleFileNameEx(hProcess, NULL, exePath, MAX_PATH))
				{
					SHFILEINFO info;
					if (SHGetFileInfo(exePath, 0, &info, sizeof(info), SHGFI_ICON | SHGFI_LARGEICON))
						data->dwBalloonIcon = (DWORD)info.hIcon;
				}

				CloseHandle(hProcess);
			}
		}
	}

	if (!data->dwBalloonIcon)
		data->dwBalloonIcon = (DWORD)LoadIcon(NULL, IDI_WINLOGO);
}

void CMainDlg::ShowBalloonTip(NOTIFYICONDATA32* data)
{
	if (!data)
		return;


	BalloonOptions* opts = (BalloonOptions*)LocalAlloc(LPTR, sizeof(BalloonOptions));
	if (!opts)
		return;

	HWND hwnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
		_T("AveGlassToolTips"), _T(""), WS_POPUP,
		0, 0, 220, 70, NULL, NULL, _Module.GetModuleInstance(), NULL);
	if (!hwnd)
	{
		LocalFree(opts);
		return;
	}

	balloonHwnd = hwnd;

	SetProp(hwnd, _T("options"), (HANDLE)opts);

	NOTIFYICONDATA32* buf = (NOTIFYICONDATA32*)LocalAlloc(LMEM_FIXED, data->cbSize);
	memcpy(buf, data, data->cbSize);
	getBalloonIcon(buf);


	Rect rcClose(0, 0, 0, 0);
	Rect rcOptions(0, 0, 0, 0);
	SIZE size;
	HBITMAP hbmp = paintBalloon(buf, &size, &rcClose, &rcOptions, OPTIONS_HIDDEN, hwnd);

	SetProp(hwnd, _T("closex"), (HANDLE)rcClose.X);
	SetProp(hwnd, _T("closey"), (HANDLE)rcClose.Y);
	SetProp(hwnd, _T("closew"), (HANDLE)rcClose.Width);
	SetProp(hwnd, _T("closeh"), (HANDLE)rcClose.Height);
	SetProp(hwnd, _T("optionsx"), (HANDLE)rcOptions.X);
	SetProp(hwnd, _T("optionsy"), (HANDLE)rcOptions.Y);
	SetProp(hwnd, _T("optionsw"), (HANDLE)rcOptions.Width);
	SetProp(hwnd, _T("optionsh"), (HANDLE)rcOptions.Height);
	SetProp(hwnd, _T("data"), (HANDLE)buf);
	SetProp(hwnd, _T("uid"), (HANDLE)data->uID);
	SetProp(hwnd, _T("msg"), (HANDLE)data->uCallbackMessage);
	SetProp(hwnd, _T("hwnd"), (HANDLE)data->dwWnd);
	SetProp(hwnd, _T("parent"), (HANDLE)m_hWnd);
	DWORD timeout = 17000;
	if (data->uTimeout != 0)
		timeout = data->uTimeout;
	SetProp(hwnd, _T("timeout"), (HANDLE)(3000 + timeout));


	HWND tray = FindWindow(_T("Shell_TrayWnd"), NULL);
	int t = 100;
	int l = 100;
	if (tray)
	{
		RECT rc;
		::GetWindowRect(tray, &rc);
		l = rc.right - size.cx - 0;
		t = rc.top - size.cy - 0;
	}
	if (GetOffsetValue(_T("stem"), _T("use"), 0))
	{
		POINT pt = { GetOffsetValue(_T("stem"), _T("xpos"), 0), GetOffsetValue(_T("stem"), _T("ypos"), 0) };
		POINT trayPos = TrayIconFinder::GetTrayIconPosition((HWND)buf->dwWnd, buf->uID);
		if (!(trayPos.x == 0 && trayPos.y == 0))
		{
			l = trayPos.x - pt.x + 8;
			t = trayPos.y - pt.y + 0;
			RECT rcDesk;
			::GetWindowRect(::GetDesktopWindow(), &rcDesk);
			if (l > rcDesk.right)
				l = rcDesk.right - size.cx;

			if (l < rcDesk.left)
				l = rcDesk.left;

			if (t > rcDesk.bottom)
				t = rcDesk.bottom - size.cy;

			if (t < rcDesk.top)
				t = rcDesk.top;
		}
	}

	balloonEvent(hwnd, NIN_BALLOONSHOW);

	SetLayeredWindowHBitmap(hwnd, hbmp, size, 0);
	DeleteObject(hbmp);
	HRGN region = CreateRegionFromMask(size);
	EnableBlurBehindWindow(hwnd, true, region);
	SetProp(hwnd, _T("region"), (HANDLE)region);
	::SetWindowPos(hwnd, 0, l, t, 0, 0, SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}

RECT closeRectFromBalloon(HWND hwnd)
{
	INT x = (INT)GetProp(hwnd, _T("closex"));
	INT y = (INT)GetProp(hwnd, _T("closey"));
	INT w = (INT)GetProp(hwnd, _T("closew"));
	INT h = (INT)GetProp(hwnd, _T("closeh"));

	RECT rc = { x, y, x + w, y + h };
	return rc;
}

RECT optionsRectFromBalloon(HWND hwnd)
{
	INT x = (INT)GetProp(hwnd, _T("optionsx"));
	INT y = (INT)GetProp(hwnd, _T("optionsy"));
	INT w = (INT)GetProp(hwnd, _T("optionsw"));
	INT h = (INT)GetProp(hwnd, _T("optionsh"));

	RECT rc = { x, y, x + w, y + h };
	return rc;
}

void checkCloseButton(HWND hWnd, LPARAM lParam, BYTE alpha, UINT msg)
{
	BalloonOptions* opts = ((BalloonOptions*)GetProp(hWnd, _T("options")));
	int& prevCloseState = ((BalloonOptions*)GetProp(hWnd, _T("options")))->prevCloseState;
	INT newCloseState = 0;

	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	RECT rc = closeRectFromBalloon(hWnd);
	if (PtInRect(&rc, pt))
		newCloseState = 1;
	else
		newCloseState = 0;

	if (msg == WM_NCMOUSELEAVE || msg == WM_MOUSELEAVE)
		newCloseState = 0;
	else if (msg == WM_LBUTTONDOWN && PtInRect(&rc, pt))
		newCloseState = 2;

	if (prevCloseState != newCloseState)
	{
		prevCloseState = newCloseState;

		SIZE s = { 0 };
		HBITMAP hbmp = paintBalloon((NOTIFYICONDATA32*)GetProp(hWnd, _T("data")), &s, NULL, NULL, newCloseState | opts->prevOptionsState, hWnd);
		SetLayeredWindowHBitmap(hWnd, hbmp, s, alpha);
		//SetWindowRgn(hWnd, (HRGN)GetProp(hWnd, L"region"), TRUE);
		EnableBlurBehindWindow(hWnd, true, (HRGN)GetProp(hWnd, _T("region")));
		DeleteObject(hbmp);
	}
}

void checkOptionsButton(HWND hWnd, LPARAM lParam, BYTE alpha, UINT msg, BOOL leaving = FALSE, BOOL visiting = FALSE)
{
	BalloonOptions* opts = ((BalloonOptions*)GetProp(hWnd, _T("options")));
	int& prevOptionsState = ((BalloonOptions*)GetProp(hWnd, _T("options")))->prevOptionsState;
	INT newOptionsState = 0;

	BOOL needsRedraw = FALSE;

	POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	RECT rc = optionsRectFromBalloon(hWnd);
	if (PtInRect(&rc, pt))
		newOptionsState = 4;
	else
		newOptionsState = 0;

	if (msg == WM_NCMOUSELEAVE || msg == WM_MOUSELEAVE)
		newOptionsState = 0;
	else if (msg == WM_LBUTTONDOWN && PtInRect(&rc, pt))
		newOptionsState = 8;

	if (GetProp(hWnd, _T("menuactive")))
		newOptionsState = 8;

	if (prevOptionsState != newOptionsState || leaving || visiting)
	{
		prevOptionsState = newOptionsState;

		SIZE s = { 0 };
		HBITMAP hbmp = paintBalloon((NOTIFYICONDATA32*)GetProp(hWnd, _T("data")), &s, NULL, NULL, newOptionsState, hWnd);
		SetLayeredWindowHBitmap(hWnd, hbmp, s, alpha);
		//SetWindowRgn(hWnd, (HRGN)GetProp(hWnd, L"region"), TRUE);
		EnableBlurBehindWindow(hWnd, true, (HRGN)GetProp(hWnd, _T("region")));
		DeleteObject(hbmp);
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BalloonOptions* opts = (BalloonOptions*)GetProp(hWnd, _T("options"));

	switch (msg)
	{

	case WM_SETCURSOR:
	{
		DWORD dwMsgPos = GetMessagePos();
		POINT pt = { GET_X_LPARAM(dwMsgPos), GET_Y_LPARAM(dwMsgPos) };
		ScreenToClient(hWnd, &pt);
		RECT rc2 = optionsRectFromBalloon(hWnd);
		if (PtInRect(&rc2, pt))
			SetCursor(LoadCursor(NULL, IDC_HAND));
		else
			SetCursor(LoadCursor(NULL, IDC_ARROW));

		return 1;
	}

	case WM_DESTROY:
	{
		NOTIFYICONDATA32* data = (NOTIFYICONDATA32*)GetProp(hWnd, _T("data"));
		if (data->dwBalloonIcon && !(data->dwInfoFlags & (NIIF_INFO | NIIF_WARNING | NIIF_ERROR)))
			DestroyIcon((HICON)data->dwBalloonIcon);

		LocalFree((HLOCAL)data);
		LocalFree((HLOCAL)GetProp(hWnd, _T("options")));
		DeleteObject((HRGN)GetProp(hWnd, _T("region")));
		PostMessage((HWND)GetProp(hWnd, _T("parent")), WM_APP_BYE, 0, 0);

		break;
	}

	case WM_TIMER:
	{
		if (wParam == 1)
		{
			opts->fadingIn = TRUE;
			SetLayeredAlpha(hWnd, opts->alpha);

			opts->alpha += 10;
			if (opts->alpha >= 255)
			{
				KillTimer(hWnd, 1);
				opts->alpha = 255;
				SetLayeredAlpha(hWnd, opts->alpha);
				if (!opts->mouseIsOn && !opts->ncmouseIsOn)
					SetTimer(hWnd, 2, (DWORD)3000, NULL);
			}
		}
		else if (wParam == 2)
		{
			if (GetProp(hWnd, _T("menuactive")) == NULL)
			{
				KillTimer(hWnd, 2);
				SetTimer(hWnd, 3, ((UINT)GetProp(hWnd, _T("timeout")) - 3000) / 255, NULL);
			}
		}
		else if (wParam == 3)
		{
			SetLayeredAlpha(hWnd, opts->alpha);

			opts->alpha -= 1;
			if (opts->alpha <= 0)
			{
				KillTimer(hWnd, 1);
				opts->alpha = 0;
				SetLayeredAlpha(hWnd, opts->alpha);
				balloonEvent(hWnd, NIN_BALLOONTIMEOUT);
				DestroyWindow(hWnd);
			}
		}

		break;
	}

	case WM_MOUSELEAVE:
	{
		opts->mouseIsOn = FALSE;
		SetTimer(hWnd, 2, (DWORD)3000, NULL);
		checkCloseButton(hWnd, lParam, opts->alpha, msg);
		checkOptionsButton(hWnd, lParam, opts->alpha, msg, TRUE, FALSE);

		break;
	}

	case WM_NCMOUSELEAVE:
	{
		opts->ncmouseIsOn = FALSE;
		SetTimer(hWnd, 2, (DWORD)3000, NULL);
		checkCloseButton(hWnd, lParam, opts->alpha, msg);
		checkOptionsButton(hWnd, lParam, opts->alpha, msg, TRUE, FALSE);

		break;
	}
	case WM_MOUSEMOVE:
	{
		if (!opts->mouseIsOn)
		{
			opts->tme.cbSize = sizeof(opts->tme);
			opts->tme.dwFlags = TME_LEAVE;
			opts->tme.hwndTrack = hWnd;
			TrackMouseEvent(&opts->tme);
		}

		BOOL prevState = opts->mouseIsOn;
		opts->mouseIsOn = TRUE;

		if (opts->isPressed && opts->startedPressOnClose)
			checkCloseButton(hWnd, lParam, opts->alpha, WM_LBUTTONDOWN);
		else
			checkCloseButton(hWnd, lParam, opts->alpha, msg);

		if (opts->isPressed && opts->startedPressOnOptions)
			checkOptionsButton(hWnd, lParam, opts->alpha, WM_LBUTTONDOWN, FALSE, !prevState);
		else
			checkOptionsButton(hWnd, lParam, opts->alpha, msg, FALSE, !prevState);

		KillTimer(hWnd, 3);
		KillTimer(hWnd, 2);
		SetTimer(hWnd, 1, 10, NULL);

		break;
	}

	case WM_NCMOUSEMOVE:
	{
		if (!opts->ncmouseIsOn)
		{
			opts->tme.cbSize = sizeof(opts->tme);
			opts->tme.dwFlags = TME_NONCLIENT | TME_LEAVE;
			opts->tme.hwndTrack = hWnd;
			TrackMouseEvent(&opts->tme);
		}

		BOOL prevState = opts->ncmouseIsOn;
		opts->ncmouseIsOn = TRUE;
		checkCloseButton(hWnd, lParam, opts->alpha, msg);
		checkOptionsButton(hWnd, lParam, opts->alpha, msg, FALSE, !prevState);

		KillTimer(hWnd, 3);
		KillTimer(hWnd, 2);
		SetTimer(hWnd, 1, 10, NULL);

		break;
	}

	case WM_LBUTTONDOWN:
	{
		SetCapture(hWnd);
		opts->isPressed = TRUE;
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		RECT rc = closeRectFromBalloon(hWnd);
		opts->startedPressOnClose = PtInRect(&rc, pt);
		if (opts->startedPressOnClose)
			checkCloseButton(hWnd, lParam, opts->alpha, msg);

		RECT rc2 = optionsRectFromBalloon(hWnd);
		opts->startedPressOnOptions = PtInRect(&rc2, pt);
		if (opts->startedPressOnOptions)
			checkOptionsButton(hWnd, lParam, opts->alpha, msg);

		break;
	}

	case WM_LBUTTONUP:
	{
		ReleaseCapture();
		opts->isPressed = FALSE;
		POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		RECT rc = closeRectFromBalloon(hWnd);
		RECT rc2 = optionsRectFromBalloon(hWnd);
		RECT rcClient;
		GetClientRect(hWnd, &rcClient);
		if (PtInRect(&rc2, pt) && opts->startedPressOnOptions)
		{
			KillTimer(hWnd, 1);
			KillTimer(hWnd, 2);
			KillTimer(hWnd, 3);
			SetProp(hWnd, _T("menuactive"), (HANDLE)1);
			HMENU menu = CreatePopupMenu();
			AppendMenu(menu, MF_STRING | MF_ENABLED, 1, _T("&GlassToasts Options"));
			AppendMenu(menu, MF_STRING | MF_ENABLED, 2, _T("&Stop GlassToasts"));

			RECT rcWin;
			GetWindowRect(hWnd, &rcWin);
			POINT ptMenu = { rcWin.left + rc2.right, rcWin.top + rc2.bottom };
			int cmd = TrackPopupMenu(menu, TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, ptMenu.x, ptMenu.y, 0, hWnd, NULL);
			switch (cmd)
			{
			case 1:
				ShowWindow((HWND)GetProp(hWnd, _T("parent")), SW_SHOW);
				break;

			case 2:
				PostMessage((HWND)GetProp(hWnd, _T("parent")), CMainDlg::WM_AVE_ACTION, 2, 0);
				break;
			}

			SetProp(hWnd, _T("menuactive"), (HANDLE)NULL);
			GetCursorPos(&pt);
			ScreenToClient(hWnd, &pt);
			checkOptionsButton(hWnd, MAKELPARAM(pt.x, pt.y), opts->alpha, msg);
		}
		if (PtInRect(&rc, pt) && opts->startedPressOnClose)
		{
			balloonEvent(hWnd, NIN_BALLOONTIMEOUT);
			DestroyWindow(hWnd);
		}
		else if (!opts->startedPressOnClose && !opts->startedPressOnOptions && PtInRect(&rcClient, pt))
		{
			balloonEvent(hWnd, NIN_BALLOONUSERCLICK);
			DestroyWindow(hWnd);
		}
		//ReleaseCapture();
		//SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);

		break;
	}

	case WM_CREATE:
	{
		SetTimer(hWnd, 1, 10, NULL);
		//return OnCreate(hWnd, message, wParam, lParam);

		break;
	}

	default:
	{
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	}
	return 0;
}
