#include "Debug.h"

namespace marvin {

    std::ofstream debug_log;

#if DEBUG_RENDER

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

    void RenderWorldLine(Vector2f screenCenterWorldPosition, Vector2f from, Vector2f to, COLORREF color) {
        Vector2f center = GetWindowCenter();

        Vector2f diff = to - from;
        from = (from - screenCenterWorldPosition) * 16.0f;
        to = from + (diff * 16.0f);

        RenderLine(center + from, center + to, color);
    }

    void RenderLine(Vector2f from, Vector2f to, COLORREF color) {
        HDC hdc = GetDC(g_hWnd);

        HGDIOBJ obj = SelectObject(hdc, GetStockObject(DC_PEN));

        SetDCPenColor(hdc, color);

        MoveToEx(hdc, (int)from.x, (int)from.y, NULL);
        LineTo(hdc, (int)to.x, (int)to.y);

        ReleaseDC(g_hWnd, hdc);
    }

    void RenderText(std::string text, Vector2f at, COLORREF color, int flags) {

        HDC hdc = GetDC(g_hWnd);

        HGDIOBJ obj = SelectObject(hdc, GetStockObject(DC_BRUSH));

        SetDCBrushColor(hdc, color);

        SetBkColor(hdc, RGB(0, 0, 0));
        SetTextColor(hdc, RGB(255, 255, 255));
        if (flags & RenderText_Centered) {
            SetTextAlign(hdc, TA_CENTER);
        }
        TextOutA(hdc, (int)at.x, (int)at.y, text.c_str(), text.size());

        ReleaseDC(g_hWnd, hdc);
    }

    void RenderPath(Vector2f position, std::vector<Vector2f> path) {

        if (path.empty()) return;

        for (std::size_t i = 0; i < path.size() - 1; ++i) {
            Vector2f current = path[i];
            Vector2f next = path[i + 1];

            if (i == 0) {
                RenderWorldLine(position, position, current, RGB(100, 0, 0));
            }

            RenderWorldLine(position, current, next, RGB(100, 0, 0));
        }
    }

    void WaitForSync() { DwmFlush(); }

#else

    void RenderWorldLine(Vector2f screenCenterWorldPosition, Vector2f from, Vector2f to, COLORREF color) {}

    void RenderLine(Vector2f from, Vector2f to, COLORREF color) {}

    void RenderText(std::string text, Vector2f at, COLORREF color, int flags) {}

    void RenderPath(Vector2f position, std::vector<Vector2f> path) {}

    void WaitForSync() {}
#endif

    Vector2f GetWindowCenter() {
        RECT rect;

        GetClientRect(g_hWnd, &rect);

        return Vector2f((float)rect.right / 2.0f, (float)rect.bottom / 2.0f);
    }

}  // namespace marvin