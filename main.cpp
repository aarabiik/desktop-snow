#include <windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <gdiplus.h>
#include <chrono>
#include <thread>
#include "config.h"
#include "physics.h"
#include "renderer.h"
#include "settings_gui.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "comctl32.lib")

#define WM_TRAYICON (WM_USER + 101)
#define ID_TRAY_SETTINGS 2001
#define ID_TRAY_CLEAR    2002
#define ID_TRAY_PAUSE    2003
#define ID_TRAY_EXIT     2004

HWND g_hwndOverlay = NULL;
SnowConfig g_config;
PhysicsEngine g_physics;
SnowRenderer g_renderer;
SettingsDialog g_dialog;
NOTIFYICONDATAW g_nid = { sizeof(NOTIFYICONDATAW) };
bool g_running = true;

bool IsFullscreenGameOrApp() {
    // 1. Direct3D Fullscreen check via Windows Shell API (DirectX / Vulkan / OpenGL games)
    QUERY_USER_NOTIFICATION_STATE state;
    if (SUCCEEDED(SHQueryUserNotificationState(&state))) {
        if (state == QUNS_RUNNING_D3D_FULL_SCREEN || state == QUNS_PRESENTATION_MODE) {
            return true;
        }
    }

    // 2. Foreground Window Geometry check (handles Borderless Fullscreen & Windowed-Fullscreen games)
    HWND fg = GetForegroundWindow();
    if (!fg) return false;

    wchar_t cname[128] = {0};
    GetClassNameW(fg, cname, 128);
    if (wcscmp(cname, L"Progman") == 0 || wcscmp(cname, L"WorkerW") == 0 || wcscmp(cname, L"Shell_TrayWnd") == 0) {
        return false;
    }

    HMONITOR hMon = MonitorFromWindow(fg, MONITOR_DEFAULTTONEAREST);
    if (!hMon) return false;

    MONITORINFO mi = { sizeof(MONITORINFO) };
    if (!GetMonitorInfoW(hMon, &mi)) return false;

    RECT r;
    GetWindowRect(fg, &r);

    if (r.left <= mi.rcMonitor.left && r.top <= mi.rcMonitor.top &&
        r.right >= mi.rcMonitor.right && r.bottom >= mi.rcMonitor.bottom) {
        return true;
    }

    return false;
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_TRAYICON) {
        if (lp == WM_LBUTTONDBLCLK || lp == WM_LBUTTONUP) {
            g_dialog.Show();
        } else if (lp == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            InsertMenuW(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_TRAY_SETTINGS, L"⚙️ Настройки и кастомизация...");
            InsertMenuW(hMenu, 1, MF_BYPOSITION | MF_STRING, ID_TRAY_CLEAR, L"🧹 Сбросить сугробы с окон");
            InsertMenuW(hMenu, 2, MF_BYPOSITION | MF_STRING, ID_TRAY_PAUSE, g_config.paused ? L"▶️ Возобновить снег" : L"⏸️ Приостановить снег");
            InsertMenuW(hMenu, 3, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
            InsertMenuW(hMenu, 4, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, L"❌ Выход");

            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        return 0;
    } else if (msg == WM_COMMAND) {
        WORD id = LOWORD(wp);
        if (id == ID_TRAY_SETTINGS) {
            g_dialog.Show();
        } else if (id == ID_TRAY_CLEAR) {
            g_physics.ClearAllDrifts();
        } else if (id == ID_TRAY_PAUSE) {
            g_config.paused = !g_config.paused;
        } else if (id == ID_TRAY_EXIT) {
            g_running = false;
            PostQuitMessage(0);
        }
        return 0;
    } else if (msg == WM_DESTROY) {
        g_running = false;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

HICON CreateSnowflakeIcon() {
    int w = 32, h = 32;
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, w, h);
    HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hBmp);

    HBRUSH bgBrush = CreateSolidBrush(RGB(0, 120, 215));
    RECT rc = {0, 0, w, h};
    FillRect(hdcMem, &rc, bgBrush);
    DeleteObject(bgBrush);

    SetTextColor(hdcMem, RGB(255, 255, 255));
    SetBkMode(hdcMem, TRANSPARENT);
    HFONT hFont = CreateFontW(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH, L"Segoe UI Symbol");
    HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);
    DrawTextW(hdcMem, L"❄", 1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    HBITMAP hMask = CreateCompatibleBitmap(hdcScreen, w, h);
    ICONINFO ii = {0};
    ii.fIcon = TRUE;
    ii.hbmColor = hBmp;
    ii.hbmMask = hMask;
    HICON hIcon = CreateIconIndirect(&ii);

    SelectObject(hdcMem, hOldFont);
    DeleteObject(hFont);
    SelectObject(hdcMem, hOld);
    DeleteObject(hBmp);
    DeleteObject(hMask);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return hIcon;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    HDESK hdesk = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (hdesk) SetThreadDesktop(hdesk);

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    GdiplusStartupInput gdiInput;
    ULONG_PTR gdiToken;
    GdiplusStartup(&gdiToken, &gdiInput, NULL);

    g_config.Load("config.ini");

    int screenX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int screenY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (screenW <= 0) screenW = 1920;
    if (screenH <= 0) screenH = 1080;

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"NativeSnowOverlayClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);

    g_hwndOverlay = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        L"NativeSnowOverlayClass", L"DesktopSnowOverlay",
        WS_POPUP, screenX, screenY, screenW, screenH,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(g_hwndOverlay, SW_SHOWNOACTIVATE);

    HDC hdcScreen = GetDC(NULL);
    g_renderer.Init(hdcScreen, screenW, screenH);
    ReleaseDC(NULL, hdcScreen);

    g_physics.ignoredHwnd = g_hwndOverlay;
    g_physics.Init(screenW, screenH, g_config);

    g_dialog.Create(hInstance, NULL, &g_config);
    g_physics.settingsHwnd = g_dialog.hwnd;
    g_dialog.onConfigChanged = []() {
        g_physics.UpdateFlakeCount(g_config);
    };
    g_dialog.onClearDrifts = []() {
        g_physics.ClearAllDrifts();
    };

    g_nid.hWnd = g_hwndOverlay;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = CreateSnowflakeIcon();
    wcscpy_s(g_nid.szTip, L"❄️ Desktop Snow & Snowdrifts (C++ Native)");
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    g_dialog.Show();

    timeBeginPeriod(1);
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto lastGameCheck = std::chrono::high_resolution_clock::now();
    const double targetFrameMs = 1000.0 / 60.0;

    bool isInGame = false;
    bool wasInGame = false;

    MSG msg;
    while (g_running) {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!g_running) break;

        auto now = std::chrono::high_resolution_clock::now();

        // 🎮 AUTO-GAME DETECTION (Zero CPU when playing games or full screen)
        if (g_config.autoPauseInGames) {
            double gameCheckElapsed = std::chrono::duration<double, std::milli>(now - lastGameCheck).count();
            if (gameCheckElapsed >= 250.0) {
                isInGame = IsFullscreenGameOrApp();
                lastGameCheck = now;
            }

            if (isInGame) {
                if (!wasInGame) {
                    // Wipe overlay cleanly so no ghost snow is visible in borderless games
                    g_renderer.Clear(g_hwndOverlay);
                    wasInGame = true;
                }
                // Deep sleep: LITERALLY 0.00% CPU USAGE
                Sleep(200);
                lastFrameTime = std::chrono::high_resolution_clock::now();
                continue;
            }
        }

        if (wasInGame) {
            wasInGame = false;
            lastFrameTime = now;
        }

        double dt = std::chrono::duration<double>(now - lastFrameTime).count();
        lastFrameTime = now;

        // Step physics and render
        g_physics.Update((float)dt, g_config);
        g_renderer.Render(g_hwndOverlay, g_physics, g_config);

        auto frameEnd = std::chrono::high_resolution_clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(frameEnd - now).count();
        if (elapsedMs < targetFrameMs) {
            DWORD sleepMs = (DWORD)(targetFrameMs - elapsedMs);
            if (sleepMs > 0) Sleep(sleepMs);
        }
    }

    timeEndPeriod(1);
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    if (g_nid.hIcon) DestroyIcon(g_nid.hIcon);
    g_renderer.Cleanup();
    GdiplusShutdown(gdiToken);
    return 0;
}
