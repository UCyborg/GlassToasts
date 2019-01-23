#pragma once

enum { WM_APP_BEGIN = WM_APP, WM_APP_BYE, WM_TRAYCALLBACK, WM_TRAYCRASHED };

#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllexport) BOOL CALLBACK StartHook(HWND);
__declspec(dllexport) void CALLBACK StopHook();
#ifdef __cplusplus
}
#endif
