#pragma once

class TrayIconFinder
{
private:
	TrayIconFinder(void);
	~TrayIconFinder(void);

	static BOOL FindOutPositionOfIconDirectly(const HWND a_hWndOwner, const int a_iButtonID, RECT& a_rcIcon);
	static BOOL CALLBACK FindTrayWnd(HWND hwnd, LPARAM lParam);
	static BOOL CALLBACK FindToolBarInTrayWnd(HWND hwnd, LPARAM lParam);
	static HWND GetTrayNotifyWnd(BOOL a_bSeekForEmbedToolbar);
	static RECT GetTrayWndRect();

public:
	static POINT GetTrayIconPosition(HWND hwnd, UINT id);
};
