#include "StdAfx.h"
#include "MainDlg.h"
#include "TrayIconFinder.h"

UINT CMainDlg::WM_AVE_ACTION = 0;

LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

HMODULE dwmLib = LoadLibrary(L"dwmapi.dll");

void getBalloonIcon(NOTIFYICONDATA* data);

BOOL HasDWM()
{
	return dwmLib != NULL;
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


const float PI = 3.14159265f;

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= 0;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;//MAKEINTRESOURCE(IDC_GLASSTOAST);
	wcex.lpszClassName	= L"AveGlassToolTips";
	wcex.hIconSm		= 0;//LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

void GetSkinName(WCHAR* skinName, DWORD nSize)
{
	WCHAR defaultPath[MAX_PATH] = {0};
	GetFilePath(defaultPath, L"defaults.ini", TRUE);

	WCHAR defaultSkinName[MAX_PATH] = {0};
	GetPrivateProfileString(L"settings", L"skin", L"Beta1", defaultSkinName, MAX_PATH, defaultPath);

	WCHAR appDataPath[MAX_PATH] = {0};
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath);
	PathAppend(appDataPath, L"avetoasts.ini");
	GetPrivateProfileString(L"settings", L"skin", defaultSkinName, skinName, MAX_PATH, appDataPath);
}

void WriteSkinName(const WCHAR* skinName)
{
	WCHAR appDataPath[MAX_PATH] = {0};
	SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appDataPath);
	PathAppend(appDataPath, L"avetoasts.ini");
	WritePrivateProfileString(L"settings", L"skin",skinName, appDataPath);
}

void GetFilePath(WCHAR* buf, const WCHAR* name, BOOL ignoreSkin)
{
	WCHAR skinName[MAX_PATH] = {0};
	if(!ignoreSkin)
	{
		GetSkinName(skinName, MAX_PATH);
	}

	GetModuleFileName(NULL, buf, MAX_PATH);
	PathRemoveFileSpec(buf);
	if(!ignoreSkin)
	{
		PathAppendW(buf, L"skins");
		PathAppendW(buf, skinName);
	}
	PathAppendW(buf, name);
}

INT GetOffsetValue(const WCHAR* name, const WCHAR* key, INT def)
{
	WCHAR path[MAX_PATH] = {0};
	GetFilePath(path, L"look.ini");

	return GetPrivateProfileInt(name, key, def, path);
}

void GetStringValue(const WCHAR* name, const WCHAR* key, const WCHAR* def, WCHAR* val, DWORD nSize)
{
	WCHAR path[MAX_PATH] = {0};
	GetFilePath(path, L"look.ini");

	GetPrivateProfileString(name, key, def, val, nSize, path);
}

COLORREF GetColor(const WCHAR* name, const WCHAR* key, COLORREF def)
{
	WCHAR color[MAX_PATH] = {0};
	GetStringValue(name, key, L"", color, MAX_PATH);
	if(wcslen(color) < 7)
		return def;

	const WCHAR* ptr = color;
	ptr++; // skip #

	WCHAR val[3] = {0};
	val[0] = *ptr++;
	val[1] = *ptr++;
	
	long r = wcstol(val, NULL, 16);

	val[0] = *ptr++;
	val[1] = *ptr++;
	
	long g = wcstol(val, NULL, 16);

	val[0] = *ptr++;
	val[1] = *ptr++;
	
	long b = wcstol(val, NULL, 16);

	return RGB(r,g,b);
}

DWORD CALLBACK GlassToastThread(void* lpVoid)
{
	NOTIFYICONDATA data = {0};
	data.cbSize = NOTIFYICONDATA_V2_SIZE;
	data.uFlags = NIF_INFO;
	data.hWnd = (HWND)lpVoid;
	data.uCallbackMessage = WM_TRAYCALLBACK;
	wcscpy_s(data.szInfoTitle, 64, L"Ave Glass Toast");
	wcscpy_s(data.szInfo, 256, L"Ave's Glass Toast has been started.\nClick here for additional options for Ave Glass Toast.");
	Shell_NotifyIcon(NIM_MODIFY, &data);

	ExitThread(0);

	return 0;
}

LRESULT CMainDlg::OnTrayCallback(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	if(NIN_BALLOONUSERCLICK == lParam)
	{
		ShowWindow(SW_SHOW);
		SetFocus();
	}

	return 0;
}

LRESULT CMainDlg::OnAveAction(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if(wParam == 1)
		OnBnClickedStarthook(0,0,0, bHandled);
	else if(2 == wParam)
		PostMessage(WM_CLOSE);
	else if(3 == wParam)
		ShowWindow(SW_SHOW);
	return 0;
}

void CMainDlg::LoadSkins()
{
	WCHAR path[MAX_PATH] = {0};
	WCHAR searchPath[MAX_PATH] = {0};
	GetFilePath(path, L"Skins\\", TRUE);
	GetFilePath(searchPath, L"Skins\\*.*", TRUE);

	WIN32_FIND_DATA data = {0};
	HANDLE h = FindFirstFile(searchPath, &data);
	if(INVALID_HANDLE_VALUE == h)
		return;

	CComboBox box = GetDlgItem(IDC_SKINS);
	box.ResetContent();

	do
	{
		if((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY &&
				wcscmp(data.cFileName, L".") != 0 && wcscmp(data.cFileName, L"..") != 0)
		{
			box.AddString(data.cFileName);
		}

	}while(FindNextFile(h,&data));

	FindClose(h);

	WCHAR skinName[MAX_PATH] = {0};
	GetSkinName(skinName, MAX_PATH);
	box.SelectString(0, skinName);


}

LRESULT CMainDlg::OnWindowsPosChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	WINDOWPOS* wp = reinterpret_cast<WINDOWPOS*>(lParam);
	if(NULL == wp)
		return 0;

	if(wp->flags & SWP_SHOWWINDOW)
	{
		LoadSkins();
	}

	return 0;
}

LRESULT CMainDlg::OnBnClickedStarthook(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	MyRegisterClass(_Module.GetModuleInstance());

	StartHook(hMod, m_hWnd);
	BOOL isRunning = IsHookRunning();
	::EnableWindow(GetDlgItem(IDC_STARTHOOK),!isRunning);
	::EnableWindow(GetDlgItem(IDC_STOPHOOK),isRunning);

	if(isRunning && showStartPopup)
	{
		DWORD id = 0;
		HANDLE h = CreateThread(0, 0, GlassToastThread, (LPVOID)m_hWnd, 0, &id);
		CloseHandle(h);
	}

	return 0;
}

LRESULT CMainDlg::OnBnClickedStophook(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	StopHook();
	BOOL isRunning = IsHookRunning();
	::EnableWindow(GetDlgItem(IDC_STARTHOOK),!isRunning);
	::EnableWindow(GetDlgItem(IDC_STOPHOOK),isRunning);

	return 0;
}

LRESULT CMainDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	ShowWindow(SW_HIDE);

	return 0;
}

LRESULT CMainDlg::OnCopyData(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	COPYDATASTRUCT* cds = reinterpret_cast<COPYDATASTRUCT*>(lParam);
	if(NULL == cds)
		return 0;

	if(cds->dwData != 1 || cds->cbData < NOTIFYICONDATA_V2_SIZE +8)
		return 0;

	NOTIFYICONDATA* data = (NOTIFYICONDATA*)((BYTE*)cds->lpData + 8);
	DWORD* msgType = (DWORD*)((BYTE*)cds->lpData + 4);
	if((data != NULL && (data->uFlags & NIF_INFO) == NIF_INFO))
	{
		if(NULL == data->szInfo || 0 == wcslen(data->szInfo))
		{
			if(balloonIsActive)
			{
				NOTIFYICONDATA*curData = (NOTIFYICONDATA*)GetProp(balloonHwnd, L"data");
				if(curData != NULL && curData->hWnd == data->hWnd && curData->uID == data->uID)
				{
					::PostMessage(balloonHwnd, WM_CLOSE,0,0);
				}
			}
		}
		else
		{
			//::MessageBox(0, L"we got a balloon.. part 2", 0, 0);
			NOTIFYICONDATA *buf = (NOTIFYICONDATA*)LocalAlloc(LMEM_FIXED, sizeof(NOTIFYICONDATA));
			if(NULL == buf)
				return 1;
			memset(buf, 0, sizeof(NOTIFYICONDATA));
			memcpy(buf, data, data->cbSize);
			buf->cbSize = sizeof(NOTIFYICONDATA);
			getBalloonIcon(buf);
			queuedBalloons.push(buf);
			if(!balloonIsActive)
				PostMessage(WM_APP_BYE);
		}

		//::MessageBox(0, L"balloon entered queue", 0,0);

		return 1;
	}

	return 0;
}

BOOL SetLayeredAlpha(HWND hwnd, BYTE alpha)
{
	if(NULL == hwnd)
		return FALSE;

	float progress = float(alpha) / 255.0f;
	float cur = sin(progress * PI/2.0f);
	BYTE correctedAlpha = (BYTE)(cur * 255.0f);

	BLENDFUNCTION bf = {0};
	bf.BlendOp = AC_SRC_OVER;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = correctedAlpha;
	return UpdateLayeredWindow(hwnd, 0, 0, 0, 0, 0, 0, &bf, ULW_ALPHA);
}

BOOL SetLayeredWindow(HWND hwnd, Bitmap* bmp, BYTE alpha=255)
{
	if(NULL == bmp || NULL == hwnd)
		return FALSE;

	HBITMAP hbmp = NULL;
	bmp->GetHBITMAP(NULL, &hbmp);
	if(NULL == hbmp)
		return FALSE;

	SIZE s = {bmp->GetWidth(), bmp->GetHeight()};
	POINT pt = {0,0};

	HDC dc = CreateCompatibleDC(0);
	HBITMAP oldbmp = (HBITMAP)SelectObject(dc, hbmp);

	BLENDFUNCTION bf = {0};
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

BOOL SetLayeredWindowHBitmap(HWND hwnd, HBITMAP hbmp,  SIZE s, BYTE alpha=255)
{
	if(NULL == hbmp || NULL == hwnd)
		return FALSE;

	if(NULL == hbmp)
		return FALSE;
	POINT pt = {0,0};

	HDC dc = CreateCompatibleDC(0);
	HBITMAP oldbmp = (HBITMAP)SelectObject(dc, hbmp);

	BLENDFUNCTION bf = {0};
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
                               HRGN region = 0,
                               bool transitionOnMaximized = false)
{
	if(!HasDWM())
		return S_FALSE;

   DWM_BLURBEHIND blurBehind = { 0 };
    
    blurBehind.dwFlags = DWM_BB_ENABLE | DWM_BB_TRANSITIONONMAXIMIZED;
    blurBehind.fEnable = enable;
    blurBehind.fTransitionOnMaximized = transitionOnMaximized;

    if (enable && 0 != region)
    {
        blurBehind.dwFlags |= DWM_BB_BLURREGION;
        blurBehind.hRgnBlur = region;
    }

    return ::DwmEnableBlurBehindWindow(window,
                                       &blurBehind);
}

void balloonEvent(HWND hwnd, LPARAM msg)
{
	DWORD uid = (DWORD)GetProp(hwnd, L"uid");
	UINT message = (UINT)GetProp(hwnd, L"msg");
	HWND win  = (HWND)GetProp(hwnd, L"hwnd");

	PostMessage(win, message, (WPARAM)uid, msg);
}

const int MOUSE_OTHER = 0;
const int MOUSE_ON = 1;
const int MOUSE_PRESSED = 2;
const int OPTIONS_ON = 4;
const int OPTIONS_PRESSED = 8;
const int OPTIONS_HIDDEN = 16;


HBITMAP paintBalloon(NOTIFYICONDATA* data, SIZE* size, Rect* closeRc, Rect* optionsRc, int states, HWND hwnd)
{
	WCHAR bgPath[MAX_PATH] = {0};
	WCHAR insetPath[MAX_PATH] = {0};
	WCHAR closePath[MAX_PATH] = {0};
	GetFilePath(bgPath, L"bgshadow.png");
	GetFilePath(insetPath, L"inset.png");
	if(states & 1)
		GetFilePath(closePath, L"closehover.png");
	else if(states &  2)
		GetFilePath(closePath, L"closepress.png");
	else 
		GetFilePath(closePath, L"close.png");

	
	Bitmap bmp(bgPath);
	Bitmap bmp2(insetPath);
	Bitmap bmp3(closePath);

	SIZE s = {bmp.GetWidth(), bmp.GetHeight()};

	INT minWidth  = GetOffsetValue(L"global", L"minwidth",205);
	INT minHeight = GetOffsetValue(L"global", L"minheight",100);
	INT maxWidth = GetOffsetValue(L"global", L"maxwidth",300);
	INT maxHeight = GetOffsetValue(L"global", L"maxheight",100);

	INT extraWidth = GetOffsetValue(L"global", L"extrawidth",70);
	INT extraHeight = GetOffsetValue(L"global", L"extraheight",20);
	

	HDC dc = CreateCompatibleDC(0);

	WCHAR textFont[LF_FACESIZE] = {0};
	GetStringValue(L"text", L"facename", L"Segoe UI", textFont, LF_FACESIZE);

	LOGFONT lf = {-12,0,0,0,400,0,0,0,0,3,2,1,34,0};
	wcscpy_s(lf.lfFaceName, LF_FACESIZE, textFont);
	HFONT hf = CreateFontIndirect(&lf);
	HFONT oldFont = (HFONT)SelectObject(dc, hf);

	RECT rcMeasure = {0,0,maxWidth-extraWidth,maxHeight - extraHeight};
	if(HasDWM())
	{
		DrawText(dc, data->szInfo , -1, &rcMeasure, DT_WORDBREAK | DT_CALCRECT);
	}
	else
	{
		Graphics g(dc);
		Font font(textFont, 10);
		RectF layoutRect(0,0, (REAL)(maxWidth-extraWidth), (REAL)(maxHeight-extraHeight));
		RectF boundingBox(0,0,0,0);
		g.MeasureString(data->szInfo,-1, &font, layoutRect, &boundingBox);
		rcMeasure.right = (INT)boundingBox.Width;
		rcMeasure.bottom = (INT)boundingBox.Height;
	}

	INT width = rcMeasure.right - rcMeasure.left;
	INT height = rcMeasure.bottom - rcMeasure.top;

	s.cx = max(minWidth-extraWidth, width) + extraWidth;
	s.cy = max(minHeight-extraHeight, height) + extraHeight;
	
	int xcor = GetOffsetValue(L"global", L"xcorrection",20);
	int ycor = GetOffsetValue(L"global", L"ycorrection",50);

	int fixX = 0;
	if(xcor != 0)
		fixX = (s.cx - minWidth) / xcor;
	int fixY = 0;
	if(ycor != 0)
		fixY = (s.cy - minHeight)  / ycor;


	int textl = GetOffsetValue(L"text", L"left",54);
	int textt = GetOffsetValue(L"text", L"top",39);
	RECT rcText = {textl - fixX,textt, textl + width - fixX , height+textt - fixY};
	//rcText = r;

	if(size != NULL)
		*size = s;


	BITMAPINFO dib = {0};
	HBITMAP hbmp = 0;
	dib.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    dib.bmiHeader.biWidth = s.cx;
    dib.bmiHeader.biHeight = -s.cy;
    dib.bmiHeader.biPlanes = 1;
    dib.bmiHeader.biBitCount = 32;
    dib.bmiHeader.biCompression = BI_RGB;
 
    hbmp = CreateDIBSection ( dc, &dib, DIB_RGB_COLORS, NULL, NULL, 0 );

	
	HBITMAP old = (HBITMAP)SelectObject(dc, hbmp);


	Graphics g(dc);
	g.SetSmoothingMode(SmoothingModeAntiAlias);
	g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
	g.DrawImage(&bmp, Rect(0,0,s.cx, s.cy),
						0,0, bmp.GetWidth(), bmp.GetHeight(),
						UnitPixel, 0,0,0);

	int intsetl = GetOffsetValue(L"inset", L"left",46);
	int intsett = GetOffsetValue(L"inset", L"top",23);
	int intsetr = GetOffsetValue(L"inset", L"right",57);
	int intsetb = GetOffsetValue(L"inset", L"bottom",29);

	g.DrawImage(&bmp2, Rect(intsetl,intsett,s.cx-intsetr - fixX,s.cy-intsetb - fixY),
	   0,0, bmp2.GetWidth(), bmp2.GetHeight(), UnitPixel, 0,0,0);

	int closelo = GetOffsetValue(L"close", L"leftoffset",15);
	int closet = GetOffsetValue(L"close", L"top",7);

   Rect rcClose(s.cx-bmp3.GetWidth()-closelo - fixX,closet - fixY,bmp3.GetWidth(),bmp3.GetHeight());
    g.DrawImage(&bmp3, rcClose,
	   0,0, bmp3.GetWidth(), bmp3.GetHeight(), UnitPixel, 0,0,0);

	if(closeRc != NULL)
		*closeRc = rcClose;

	int headerl = GetOffsetValue(L"header", L"left",54);
	int headert = GetOffsetValue(L"header", L"top",24);
	int headerr = GetOffsetValue(L"header", L"right",24);
	int headerb = GetOffsetValue(L"header", L"bottom",45);

	RECT rcHeaderText = {headerl - fixX,headert - fixY, s.cx - headerr - fixX, headerb - fixY};

	COLORREF textColor = GetColor(L"text", L"color", RGB(99,99,99));
	COLORREF headerColor = GetColor(L"header", L"color", RGB(0,100,120));

	WCHAR headerFont[LF_FACESIZE] = {0};
	GetStringValue(L"header", L"facename", L"Segoe UI", headerFont, LF_FACESIZE);
	if(HasDWM())
	{
		HTHEME hTheme = OpenThemeData(hwnd, L"globals");
		DTTOPTS opts = {0};
		opts.dwSize = sizeof(opts);
		opts.fApplyOverlay = TRUE;
		opts.crText = textColor;
		opts.dwFlags = DTT_COMPOSITED | DTT_TEXTCOLOR;
		HMODULE uxthemeLib = LoadLibrary(L"uxtheme.dll");
		HRESULT (CALLBACK *PDrawThemeTextEx)(HTHEME,HDC,int,int,LPCWSTR,int,DWORD,LPRECT,const DTTOPTS *) = 
			(HRESULT (CALLBACK*)(HTHEME,HDC,int,int,LPCWSTR,int,DWORD,LPRECT,const DTTOPTS *)) 
			GetProcAddress(uxthemeLib, "DrawThemeTextEx");

		if(PDrawThemeTextEx != NULL)
			PDrawThemeTextEx(hTheme, dc, 0, 0, data->szInfo, -1, DT_WORDBREAK, &rcText, &opts);

		FreeLibrary(uxthemeLib);

		LOGFONT lf2 = {-16,0,0,0,400,0,0,0,0,3,2,1,34,0};
		wcscpy_s(lf2.lfFaceName, LF_FACESIZE, headerFont);
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

	WCHAR optionsPath[MAX_PATH] = {0};
	if(states & OPTIONS_ON)
		GetFilePath(optionsPath, L"optionshover.png");
	else if(states & OPTIONS_PRESSED)
		GetFilePath(optionsPath, L"optionspress.png");
	else
		GetFilePath(optionsPath, L"options.png");
	Bitmap bmpOptions(optionsPath);

	int optionslo = GetOffsetValue(L"options", L"leftoffset",17);
	int optionst = GetOffsetValue(L"options", L"top",4);

	Rect rcOptions(s.cx - bmp3.GetWidth() -optionslo - bmpOptions.GetWidth() - fixX, optionst - fixY, bmpOptions.GetWidth(), bmpOptions.GetHeight());
	if(optionsRc != NULL)
		*optionsRc = rcOptions;

	BalloonOptions* opts = (BalloonOptions*)GetProp(hwnd, L"options");
	if(opts != NULL && (opts->mouseIsOn || opts->ncmouseIsOn) || GetProp(hwnd, L"menuactive"))
		g.DrawImage(&bmpOptions,rcOptions,0,0,bmpOptions.GetWidth(), bmpOptions.GetHeight(), UnitPixel, 0,0,0);

	int iconl = GetOffsetValue(L"icon", L"left",14);
	int icont = GetOffsetValue(L"icon", L"top",-1);
	if(-1 == icont)
		icont = s.cy / 2 - 16;

	if(data->hBalloonIcon != NULL)
	{
		

		DrawIcon(dc, iconl, icont, data->hBalloonIcon);

		if(GetOffsetValue(L"icon", L"usereflection", 1)) // useReflection
		{
			int reflectionpixeltreshold = 32 - GetOffsetValue(L"icon", L"reflectionpixeltreshold", 24);
			int reflectionalpha = GetOffsetValue(L"icon", L"reflectionalpha", 150);
			HDC iconDC = CreateCompatibleDC(0);
			BITMAPINFO dib = {0};
			HBITMAP iconBmp = 0;
			dib.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			dib.bmiHeader.biWidth = 32;
			dib.bmiHeader.biHeight = 32;
			dib.bmiHeader.biPlanes = 1;
			dib.bmiHeader.biBitCount = 32;
			dib.bmiHeader.biCompression = BI_RGB;
	 
			iconBmp = CreateDIBSection ( dc, &dib, DIB_RGB_COLORS, NULL, NULL, 0 );
			HBITMAP old = (HBITMAP)SelectObject(iconDC, iconBmp);
			DrawIcon(iconDC, 0,0, data->hBalloonIcon);

			BYTE data[32*32*4] = {0};
			LPBYTE ptr = data;
			BYTE data2[32*32*4];
			LPBYTE ptr2 = data2;
			LPBYTE optr = (&ptr2[0])+32*32*4;
			LPBYTE endptr = (&ptr2[0])+32*32*4;
			GetBitmapBits(iconBmp, 32*32*4, (LPVOID)ptr);
			int alphaCalc=0;
			for(int y=0; y < 32; ++y)
			{
				optr = endptr-32*4*(y+1); 
				if(y > reflectionpixeltreshold)
				{
					alphaCalc = reflectionalpha/(32-reflectionpixeltreshold) * (y-reflectionpixeltreshold);
					if(alphaCalc > 255)
						alphaCalc = 255;
				}
				else
				{
					alphaCalc = 0;
				}
				for(int x=0; x < 32; ++x)
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
					*(optr+1) = r;
					*(optr+2) = g;
					*(optr+3) = b;

					optr+=4;
				}
			}

			SetBitmapBits(iconBmp, 32*32*4, (LPVOID)ptr2);

			BLENDFUNCTION bf = {0};
			bf.BlendOp = AC_SRC_OVER;
			bf.AlphaFormat = AC_SRC_ALPHA;
			bf.BlendFlags = 0;
			bf.SourceConstantAlpha = reflectionalpha;
			AlphaBlend(dc, iconl, icont+32, 32, 32, iconDC, 0,0,32,32, bf);

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
	if(!HasDWM())
		return 0;

	WCHAR maskPath[MAX_PATH] = {0};
	GetFilePath(maskPath, L"mask.bmp");

	Bitmap bmpFile(maskPath);
	Bitmap bmp(s.cx, s.cy);
	Graphics g(&bmp);
	//g.Clear(Color(255,255,255,255));
	g.SetSmoothingMode(SmoothingModeNone);
	g.SetInterpolationMode(InterpolationModeLowQuality);
	g.DrawImage(&bmpFile, Rect(0,0,s.cx, s.cy),
						0,0, bmpFile.GetWidth(), bmpFile.GetHeight(),
						UnitPixel, 0,0,0);
	//BitmapData data = {0};
	//Rect rc(0, 0, bmp.GetWidth(), bmp.GetHeight());
	//bmp.LockBits(&rc, ImageLockModeRead,PixelFormat32bppARGB, &data);

	HRGN region = CreateRectRgn(-1,-1,0,0);
	
	for(size_t y = 0; y < bmp.GetHeight(); ++y)
	{
		//BYTE* ptr = ((BYTE*)(data.Scan0)+data.Stride*y);
		for(size_t x = 0; x < bmp.GetWidth(); ++x)
		{
			//*ptr++;
			//BOOL black = TRUE;
			//black &= *ptr++ == 0;
			//black &= *ptr++ == 0;
			//black &= *ptr++ == 0;
			Color c;
			bmp.GetPixel((INT)x,(INT)y, &c);
			BOOL black = c.GetB() == 0 && c.GetR() == 0 && c.GetG() == 0;

			if(black)
			{
				HRGN rcRegion = CreateRectRgn((INT)x,(INT)y,(INT)x+1,(INT)y+1);
				CombineRgn(region, rcRegion, region, RGN_OR);

				DeleteObject(rcRegion);
			}
		}
	}

	//bmp.UnlockBits(&data);

	return region;
}

LRESULT CMainDlg::OnBalloonFinished(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	balloonHwnd = NULL;
	balloonIsActive = FALSE;
	if(!queuedBalloons.empty())
	{
		NOTIFYICONDATA* buf = queuedBalloons.front();
		queuedBalloons.pop();
		ShowBalloonTip(buf);
		if(buf->hBalloonIcon != NULL)
			DestroyIcon(buf->hBalloonIcon);
		LocalFree(buf);
	}
	return 0;
}

HICON WINAPI _GetWindowIcon(HWND hwnd)
{
	HICON hIcon = NULL;

	if(!IsWindow(hwnd))
	return (HICON) LoadIcon(NULL, IDI_WINLOGO);

 	SendMessageTimeout(hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 100, (PDWORD_PTR) &hIcon);
	if (!hIcon) hIcon = (HICON) GetClassLongPtr(hwnd, GCLP_HICON);
	return hIcon;
}


void getBalloonIcon(NOTIFYICONDATA* data)
{
	if(data->hBalloonIcon != NULL)
		data->hBalloonIcon = CopyIcon(data->hBalloonIcon);
	else
	{
		if((data->dwInfoFlags & NIIF_ERROR) == NIIF_ERROR)
			data->hBalloonIcon = LoadIcon(NULL, IDI_ERROR);
		else if((data->dwInfoFlags & NIIF_INFO) == NIIF_INFO)
			data->hBalloonIcon = LoadIcon(NULL, IDI_INFORMATION);
		else if((data->dwInfoFlags & NIIF_WARNING) == NIIF_WARNING)
			data->hBalloonIcon = LoadIcon(NULL, IDI_WARNING);
		else if((data->dwInfoFlags & NIIF_USER) == NIIF_USER)
			data->hBalloonIcon = CopyIcon(data->hIcon);

		data->dwInfoFlags = 0;
	}

	if(NULL == data->hBalloonIcon)
	{
		HICON hIcon = _GetWindowIcon(data->hWnd);
		if(hIcon != NULL)
			data->hBalloonIcon = CopyIcon(hIcon);
	}

	if(NULL == data->hBalloonIcon)
	{
		DWORD pid = 0;
		GetWindowThreadProcessId(data->hWnd, &pid);
		if(pid != NULL)
		{
			HANDLE hProcess =  OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |PROCESS_VM_READ, 
				FALSE, pid);

			WCHAR exePath[MAX_PATH] = {0};
			GetModuleFileNameEx(hProcess, NULL, exePath, MAX_PATH);
			if(wcslen(exePath) > 0)
			{
				SHFILEINFO info = {0};
				SHGetFileInfo(exePath,0, &info, sizeof(info), SHGFI_ICON | SHGFI_LARGEICON);
				if(info.hIcon != NULL)
					data->hBalloonIcon = info.hIcon;
			}

			CloseHandle(hProcess);
		}
	}

	if (data->hBalloonIcon == NULL) 
		data->hBalloonIcon = LoadIcon(NULL, IDI_WINLOGO);
}

void CMainDlg::ShowBalloonTip(NOTIFYICONDATA* data)
{
	if(NULL == data)
		return;

	balloonIsActive = TRUE;


	BalloonOptions* opts = (BalloonOptions*)LocalAlloc(LMEM_FIXED, sizeof(BalloonOptions));
	memset(opts, 0, sizeof(BalloonOptions));

   HWND hwnd = CreateWindowEx(WS_EX_TOOLWINDOW |WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE, 
	   L"AveGlassToolTips", L"", WS_POPUP ,
      0, 0, 220, 70, NULL, NULL, _Module.GetModuleInstance(), NULL);

   balloonHwnd = hwnd;

   SetProp(hwnd, L"options", (HANDLE)opts);

	NOTIFYICONDATA* buf = (NOTIFYICONDATA*)LocalAlloc(LMEM_FIXED, data->cbSize);
	memcpy(buf, data, data->cbSize);
	getBalloonIcon(buf);
	

	Rect rcClose(0,0,0,0);
	Rect rcOptions(0,0,0,0);
	SIZE size = {0};
	HBITMAP hbmp = paintBalloon(buf, &size, &rcClose, &rcOptions, OPTIONS_HIDDEN, hwnd);

	SetProp(hwnd, L"closex", (HANDLE)rcClose.X);
    SetProp(hwnd, L"closey", (HANDLE)rcClose.Y);
    SetProp(hwnd, L"closew", (HANDLE)rcClose.Width);
    SetProp(hwnd, L"closeh", (HANDLE)rcClose.Height);
	SetProp(hwnd, L"optionsx", (HANDLE)rcOptions.X);
	SetProp(hwnd, L"optionsy", (HANDLE)rcOptions.Y);
	SetProp(hwnd, L"optionsw", (HANDLE)rcOptions.Width);
	SetProp(hwnd, L"optionsh", (HANDLE)rcOptions.Height);
	SetProp(hwnd, L"data", (HANDLE)buf);
	SetProp(hwnd, L"uid", (HANDLE)data->uID);
	SetProp(hwnd, L"msg", (HANDLE)data->uCallbackMessage);
	SetProp(hwnd, L"hwnd", (HANDLE)data->hWnd);
	SetProp(hwnd, L"parent", (HANDLE)m_hWnd);
	DWORD timeout = 17000;
	if(data->uTimeout != 0)
		timeout = data->uTimeout;
	SetProp(hwnd, L"timeout", (HANDLE)(3000+timeout));

   
	RECT rc = {0};
   HWND tray = FindWindow(L"Shell_TrayWnd", NULL);
   int t = 100;
   int l = 100;
   if(tray != NULL)
   {
	   ::GetWindowRect(tray,&rc);
	   l = rc.right - size.cx - 0;
	   t = rc.top - size.cy -  0;
   }
	   if(GetOffsetValue(L"stem", L"use", 0))
	   {
		   POINT pt = {GetOffsetValue(L"stem", L"xpos", 0), GetOffsetValue(L"stem", L"ypos", 0)};
		   POINT trayPos = TrayIconFinder::GetTrayIconPosition(buf->hWnd, buf->uID);
		   if(!(trayPos.x == 0 && trayPos.y == 0))
		   {
			l = trayPos.x - pt.x + 8;
			t = trayPos.y - pt.y + 0;
			RECT rcDesk = {0};
			::GetWindowRect(::GetDesktopWindow(), &rcDesk);
			if( l > rcDesk.right)
				l = rcDesk.right - size.cx;

			if(l < rcDesk.left )
				l = rcDesk.left;

			if(t > rcDesk.bottom)
				t = rcDesk.bottom - size.cy;

			if(t < rcDesk.top)
				t = rcDesk.top;
		   }
	   }

   balloonEvent(hwnd, NIN_BALLOONSHOW);

   SetLayeredWindowHBitmap(hwnd, hbmp, size, 0);
   DeleteObject(hbmp);
  HRGN region = CreateRegionFromMask(size);
   EnableBlurBehindWindow(hwnd, true, region);
   SetProp(hwnd, L"region", (HANDLE)region);
   ::SetWindowPos(hwnd, 0, l, t, 0, 0, SWP_SHOWWINDOW | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
}


RECT closeRectFromBalloon(HWND hwnd)
{
	INT x = (INT)GetProp(hwnd, L"closex");
	INT y = (INT)GetProp(hwnd, L"closey");
	INT w = (INT)GetProp(hwnd, L"closew");
	INT h = (INT)GetProp(hwnd, L"closeh");

	RECT rc = {x,y, x+w, y+h};
	return rc;
}

RECT optionsRectFromBalloon(HWND hwnd)
{
	INT x = (INT)GetProp(hwnd, L"optionsx");
	INT y = (INT)GetProp(hwnd, L"optionsy");
	INT w = (INT)GetProp(hwnd, L"optionsw");
	INT h = (INT)GetProp(hwnd, L"optionsh");

	RECT rc = {x,y, x+w, y+h};
	return rc;
}

void checkCloseButton(HWND hWnd, LPARAM lParam, BYTE alpha, UINT msg)
{
	BalloonOptions* opts = ((BalloonOptions*)GetProp(hWnd, L"options"));
	int& prevCloseState = ((BalloonOptions*)GetProp(hWnd, L"options"))->prevCloseState;
			INT newCloseState = 0;

			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			RECT rc = closeRectFromBalloon(hWnd);
			if(PtInRect(&rc, pt))
				newCloseState = 1;
			else
				newCloseState = 0;

			if(msg == WM_NCMOUSELEAVE || msg == WM_MOUSELEAVE)
				newCloseState = 0;
			else if(msg == WM_LBUTTONDOWN && PtInRect(&rc, pt))
				newCloseState = 2;

			if(prevCloseState != newCloseState)
			{
				prevCloseState = newCloseState;

				SIZE s = {0};
				HBITMAP hbmp = paintBalloon((NOTIFYICONDATA*)GetProp(hWnd, L"data"),&s,NULL, NULL, newCloseState | opts->prevOptionsState , hWnd);
				SetLayeredWindowHBitmap(hWnd, hbmp, s, alpha);
				//SetWindowRgn(hWnd, (HRGN)GetProp(hWnd, L"region"), TRUE);
				EnableBlurBehindWindow(hWnd, true, (HRGN)GetProp(hWnd, L"region"));
				DeleteObject(hbmp);
			}
}

void checkOptionsButton(HWND hWnd, LPARAM lParam, BYTE alpha, UINT msg, BOOL leaving = FALSE, BOOL visiting=FALSE)
{
	BalloonOptions* opts = ((BalloonOptions*)GetProp(hWnd, L"options"));
	int& prevOptionsState = ((BalloonOptions*)GetProp(hWnd, L"options"))->prevOptionsState;
			INT newOptionsState = 0;

			BOOL needsRedraw = FALSE;

			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			RECT rc = optionsRectFromBalloon(hWnd);
			if(PtInRect(&rc, pt))
				newOptionsState = 4;
			else
				newOptionsState = 0;

			if(msg == WM_NCMOUSELEAVE || msg == WM_MOUSELEAVE)
				newOptionsState = 0;
			else if(msg == WM_LBUTTONDOWN && PtInRect(&rc, pt))
				newOptionsState = 8;

			if(GetProp(hWnd, L"menuactive") != NULL)
				newOptionsState = 8;

			if(prevOptionsState != newOptionsState || leaving || visiting)
			{
				prevOptionsState = newOptionsState;

				SIZE s = {0};
				HBITMAP hbmp = paintBalloon((NOTIFYICONDATA*)GetProp(hWnd, L"data"),&s,NULL, NULL, newOptionsState, hWnd);
				SetLayeredWindowHBitmap(hWnd, hbmp, s, alpha);
				//SetWindowRgn(hWnd, (HRGN)GetProp(hWnd, L"region"), TRUE);
				EnableBlurBehindWindow(hWnd, true, (HRGN)GetProp(hWnd, L"region"));
				DeleteObject(hbmp);
			}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BalloonOptions* opts = (BalloonOptions*)GetProp(hWnd, L"options");

	switch (msg)
	{

	case WM_SETCURSOR:
		if(1)
		{
			DWORD dwMsgPos = GetMessagePos();
			POINT pt = {GET_X_LPARAM(dwMsgPos), GET_Y_LPARAM(dwMsgPos)};
			ScreenToClient(hWnd, &pt);
			RECT rc2 = optionsRectFromBalloon(hWnd);
			if(PtInRect(&rc2, pt))
				SetCursor(LoadCursor(NULL, IDC_HAND));
			else
				SetCursor(LoadCursor(NULL, IDC_ARROW));

			return 1;
		}
	break;

	case WM_DESTROY:
		if(1)
		{
			NOTIFYICONDATA* data = (NOTIFYICONDATA*) GetProp(hWnd, L"data");
			if(data->hBalloonIcon != NULL)
				DestroyIcon(data->hBalloonIcon);
		}
		LocalFree((HLOCAL)GetProp(hWnd, L"data"));
		LocalFree((HLOCAL)GetProp(hWnd, L"options"));
		DeleteObject((HRGN)GetProp(hWnd, L"region"));
		PostMessage((HWND)GetProp(hWnd, L"parent"), WM_APP_BYE, 0, 0);
		break;
	case WM_TIMER:
		if(1 == wParam)
		{
			opts->fadingIn = TRUE;
			SetLayeredAlpha(hWnd, opts->alpha);

			opts->alpha+=10;
			if(opts->alpha >= 255)
			{
				KillTimer(hWnd, 1);
				opts->alpha = 255;
				SetLayeredAlpha(hWnd, opts->alpha);
				if(!opts->mouseIsOn && !opts->ncmouseIsOn)
					SetTimer(hWnd, 2, (DWORD)3000,0);
			}
		}
		else if(2 == wParam)
		{
			if(GetProp(hWnd, L"menuactive") == NULL)
			{
				KillTimer(hWnd,2);
				SetTimer(hWnd, 3, ((UINT)GetProp(hWnd, L"timeout")-3000)/255,0);
			}
		}
		else if(3 == wParam)
		{
			SetLayeredAlpha(hWnd, opts->alpha);

			opts->alpha-=1;
			if(opts->alpha <= 0)
			{
				KillTimer(hWnd, 1);
				opts->alpha = 0;
				SetLayeredAlpha(hWnd, opts->alpha);
				balloonEvent(hWnd, NIN_BALLOONTIMEOUT);
				DestroyWindow(hWnd);
			}
		}

		break;
	case WM_MOUSELEAVE:
			opts->mouseIsOn = FALSE;
			SetTimer(hWnd, 2, (DWORD)3000,0);
			checkCloseButton(hWnd, lParam, opts->alpha, msg);
			checkOptionsButton(hWnd, lParam, opts->alpha, msg, TRUE, FALSE);
		break;
	case WM_NCMOUSELEAVE:
			opts->ncmouseIsOn = FALSE;
			SetTimer(hWnd, 2, (DWORD)3000,0);
			checkCloseButton(hWnd, lParam, opts->alpha, msg);
			checkOptionsButton(hWnd, lParam, opts->alpha, msg, TRUE, FALSE);

		break;
	case WM_MOUSEMOVE:
		if(!opts->mouseIsOn)
		{
			opts->tme.cbSize = sizeof(opts->tme);
			opts->tme.dwFlags = TME_LEAVE;
			opts->tme.hwndTrack = hWnd;
			TrackMouseEvent(&opts->tme);
		}
		if(1)
		{
			BOOL prevState = opts->mouseIsOn;
			opts->mouseIsOn = TRUE;

			if(opts->isPressed && opts->startedPressOnClose)
				checkCloseButton(hWnd, lParam, opts->alpha, WM_LBUTTONDOWN);
			else
				checkCloseButton(hWnd, lParam, opts->alpha,  msg);

			if(opts->isPressed && opts->startedPressOnOptions)
				checkOptionsButton(hWnd, lParam, opts->alpha, WM_LBUTTONDOWN, FALSE, !prevState);
			else
				checkOptionsButton(hWnd, lParam, opts->alpha, msg, FALSE, !prevState);

			
			KillTimer(hWnd, 3);
			KillTimer(hWnd, 2);
			SetTimer(hWnd, 1, 10, 0);
		}
		break;

	case WM_NCMOUSEMOVE:
		if(!opts->ncmouseIsOn)
		{
			opts->tme.cbSize = sizeof(opts->tme);
			opts->tme.dwFlags = TME_NONCLIENT | TME_LEAVE;
			opts->tme.hwndTrack = hWnd;
			TrackMouseEvent(&opts->tme);
		}

		if(1)
		{
			BOOL prevState = opts->ncmouseIsOn;
			opts->ncmouseIsOn = TRUE;
			checkCloseButton(hWnd, lParam, opts->alpha, msg);
			checkOptionsButton(hWnd, lParam, opts->alpha, msg, FALSE, !prevState);
			
			KillTimer(hWnd, 3);
			KillTimer(hWnd, 2);
			SetTimer(hWnd, 1, 10, 0);
		}
		break;

	case WM_LBUTTONDOWN:
		if(1)
		{
			SetCapture(hWnd);
			opts->isPressed = TRUE;
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			RECT rc = closeRectFromBalloon(hWnd);
			opts->startedPressOnClose = PtInRect(&rc, pt);
			if(opts->startedPressOnClose)
				checkCloseButton(hWnd, lParam, opts->alpha, msg);

			RECT rc2 = optionsRectFromBalloon(hWnd);
			opts->startedPressOnOptions = PtInRect(&rc2, pt);
			if(opts->startedPressOnOptions)
				checkOptionsButton(hWnd, lParam, opts->alpha, msg);
		}
	break;

	case WM_LBUTTONUP:
		if(1)
		{
			ReleaseCapture();
			opts->isPressed = FALSE;
			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			RECT rc = closeRectFromBalloon(hWnd);
			RECT rc2 = optionsRectFromBalloon(hWnd);
			RECT rcClient = {0};
			GetClientRect(hWnd, &rcClient);
			if(PtInRect(&rc2, pt) && opts->startedPressOnOptions)
			{
				KillTimer(hWnd, 1);
				KillTimer(hWnd, 2);
				KillTimer(hWnd, 3);
				SetProp(hWnd, L"menuactive", (HANDLE)1);
				HMENU menu = CreatePopupMenu();
				AppendMenu(menu, MF_STRING | MF_ENABLED, 1, L"&GlassToast Options");
				AppendMenu(menu, MF_STRING | MF_ENABLED, 2, L"&Stop GlassToasts");

				RECT rcWin = {0};
				GetWindowRect(hWnd, &rcWin);
				POINT ptMenu = {rcWin.left + rc2.right, rcWin.top + rc2.bottom};
				int cmd = TrackPopupMenu(menu, TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, ptMenu.x, ptMenu.y, 0, hWnd, NULL); 
				switch(cmd)
				{
					case 1:
						ShowWindow((HWND)GetProp(hWnd, L"parent"), SW_SHOW);
					break;

					case 2:
						PostQuitMessage(0);
					break;
				}

				SetProp(hWnd, L"menuactive", (HANDLE)0);
				GetCursorPos(&pt);
				ScreenToClient(hWnd, &pt);
				checkOptionsButton(hWnd, MAKELPARAM(pt.x, pt.y), opts->alpha, msg);
			}
			if(PtInRect(&rc, pt) && opts->startedPressOnClose)
			{
				balloonEvent(hWnd, NIN_BALLOONTIMEOUT);
				DestroyWindow(hWnd);
			}
			else if(!opts->startedPressOnClose && !opts->startedPressOnOptions && PtInRect(&rcClient, pt))
			{
				balloonEvent(hWnd, NIN_BALLOONUSERCLICK);
				DestroyWindow(hWnd);
			}

			
		}
		//ReleaseCapture();
		//SendMessage(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
		break;
	case WM_CREATE:
		SetTimer(hWnd, 1, 10, 0);
			//return OnCreate(hWnd, message, wParam, lParam);
		break;

	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

DWORD CALLBACK TestThread(void* lpVoid)
{

	WCHAR name[MAX_PATH] = {0};
	GetStringValue(L"info", L"author", L"unknown", name, MAX_PATH);
	NOTIFYICONDATA data = {0};
	data.cbSize = NOTIFYICONDATA_V2_SIZE;
	data.uFlags = NIF_INFO;
	data.dwInfoFlags = NIIF_INFO;
	wcscpy_s(data.szInfoTitle, 64, L"Ave's GlassToasts");
	wcscpy_s(data.szInfo, 256, L"Thank you for choosing Ave's Glass Toasts.\nCode by Andreas Verhoeven,\nGraphics by ");
	wcscat_s(data.szInfo, 256, name);
	wcscat_s(data.szInfo, 256, L".");
	Shell_NotifyIcon(NIM_MODIFY, &data);

	ExitThread(0);

	return 0;
}


LRESULT CMainDlg::OnBnClickedShowballoon(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	DWORD id = 0;
	HANDLE h = CreateThread(0, 0, TestThread, 0, 0, &id);
	CloseHandle(h);
	
	return 0;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if(hMod && StopHook)
		StopHook();

	PostQuitMessage(0);

	return 0;
}

LRESULT CMainDlg::OnBnClickedHidedlg(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	
	if(hMod && StopHook)
		StopHook();

	PostQuitMessage(0);

	return 0;
}

LRESULT CMainDlg::OnCbnSelchangeSkins(WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	WCHAR newSkinName[MAX_PATH] = {0};
	CComboBox box = GetDlgItem(IDC_SKINS);
	int sel = box.GetCurSel();
	box.GetLBText(sel, newSkinName);

	WriteSkinName(newSkinName);

	if(balloonHwnd != NULL)
		::DestroyWindow(balloonHwnd);

	DWORD id = 0;
	HANDLE h = CreateThread(0, 0, TestThread, 0, 0, &id);
	CloseHandle(h);
	
	return 0;
}
