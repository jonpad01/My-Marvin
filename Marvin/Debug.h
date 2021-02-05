
#pragma once

#include <fstream>

#include "Vector2f.h"
#include "platform/Platform.h"

#define DEBUG_RENDER 1
#define DEBUG_USER_CONTROL 1

extern HWND g_hWnd;

namespace marvin {

    extern std::ofstream debug_log;

    enum RenderTextFlags {
        RenderText_Centered = (1 << 1),
    };

    void RenderWorldLine(Vector2f screenCenterWorldPosition, Vector2f from, Vector2f to);
    void RenderLine(Vector2f from, Vector2f to, COLORREF color);
    void RenderText(std::string text, Vector2f at, COLORREF color, int flags = 0);
    void WaitForSync();

    Vector2f GetWindowCenter();

}  // namespace marvin