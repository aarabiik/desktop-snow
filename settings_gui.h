#pragma once
#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <string>
#include "config.h"

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#define IDC_SLIDER_COUNT       1001
#define IDC_SLIDER_SIZE        1002
#define IDC_SLIDER_GRAVITY     1003
#define IDC_SLIDER_WIND        1004
#define IDC_SLIDER_GROWTH      1005
#define IDC_SLIDER_MAX_HEIGHT  1006
#define IDC_SLIDER_MELT        1007
#define IDC_COMBO_STYLE        1008
#define IDC_COMBO_COLOR        1009
#define IDC_BTN_PRESET_TALE    1010
#define IDC_BTN_PRESET_BLIZZ   1011
#define IDC_BTN_PRESET_GLOW    1012
#define IDC_BTN_PRESET_NEON    1013
#define IDC_BTN_PRESET_ECO     1014
#define IDC_CHK_GAME_MODE      1015
#define IDC_BTN_CLEAR_DRIFTS   1020
#define IDC_BTN_SAVE           1021
#define IDC_BTN_WINDHAWK       1022

class SettingsDialog {
public:
    HWND hwnd = NULL;
    SnowConfig* pConfig = NULL;
    void (*onConfigChanged)() = NULL;
    void (*onClearDrifts)() = NULL;
    HBRUSH hDarkBrush = NULL;
    HFONT hFontUI = NULL;
    HFONT hFontHeader = NULL;

    HWND lblCount = NULL, sldCount = NULL;
    HWND lblSize = NULL, sldSize = NULL;
    HWND lblGrowth = NULL, sldGrowth = NULL;
    HWND lblMaxH = NULL, sldMaxH = NULL;
    HWND lblMelt = NULL, sldMelt = NULL;
    HWND lblGravity = NULL, sldGravity = NULL;
    HWND lblWind = NULL, sldWind = NULL;
    HWND cbStyle = NULL;
    HWND cbColor = NULL;
    HWND chkGameMode = NULL;

    void Create(HINSTANCE hInstance, HWND parent, SnowConfig* cfg) {
        pConfig = cfg;
        hDarkBrush = CreateSolidBrush(RGB(28, 28, 28));
        hFontUI = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                              DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        hFontHeader = CreateFontW(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                  OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                  DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        wc.lpfnWndProc = DialogProcThunk;
        wc.hInstance = hInstance;
        wc.lpszClassName = L"SnowSettingsDialogClass";
        wc.hbrBackground = hDarkBrush;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassExW(&wc);

        hwnd = CreateWindowExW(
            WS_EX_APPWINDOW, L"SnowSettingsDialogClass",
            L"❄️ Desktop Snow & Snowdrifts (C++ Native) - Кастомизация",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, 570, 750,
            parent, NULL, hInstance, this
        );

        BOOL dark = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

        InitControls(hInstance);
        UpdateControlsFromConfig();
    }

    void Show() {
        if (hwnd) {
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
        }
    }

    void Hide() {
        if (hwnd) ShowWindow(hwnd, SW_HIDE);
    }

private:
    static LRESULT CALLBACK DialogProcThunk(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
        SettingsDialog* self = NULL;
        if (msg == WM_NCCREATE) {
            auto cs = (CREATESTRUCT*)lp;
            self = (SettingsDialog*)cs->lpCreateParams;
            SetWindowLongPtr(h, GWLP_USERDATA, (LONG_PTR)self);
            self->hwnd = h;
        } else {
            self = (SettingsDialog*)GetWindowLongPtr(h, GWLP_USERDATA);
        }
        if (self) return self->DialogProc(h, msg, wp, lp);
        return DefWindowProcW(h, msg, wp, lp);
    }

    LRESULT DialogProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_CTLCOLORSTATIC: {
                HDC hdc = (HDC)wp;
                SetTextColor(hdc, RGB(235, 235, 235));
                SetBkColor(hdc, RGB(28, 28, 28));
                return (LRESULT)hDarkBrush;
            }
            case WM_HSCROLL: {
                OnSliderScroll();
                return 0;
            }
            case WM_COMMAND: {
                WORD id = LOWORD(wp);
                WORD code = HIWORD(wp);
                if (code == CBN_SELCHANGE) {
                    if (id == IDC_COMBO_STYLE) {
                        pConfig->style = (int)SendMessageW(cbStyle, CB_GETCURSEL, 0, 0);
                        if (onConfigChanged) onConfigChanged();
                    } else if (id == IDC_COMBO_COLOR) {
                        int sel = (int)SendMessageW(cbColor, CB_GETCURSEL, 0, 0);
                        pConfig->SetColorPreset(sel);
                        if (onConfigChanged) onConfigChanged();
                    }
                } else if (id == IDC_CHK_GAME_MODE) {
                    pConfig->autoPauseInGames = (SendMessageW(chkGameMode, BM_GETCHECK, 0, 0) == BST_CHECKED);
                } else if (id >= IDC_BTN_PRESET_TALE && id <= IDC_BTN_PRESET_ECO) {
                    pConfig->ApplyPreset(id - IDC_BTN_PRESET_TALE);
                    UpdateControlsFromConfig();
                    if (onConfigChanged) onConfigChanged();
                } else if (id == IDC_BTN_CLEAR_DRIFTS) {
                    if (onClearDrifts) onClearDrifts();
                } else if (id == IDC_BTN_SAVE) {
                    pConfig->Save("config.ini");
                    MessageBoxW(hwnd, L"Настройки сохранены в config.ini!", L"Сохранено", MB_OK | MB_ICONINFORMATION);
                } else if (id == IDC_BTN_WINDHAWK) {
                    ShellExecuteW(NULL, L"open", L"https://windhawk.net/mods/windows-11-corners", NULL, NULL, SW_SHOWNORMAL);
                }
                return 0;
            }
            case WM_CLOSE: {
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }
            case WM_DESTROY: {
                if (hDarkBrush) DeleteObject(hDarkBrush);
                if (hFontUI) DeleteObject(hFontUI);
                if (hFontHeader) DeleteObject(hFontHeader);
                break;
            }
        }
        return DefWindowProcW(h, msg, wp, lp);
    }

    void InitControls(HINSTANCE hInst) {
        int y = 14;

        HWND hHeader = CreateWindowW(L"STATIC", L"🏔️ Нативные сугробы на окнах и физический снег",
            WS_CHILD | WS_VISIBLE, 18, y, 510, 24, hwnd, NULL, hInst, NULL);
        SendMessageW(hHeader, WM_SETFONT, (WPARAM)hFontHeader, TRUE);
        y += 28;

        HWND hSub = CreateWindowW(L"STATIC", L"Сугробы на окнах • Авто-пауза в играх (0.00% ЦП) • Без ссыпания",
            WS_CHILD | WS_VISIBLE, 18, y, 510, 20, hwnd, NULL, hInst, NULL);
        SendMessageW(hSub, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        y += 28;

        // Presets
        CreateWindowW(L"STATIC", L"Быстрые пресеты:", WS_CHILD | WS_VISIBLE, 18, y, 120, 20, hwnd, NULL, hInst, NULL);
        y += 22;

        HWND b1 = CreateWindowW(L"BUTTON", L"🏔️ Сказка (Сугробы)", WS_CHILD | WS_VISIBLE, 18, y, 134, 28, hwnd, (HMENU)IDC_BTN_PRESET_TALE, hInst, NULL);
        HWND b2 = CreateWindowW(L"BUTTON", L"❄️ Метель", WS_CHILD | WS_VISIBLE, 158, y, 86, 28, hwnd, (HMENU)IDC_BTN_PRESET_BLIZZ, hInst, NULL);
        HWND b3 = CreateWindowW(L"BUTTON", L"🌟 Свечение", WS_CHILD | WS_VISIBLE, 250, y, 92, 28, hwnd, (HMENU)IDC_BTN_PRESET_GLOW, hInst, NULL);
        HWND b4 = CreateWindowW(L"BUTTON", L"🌈 Неон", WS_CHILD | WS_VISIBLE, 348, y, 80, 28, hwnd, (HMENU)IDC_BTN_PRESET_NEON, hInst, NULL);
        HWND b5 = CreateWindowW(L"BUTTON", L"⚡ Эко (0.1% ЦП)", WS_CHILD | WS_VISIBLE, 434, y, 100, 28, hwnd, (HMENU)IDC_BTN_PRESET_ECO, hInst, NULL);

        SendMessageW(b1, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(b2, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(b3, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(b4, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(b5, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        y += 36;

        // Game Mode Checkbox
        chkGameMode = CreateWindowW(L"BUTTON", L"🎮 Авто-пауза в играх и полноэкранных приложениях (0.00% ЦП)",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 18, y, 510, 24, hwnd, (HMENU)IDC_CHK_GAME_MODE, hInst, NULL);
        SendMessageW(chkGameMode, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(chkGameMode, BM_SETCHECK, BST_CHECKED, 0);
        y += 30;

        // Drift Growth
        lblGrowth = CreateWindowW(L"STATIC", L"Скорость роста сугробов на окнах:", WS_CHILD | WS_VISIBLE, 18, y, 510, 18, hwnd, NULL, hInst, NULL);
        y += 20;
        sldGrowth = CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 18, y, 510, 26, hwnd, (HMENU)IDC_SLIDER_GROWTH, hInst, NULL);
        SendMessageW(sldGrowth, TBM_SETRANGE, TRUE, MAKELPARAM(0, 30));
        y += 32;

        // Max Drift Height
        lblMaxH = CreateWindowW(L"STATIC", L"Максимальная высота сугробов на окнах:", WS_CHILD | WS_VISIBLE, 18, y, 510, 18, hwnd, NULL, hInst, NULL);
        y += 20;
        sldMaxH = CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 18, y, 510, 26, hwnd, (HMENU)IDC_SLIDER_MAX_HEIGHT, hInst, NULL);
        SendMessageW(sldMaxH, TBM_SETRANGE, TRUE, MAKELPARAM(8, 60));
        y += 32;

        // Melt Speed
        lblMelt = CreateWindowW(L"STATIC", L"Скорость таяния сугробов:", WS_CHILD | WS_VISIBLE, 18, y, 510, 18, hwnd, NULL, hInst, NULL);
        y += 20;
        sldMelt = CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 18, y, 510, 26, hwnd, (HMENU)IDC_SLIDER_MELT, hInst, NULL);
        SendMessageW(sldMelt, TBM_SETRANGE, TRUE, MAKELPARAM(0, 30));
        y += 32;

        // Flake Count
        lblCount = CreateWindowW(L"STATIC", L"Количество снежинок:", WS_CHILD | WS_VISIBLE, 18, y, 510, 18, hwnd, NULL, hInst, NULL);
        y += 20;
        sldCount = CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 18, y, 510, 26, hwnd, (HMENU)IDC_SLIDER_COUNT, hInst, NULL);
        SendMessageW(sldCount, TBM_SETRANGE, TRUE, MAKELPARAM(30, 800));
        y += 32;

        // Flake Size
        lblSize = CreateWindowW(L"STATIC", L"Размер снежинок (от мелкой крупки до хлопьев):", WS_CHILD | WS_VISIBLE, 18, y, 510, 18, hwnd, NULL, hInst, NULL);
        y += 20;
        sldSize = CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 18, y, 510, 26, hwnd, (HMENU)IDC_SLIDER_SIZE, hInst, NULL);
        SendMessageW(sldSize, TBM_SETRANGE, TRUE, MAKELPARAM(15, 75));
        y += 32;

        // Gravity / Speed
        lblGravity = CreateWindowW(L"STATIC", L"Скорость падения снега:", WS_CHILD | WS_VISIBLE, 18, y, 510, 18, hwnd, NULL, hInst, NULL);
        y += 20;
        sldGravity = CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 18, y, 510, 26, hwnd, (HMENU)IDC_SLIDER_GRAVITY, hInst, NULL);
        SendMessageW(sldGravity, TBM_SETRANGE, TRUE, MAKELPARAM(40, 260));
        y += 32;

        // Wind
        lblWind = CreateWindowW(L"STATIC", L"Ветер (<- влево / вправо ->):", WS_CHILD | WS_VISIBLE, 18, y, 510, 18, hwnd, NULL, hInst, NULL);
        y += 20;
        sldWind = CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ, 18, y, 510, 26, hwnd, (HMENU)IDC_SLIDER_WIND, hInst, NULL);
        SendMessageW(sldWind, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
        y += 32;

        // Style Row
        CreateWindowW(L"STATIC", L"Стиль снежинок:", WS_CHILD | WS_VISIBLE, 18, y, 130, 20, hwnd, NULL, hInst, NULL);
        cbStyle = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 150, y - 2, 378, 160, hwnd, (HMENU)IDC_COMBO_STYLE, hInst, NULL);
        SendMessageW(cbStyle, CB_ADDSTRING, 0, (LPARAM)L"Пушистые мягкие хлопья (Fluffy Snow)");
        SendMessageW(cbStyle, CB_ADDSTRING, 0, (LPARAM)L"Зимнее свечение (Soft Glow)");
        SendMessageW(cbStyle, CB_ADDSTRING, 0, (LPARAM)L"Кристаллы и звёздочки (❄ Crystals)");
        SendMessageW(cbStyle, CB_ADDSTRING, 0, (LPARAM)L"Киберпанк Неон (Cyber Neon)");
        SendMessageW(cbStyle, CB_ADDSTRING, 0, (LPARAM)L"Минималистичные точки (Dots)");
        SendMessageW(cbStyle, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        y += 30;

        // Color Row
        CreateWindowW(L"STATIC", L"Цветовая гамма:", WS_CHILD | WS_VISIBLE, 18, y, 130, 20, hwnd, NULL, hInst, NULL);
        cbColor = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 150, y - 2, 378, 160, hwnd, (HMENU)IDC_COMBO_COLOR, hInst, NULL);
        SendMessageW(cbColor, CB_ADDSTRING, 0, (LPARAM)L"Белый (Классический чистый снег)");
        SendMessageW(cbColor, CB_ADDSTRING, 0, (LPARAM)L"Ледяной синий (Cool Ice Blue)");
        SendMessageW(cbColor, CB_ADDSTRING, 0, (LPARAM)L"Золотистый праздничный (Golden Warm)");
        SendMessageW(cbColor, CB_ADDSTRING, 0, (LPARAM)L"Неоновый Циан (Cyber Cyan)");
        SendMessageW(cbColor, CB_ADDSTRING, 0, (LPARAM)L"Неоновый Розовый (Neon Pink)");
        SendMessageW(cbColor, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        y += 36;

        // Action Buttons Row
        HWND btnClear = CreateWindowW(L"BUTTON", L"🧹 Сбросить сугробы", WS_CHILD | WS_VISIBLE, 18, y, 164, 32, hwnd, (HMENU)IDC_BTN_CLEAR_DRIFTS, hInst, NULL);
        HWND btnSave = CreateWindowW(L"BUTTON", L"💾 Сохранить настройки", WS_CHILD | WS_VISIBLE, 190, y, 164, 32, hwnd, (HMENU)IDC_BTN_SAVE, hInst, NULL);
        HWND btnWindhawk = CreateWindowW(L"BUTTON", L"🪟 Скругление окон", WS_CHILD | WS_VISIBLE, 362, y, 164, 32, hwnd, (HMENU)IDC_BTN_WINDHAWK, hInst, NULL);

        SendMessageW(btnClear, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(btnSave, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(btnWindhawk, WM_SETFONT, (WPARAM)hFontUI, TRUE);

        SendMessageW(lblGrowth, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(lblMaxH, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(lblMelt, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(lblCount, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(lblSize, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(lblGravity, WM_SETFONT, (WPARAM)hFontUI, TRUE);
        SendMessageW(lblWind, WM_SETFONT, (WPARAM)hFontUI, TRUE);
    }

    void UpdateControlsFromConfig() {
        if (!pConfig) return;
        SendMessageW(chkGameMode, BM_SETCHECK, pConfig->autoPauseInGames ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(sldGrowth, TBM_SETPOS, TRUE, (LPARAM)(pConfig->driftGrowthRate * 10.0f));
        SendMessageW(sldMaxH, TBM_SETPOS, TRUE, (LPARAM)pConfig->maxDriftHeight);
        SendMessageW(sldMelt, TBM_SETPOS, TRUE, (LPARAM)(pConfig->meltSpeed * 10.0f));
        SendMessageW(sldCount, TBM_SETPOS, TRUE, (LPARAM)pConfig->flakeCount);
        SendMessageW(sldSize, TBM_SETPOS, TRUE, (LPARAM)(pConfig->flakeSize * 10.0f));
        SendMessageW(sldGravity, TBM_SETPOS, TRUE, (LPARAM)pConfig->gravity);
        SendMessageW(sldWind, TBM_SETPOS, TRUE, (LPARAM)(pConfig->wind + 50.0f));
        SendMessageW(cbStyle, CB_SETCURSEL, pConfig->style, 0);
        SendMessageW(cbColor, CB_SETCURSEL, pConfig->colorPreset, 0);

        UpdateLabels();
    }

    void OnSliderScroll() {
        pConfig->driftGrowthRate = (float)SendMessageW(sldGrowth, TBM_GETPOS, 0, 0) / 10.0f;
        pConfig->maxDriftHeight = (float)SendMessageW(sldMaxH, TBM_GETPOS, 0, 0);
        pConfig->meltSpeed = (float)SendMessageW(sldMelt, TBM_GETPOS, 0, 0) / 10.0f;
        pConfig->flakeCount = (int)SendMessageW(sldCount, TBM_GETPOS, 0, 0);
        pConfig->flakeSize = (float)SendMessageW(sldSize, TBM_GETPOS, 0, 0) / 10.0f;
        pConfig->gravity = (float)SendMessageW(sldGravity, TBM_GETPOS, 0, 0);
        pConfig->wind = (float)SendMessageW(sldWind, TBM_GETPOS, 0, 0) - 50.0f;

        UpdateLabels();
        if (onConfigChanged) onConfigChanged();
    }

    void UpdateLabels() {
        wchar_t buf[128];
        swprintf_s(buf, L"Скорость роста сугробов на окнах: %.1fx", pConfig->driftGrowthRate);
        SetWindowTextW(lblGrowth, buf);

        swprintf_s(buf, L"Максимальная высота сугробов на окнах: %d px", (int)pConfig->maxDriftHeight);
        SetWindowTextW(lblMaxH, buf);

        swprintf_s(buf, L"Скорость таяния сугробов: %.1f", pConfig->meltSpeed);
        SetWindowTextW(lblMelt, buf);

        swprintf_s(buf, L"Количество снежинок: %d", pConfig->flakeCount);
        SetWindowTextW(lblCount, buf);

        swprintf_s(buf, L"Размер снежинок: %.1f px", pConfig->flakeSize);
        SetWindowTextW(lblSize, buf);

        swprintf_s(buf, L"Скорость падения снега: %d", (int)pConfig->gravity);
        SetWindowTextW(lblGravity, buf);

        swprintf_s(buf, L"Ветер: %d px/s (<- влево / вправо ->)", (int)pConfig->wind);
        SetWindowTextW(lblWind, buf);
    }
};
