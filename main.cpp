#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <vector>
#include <cmath>

struct Point {
    float x, y;
};

// 全局变量
std::vector<Point> g_points;
Point* g_dragPoint = nullptr;
const int CLICK_RADIUS = 10;
int g_selectedIndex = -1;
std::vector<COLORREF> g_colors = { RGB(0,0,255), RGB(0,128,0), RGB(255,0,0), RGB(128,0,128) };
int g_curColor = 0;

// Catmull-Rom 插值函数
Point CatmullRom(const Point& p0, const Point& p1, const Point& p2, const Point& p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    
    Point p;
    p.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
    p.y = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
    return p;
}

void DrawScene(HWND hwnd, HDC hdc) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    
    // 双缓冲防止闪烁
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
    SelectObject(memDC, memBitmap);
    
    // 背景
    FillRect(memDC, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
    
    // 画控制点（高亮选中点）
    for (size_t i = 0; i < g_points.size(); ++i) {
        const auto &p = g_points[i];
        HBRUSH brush = CreateSolidBrush((int)i == g_selectedIndex ? RGB(255, 200, 0) : RGB(255, 0, 0));
        HBRUSH old = (HBRUSH)SelectObject(memDC, brush);
        Ellipse(memDC, (int)p.x - 5, (int)p.y - 5, (int)p.x + 5, (int)p.y + 5);
        SelectObject(memDC, old);
        DeleteObject(brush);
    }
    
    // 画 Catmull-Rom 曲线
    if (g_points.size() >= 2) {
        HPEN hPen = CreatePen(PS_SOLID, 2, g_colors[g_curColor]);
        SelectObject(memDC, hPen);
        
        for (size_t i = 0; i < g_points.size() - 1; ++i) {
            // 构造 4 个控制点，处理边界情况
            Point p0 = (i == 0) ? g_points[i] : g_points[i - 1];
            Point p1 = g_points[i];
            Point p2 = g_points[i + 1];
            Point p3 = (i + 2 >= g_points.size()) ? g_points[i + 1] : g_points[i + 2];
            
            for (int step = 0; step <= 20; ++step) {
                float t = step / 20.0f;
                Point p = CatmullRom(p0, p1, p2, p3, t);
                if (i == 0 && step == 0) MoveToEx(memDC, (int)p.x, (int)p.y, NULL);
                else LineTo(memDC, (int)p.x, (int)p.y);
            }
        }
        DeleteObject(hPen);
    }

    // 绘制颜色选择提示
    HFONT hf = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT oldf = (HFONT)SelectObject(memDC, hf);
    SetTextColor(memDC, RGB(0,0,0));
    SetBkMode(memDC, TRANSPARENT);
    TextOut(memDC, 8, 8, L"Colors: 1-Blue 2-Green 3-Red 4-Purple    Press Delete to remove selected point", 74);
    SelectObject(memDC, oldf);
    
    BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);

            // 检查是否点中已有点（选中并开始拖动）
            bool hit = false;
            for (int i = 0; i < (int)g_points.size(); ++i) {
                auto &p = g_points[i];
                if (hypot(p.x - x, p.y - y) < CLICK_RADIUS) {
                    g_selectedIndex = i;
                    g_dragPoint = &p;
                    SetCapture(hwnd);
                    hit = true;
                    InvalidateRect(hwnd, NULL, FALSE);
                    break;
                }
            }
            if (!hit) {
                // 添加新点并选中
                g_points.push_back({ (float)x, (float)y });
                g_selectedIndex = (int)g_points.size() - 1;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        case WM_RBUTTONDOWN: {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            // 如果右键点击到某个点则删除该点
            for (int i = 0; i < (int)g_points.size(); ++i) {
                auto &p = g_points[i];
                if (hypot(p.x - x, p.y - y) < CLICK_RADIUS) {
                    g_points.erase(g_points.begin() + i);
                    if (g_selectedIndex == i) g_selectedIndex = -1;
                    else if (g_selectedIndex > i) --g_selectedIndex;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
            }
            // 否则删除最后一个点（保持向后兼容）
            if (!g_points.empty()) {
                g_points.pop_back();
                if (g_selectedIndex >= (int)g_points.size()) g_selectedIndex = -1;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (g_dragPoint && (wParam & MK_LBUTTON)) {
                g_dragPoint->x = (float)LOWORD(lParam);
                g_dragPoint->y = (float)HIWORD(lParam);
                // 拖动时只需重绘相邻区域，但为简单起见刷新整个窗口
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
        case WM_LBUTTONUP: {
            g_dragPoint = nullptr;
            ReleaseCapture();
            break;
        }
        case WM_KEYDOWN: {
            // 颜色选择：按 '1'..'4'
            if (wParam == '1') { g_curColor = 0; InvalidateRect(hwnd, NULL, FALSE); return 0; }
            if (wParam == '2') { g_curColor = 1; InvalidateRect(hwnd, NULL, FALSE); return 0; }
            if (wParam == '3') { g_curColor = 2; InvalidateRect(hwnd, NULL, FALSE); return 0; }
            if (wParam == '4') { g_curColor = 3; InvalidateRect(hwnd, NULL, FALSE); return 0; }
            // 删除选中点
            if (wParam == VK_DELETE) {
                if (g_selectedIndex >= 0 && g_selectedIndex < (int)g_points.size()) {
                    g_points.erase(g_points.begin() + g_selectedIndex);
                    g_dragPoint = nullptr;
                    g_selectedIndex = -1;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                return 0;
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            DrawScene(hwnd, hdc);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR pCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"CurveDrawWindowClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClass(&wc);
    
    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Catmull-Rom Curve Draw", WS_OVERLAPPEDWINDOW, 
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);
    
    if (hwnd == NULL) return 0;
    
    ShowWindow(hwnd, nCmdShow);
    
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}
