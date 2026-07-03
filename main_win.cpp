// Windows Win32 GUI for Perpetual Calendar
// Cross-compiled with MinGW64 - single binary (static link)
#define WIN32_LEAN_AND_MEAN
#define _WIN32_IE 0x0500
#include <windows.h>
#include <commctrl.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <map>

#include "calendar_core.h"

// GET_X_LPARAM / GET_Y_LPARAM macros
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

CalendarApp g_app;
HWND g_hwnd = NULL;
HWND g_hwndYearLabel = NULL;
HWND g_hwndStatus = NULL;
HWND g_hwndEditDlg = NULL;
HWND g_hwndEditInput = NULL;
int g_editSerial = 0;

// Forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void RefreshCalendar();
void ShowEditDialog(int serial, const wchar_t* current_text);
void CenterWindow(HWND hwndChild, HWND hwndParent);

// Korean day names (UTF-8)
static const char* DOW_SHORT[7] = {"\xEC\x9D\xBC", "\xEC\x9B\x94", "\xED\x99\x94",
                                    "\xEC\x88\x98", "\xEB\xAA\xA9", "\xEA\xB8\x88",
                                    "\xED\x86\xA0"};

static std::string month_name_korean(int m) {
    static const char* names[12] = {"1\xEC\x9B\x94","2\xEC\x9B\x94","3\xEC\x9B\x94",
                                    "4\xEC\x9B\x94","5\xEC\x9B\x94","6\xEC\x9B\x94",
                                    "7\xEC\x9B\x94","8\xEC\x9B\x94","9\xEC\x9B\x94",
                                    "10\xEC\x9B\x94","11\xEC\x9B\x94","12\xEC\x9B\x94"};
    return names[m-1];
}

static std::wstring u2w(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], len);
    if (!w.empty() && w.back() == '\0') w.pop_back();
    return w;
}

static std::string w2u(const wchar_t* s) {
    if (!s || !*s) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
    std::string u(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s, -1, &u[0], len, NULL, NULL);
    if (!u.empty() && u.back() == '\0') u.pop_back();
    return u;
}

HBRUSH g_brWhite, g_brSunday, g_brSaturday, g_brHoliday;
HFONT g_fontBold, g_fontNormal, g_fontSmall, g_fontLarge, g_fontDOW;

void InitGDI() {
    g_brWhite = CreateSolidBrush(RGB(255,255,255));
    g_brSunday = CreateSolidBrush(RGB(255,238,238));
    g_brSaturday = CreateSolidBrush(RGB(238,238,255));
    g_brHoliday = CreateSolidBrush(RGB(255,224,224));
    LOGFONTW lf;
    memset(&lf, 0, sizeof(lf));
    lf.lfHeight = 14; lf.lfWeight = FW_BOLD; wcscpy(lf.lfFaceName, L"Malgun Gothic");
    g_fontBold = CreateFontIndirectW(&lf);
    lf.lfHeight = 13; lf.lfWeight = FW_NORMAL;
    lf.lfFaceName[0] = 0; wcscpy(lf.lfFaceName, L"Malgun Gothic");
    g_fontNormal = CreateFontIndirectW(&lf);
    lf.lfHeight = 11; lf.lfWeight = FW_NORMAL;
    g_fontSmall = CreateFontIndirectW(&lf);
    lf.lfHeight = 22; lf.lfWeight = FW_BOLD;
    g_fontLarge = CreateFontIndirectW(&lf);
    lf.lfHeight = 11; lf.lfWeight = FW_BOLD;
    g_fontDOW = CreateFontIndirectW(&lf);
}

void CleanupGDI() {
    if (g_brWhite) DeleteObject(g_brWhite);
    if (g_brSunday) DeleteObject(g_brSunday);
    if (g_brSaturday) DeleteObject(g_brSaturday);
    if (g_brHoliday) DeleteObject(g_brHoliday);
    if (g_fontBold) DeleteObject(g_fontBold);
    if (g_fontNormal) DeleteObject(g_fontNormal);
    if (g_fontSmall) DeleteObject(g_fontSmall);
    if (g_fontLarge) DeleteObject(g_fontLarge);
    if (g_fontDOW) DeleteObject(g_fontDOW);
}

LRESULT CALLBACK EditDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hwndEditDlg = hwnd;
        RECT rc; GetClientRect(hwnd, &rc);
        g_hwndEditInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            10, 10, rc.right - 20, 28, hwnd, NULL, GetModuleHandleW(NULL), NULL);
        SendMessageW(g_hwndEditInput, WM_SETFONT, (WPARAM)g_fontNormal, TRUE);
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        if (cs && cs->lpCreateParams) {
            SetWindowTextW(g_hwndEditInput, (LPCWSTR)cs->lpCreateParams);
        }
        CreateWindowExW(0, L"BUTTON", L"\uC800\uC7A5",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            rc.right - 180, 50, 80, 30, hwnd, (HMENU)IDOK, GetModuleHandleW(NULL), NULL);
        CreateWindowExW(0, L"BUTTON", L"\uCDE8\uC18C",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            rc.right - 90, 50, 80, 30, hwnd, (HMENU)IDCANCEL, GetModuleHandleW(NULL), NULL);
        SetFocus(g_hwndEditInput);
        return 0;
    }
    case WM_SIZE: {
        RECT rc; GetClientRect(hwnd, &rc);
        if (g_hwndEditInput)
            SetWindowPos(g_hwndEditInput, NULL, 10, 10, rc.right - 20, 28, SWP_NOZORDER);
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == IDOK) {
            wchar_t buf[1024] = {0};
            GetWindowTextW(g_hwndEditInput, buf, 1024);
            std::string text = w2u(buf);
            g_app.set_schedule(g_editSerial, text.c_str());
            RefreshCalendar();
            DestroyWindow(hwnd);
        } else if (LOWORD(wParam) == IDCANCEL) {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        g_hwndEditDlg = NULL;
        g_hwndEditInput = NULL;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowEditDialog(int serial, const wchar_t* current_text) {
    if (g_hwndEditDlg) { SetForegroundWindow(g_hwndEditDlg); return; }
    g_editSerial = serial;
    auto day = serial_to_date(serial);
    wchar_t title[128];
    swprintf(title, 128, L"%d\uB144 %d\uC6D4 %d\uC77C \uC77C\uC815 \uC218\uC815",
             day.tm_year + 1900, day.tm_mon + 1, day.tm_mday);
    HWND hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"EditDialog", title,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 420, 130,
        g_hwnd, NULL, GetModuleHandleW(NULL), (LPVOID)current_text);
    if (hwnd) CenterWindow(hwnd, g_hwnd);
}

void CenterWindow(HWND hwndChild, HWND hwndParent) {
    RECT rcChild, rcParent;
    GetWindowRect(hwndChild, &rcChild);
    GetWindowRect(hwndParent, &rcParent);
    int w = rcChild.right - rcChild.left;
    int h = rcChild.bottom - rcChild.top;
    int cx = (rcParent.left + rcParent.right) / 2 - w / 2;
    int cy = (rcParent.top + rcParent.bottom) / 2 - h / 2;
    SetWindowPos(hwndChild, NULL, cx, cy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void DrawDayCell(HDC hdc, const CalendarDay& day, int x, int y, int w, int h) {
    HBRUSH bg = g_brWhite;
    if (day.is_holiday) bg = g_brHoliday;
    else if (day.dow == 0) bg = g_brSunday;
    else if (day.dow == 6) bg = g_brSaturday;
    RECT rc = {x, y, x + w, y + h};
    FillRect(hdc, &rc, bg);
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(220,220,220));
    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    SetBkMode(hdc, TRANSPARENT);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d %s", day.day, day.dow_str.c_str());
    std::wstring wbuf = u2w(buf);
    SelectObject(hdc, g_fontBold);
    if (day.dow == 0) SetTextColor(hdc, RGB(204,0,0));
    else if (day.dow == 6) SetTextColor(hdc, RGB(0,0,204));
    else SetTextColor(hdc, RGB(0,0,0));
    RECT tr = {x + 2, y + 1, x + w, y + 18};
    DrawTextW(hdc, wbuf.c_str(), -1, &tr, DT_LEFT | DT_TOP | DT_NOPREFIX);
    std::string display;
    if (day.is_holiday) display = day.holiday_name;
    else if (!day.schedule.empty()) display = day.schedule;
    if (!display.empty()) {
        std::wstring wdisp = u2w(display);
        SelectObject(hdc, g_fontSmall);
        if (day.is_holiday) SetTextColor(hdc, RGB(204,0,0));
        else SetTextColor(hdc, RGB(0,102,0));
        RECT sr = {x + 2, y + 16, x + w - 2, y + h};
        DrawTextW(hdc, wdisp.c_str(), -1, &sr, DT_LEFT | DT_TOP | DT_NOPREFIX | DT_END_ELLIPSIS);
    }
}

void RefreshCalendar() {
    if (g_hwnd) {
        // Update year label
        int year = g_app.get_year();
        int start = g_app.get_start_month();
        char ys[64];
        snprintf(ys, sizeof(ys), "%d\xEB\x85\x84 (%s)", year,
                 start == 1 ? "\xEC\x83\x81\xEB\xB0\x98\xEA\xB8\xB0" : "\xED\x95\x98\xEB\xB0\x98\xEA\xB8\xB0");
        std::wstring wys = u2w(ys);
        SetWindowTextW(g_hwndYearLabel, wys.c_str());
        InvalidateRect(g_hwnd, NULL, TRUE);
        UpdateWindow(g_hwnd);
    }
}

void DrawCalendar(HDC hdc, RECT* rc) {
    int year = g_app.get_year();
    int start = g_app.get_start_month();
    char ys[64];
    snprintf(ys, sizeof(ys), "%d\xEB\x85\x84 (%s)", year,
             start == 1 ? "\xEC\x83\x81\xEB\xB0\x98\xEA\xB8\xB0" : "\xED\x95\x98\xEB\xB0\x98\xEA\xB8\xB0");
    std::wstring wys = u2w(ys);
    SelectObject(hdc, g_fontLarge);
    SetTextColor(hdc, RGB(0,0,0));
    SetBkMode(hdc, TRANSPARENT);
    RECT yr = {rc->left, rc->top + 5, rc->right, rc->top + 35};
    DrawTextW(hdc, wys.c_str(), -1, &yr, DT_CENTER | DT_TOP | DT_NOPREFIX);

    int calTop = rc->top + 40;
    int calLeft = rc->left + 5;
    int calRight = rc->right - 5;
    int calBottom = rc->bottom - 5;
    int calW = calRight - calLeft;
    int calH = calBottom - calTop;
    int cols = 3, rows = 2, margin = 3;
    int cellW = (calW - margin * (cols - 1)) / cols;
    int cellH = (calH - margin * (rows - 1)) / rows;
    auto& data = g_app.get_calendar_data();

    for (int mi = 0; mi < 6 && mi < (int)data.size(); mi++) {
        auto& days = data[mi];
        if (days.empty()) continue;
        int gr = mi / 3, gc = mi % 3;
        int mx = calLeft + gc * (cellW + margin);
        int my = calTop + gr * (cellH + margin);
        int m = days[0].month;
        int y = days[0].year;
        char mh[64];
        snprintf(mh, sizeof(mh), "%d\xEB\x85\x84 %s", y, month_name_korean(m).c_str());
        std::wstring wmh = u2w(mh);
        SelectObject(hdc, g_fontBold);
        SetTextColor(hdc, RGB(0,0,0));
        RECT mr = {mx + 5, my + 2, mx + cellW - 5, my + 22};
        DrawTextW(hdc, wmh.c_str(), -1, &mr, DT_LEFT | DT_TOP | DT_NOPREFIX);

        int dowY = my + 24;
        int dw = cellW / 7;
        for (int di = 0; di < 7; di++) {
            std::wstring wd = u2w(DOW_SHORT[di]);
            SelectObject(hdc, g_fontDOW);
            if (di == 0) SetTextColor(hdc, RGB(204,0,0));
            else if (di == 6) SetTextColor(hdc, RGB(0,0,204));
            else SetTextColor(hdc, RGB(102,102,102));
            RECT dr = {mx + di * dw, dowY, mx + (di+1) * dw, dowY + 18};
            DrawTextW(hdc, wd.c_str(), -1, &dr, DT_CENTER | DT_TOP | DT_NOPREFIX);
        }

        int first_dow = days[0].dow;
        int dayCells = first_dow + (int)days.size();
        int weeks = (dayCells + 6) / 7;
        int dh = (cellH - 48) / weeks;
        if (dh < 18) dh = 18;
        int idx = first_dow;
        for (size_t di = 0; di < days.size() && idx < 42; di++) {
            int dr = idx / 7, dc = idx % 7;
            int dx = mx + dc * dw;
            int dy = dowY + 18 + dr * dh + 1;
            DrawDayCell(hdc, days[di], dx, dy, dw - 1, dh - 1);
            idx++;
        }
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hwnd = hwnd;
        g_app.load_schedule("schedule.csv");
        g_hwndStatus = CreateWindowExW(0, STATUSCLASSNAMEW, NULL,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0, hwnd, NULL, GetModuleHandleW(NULL), NULL);
        std::wstring st = L"\uC77C\uC815\uC740 \uB0A0\uC9DC\uB97C \uB354\uBE14\uD074\uB9AD\uD558\uC5EC \uCD94\uAC00/\uC218\uC815";
        SetWindowTextW(g_hwndStatus, st.c_str());
        return 0;
    }
    case WM_SIZE: {
        if (g_hwndStatus) SendMessageW(g_hwndStatus, WM_SIZE, 0, 0);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc; GetClientRect(hwnd, &rc);
        rc.top += 40;
        RECT sbr; memset(&sbr, 0, sizeof(sbr));
        if (g_hwndStatus) { GetWindowRect(g_hwndStatus, &sbr); rc.bottom -= (sbr.bottom - sbr.top); }
        DrawCalendar(hdc, &rc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_LBUTTONDBLCLK: {
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        RECT rc; GetClientRect(hwnd, &rc);
        rc.top += 40;
        RECT sbr; memset(&sbr, 0, sizeof(sbr));
        if (g_hwndStatus) { GetWindowRect(g_hwndStatus, &sbr); rc.bottom -= (sbr.bottom - sbr.top); }
        int calTop = rc.top + 40, calLeft = rc.left + 5;
        int calW = (rc.right - rc.left) - 10, calH = (rc.bottom - rc.top) - 45;
        int cols = 3, rows = 2, margin = 3;
        int cellW = (calW - margin * (cols - 1)) / cols;
        int cellH = (calH - margin * (rows - 1)) / rows;
        auto& data = g_app.get_calendar_data();
        for (int mi = 0; mi < 6 && mi < (int)data.size(); mi++) {
            auto& days = data[mi];
            if (days.empty()) continue;
            int gr = mi / 3, gc = mi % 3;
            int mx = calLeft + gc * (cellW + margin);
            int my = calTop + gr * (cellH + margin);
            int dw = cellW / 7, dowY = my + 24;
            int dayCells = days[0].dow + (int)days.size();
            int weeks = (dayCells + 6) / 7;
            int dh = (cellH - 48) / weeks;
            if (dh < 18) dh = 18;
            int idx = days[0].dow;
            for (size_t di = 0; di < days.size(); di++) {
                int dr = idx / 7, dc = idx % 7;
                int dx = mx + dc * dw, dy = dowY + 18 + dr * dh + 1;
                if (pt.x >= dx && pt.x < dx + dw - 1 && pt.y >= dy && pt.y < dy + dh - 1) {
                    std::string text = g_app.get_schedule(days[di].serial);
                    std::wstring wtext = u2w(text);
                    ShowEditDialog(days[di].serial, wtext.c_str());
                    return 0;
                }
                idx++;
            }
        }
        return 0;
    }
    case WM_COMMAND: {
        if (HIWORD(wParam) == BN_CLICKED) {
            switch (LOWORD(wParam)) {
            case 1001: g_app.move_prev_quarter(); RefreshCalendar(); break;
            case 1002: g_app.move_next_quarter(); RefreshCalendar(); break;
            }
        }
        return 0;
    }
    case WM_DESTROY:
        g_app.save_schedule("schedule.csv");
        CleanupGDI();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    InitCommonControls();
    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"PerpetualCalendar";
    RegisterClassExW(&wc);

    WNDCLASSEXW ec;
    memset(&ec, 0, sizeof(ec));
    ec.cbSize = sizeof(WNDCLASSEXW);
    ec.style = CS_HREDRAW | CS_VREDRAW;
    ec.lpfnWndProc = EditDlgProc;
    ec.hInstance = hInst;
    ec.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    ec.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    ec.lpszClassName = L"EditDialog";
    RegisterClassExW(&ec);

    // Initialize GDI before creating window controls
    InitGDI();
    
    HWND hwnd = CreateWindowExW(0, L"PerpetualCalendar",
        L"\uB9CC\uB144 \uB2EC\uB825 - Perpetual Calendar",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 1350, 820,
        NULL, NULL, hInst, NULL);
    if (!hwnd) return 1;

    // Toolbar buttons
    CreateWindowExW(0, L"BUTTON", L"\u25C0 \uC774\uC804 \uBD84\uAE30",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 5, 120, 30, hwnd, (HMENU)1001, hInst, NULL);
    
    g_hwndYearLabel = CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        140, 5, 300, 30, hwnd, NULL, hInst, NULL);
    SendMessageW(g_hwndYearLabel, WM_SETFONT, (WPARAM)g_fontLarge, TRUE);
    
    CreateWindowExW(0, L"BUTTON", L"\uB2E4\uC74C \uBD84\uAE30 \u25B6",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        1100, 5, 120, 30, hwnd, (HMENU)1002, hInst, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
