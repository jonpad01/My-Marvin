#include "Debug.h"

#include <ddraw.h>
#include <stdarg.h>

#include "Types.h"

#pragma comment(lib, "ddraw.lib")
#pragma comment(lib, "dxguid.lib")

namespace marvin {

RenderState g_RenderState;

std::ofstream debug_log;
std::ofstream error_log;
std::ofstream memory_log;

const bool RenderState::kDisplayDebugText = true;

#if DEBUG_RENDER
//bot crashes when screen moves to bottom edge
void RenderWorldLine(Vector2f screenCenterWorldPosition, Vector2f from, Vector2f to, COLORREF color) {
  Vector2f center = GetWindowCenter();

  Vector2f diff = to - from;
  from = (from - screenCenterWorldPosition) * 16.0f;
  to = from + (diff * 16.0f);

  RenderLine(center + from, center + to, color);
}

void RenderLine(Vector2f from, Vector2f to, COLORREF color) {
  RenderableLine renderable;

  renderable.from = from;
  renderable.to = to;
  renderable.color = color;

  g_RenderState.renderable_lines.push_back(renderable);
}

void RenderWorldBox(Vector2f screenCenterWorldPosition, Vector2f box_top_left, Vector2f box_bottom_right,
                    COLORREF color) {
  Vector2f diff = box_bottom_right - box_top_left;

  Vector2f box_top_right = box_top_left + Vector2f(diff.x, 0);
  Vector2f box_bottom_left = box_top_left + Vector2f(0, diff.y);

  RenderWorldLine(screenCenterWorldPosition, box_top_left, box_top_right, color);
  RenderWorldLine(screenCenterWorldPosition, box_top_right, box_bottom_right, color);
  RenderWorldLine(screenCenterWorldPosition, box_top_left, box_bottom_left, color);
  RenderWorldLine(screenCenterWorldPosition, box_bottom_left, box_bottom_right, color);
}

void RenderWorldText(Vector2f screenCenterWorldPosition, const std::string& text, const Vector2f& at, TextColor color,
                     int flags) {
  Vector2f center = GetWindowCenter();

  Vector2f screen_at = center + (at - screenCenterWorldPosition) * 16.0f;

  if (screen_at.x > 0 && screen_at.y > 0 && screen_at.x < center.x * 2 && screen_at.y < center.y * 2) {
    RenderText(text, screen_at, color, flags);
  }
}

void RenderState::Render() {
  u32 graphics_addr = *(u32*)(0x4C1AFC) + 0x30;
  LPDIRECTDRAWSURFACE back_surface = (LPDIRECTDRAWSURFACE) * (u32*)(graphics_addr + 0x44);

  typedef void(__fastcall * RenderTextFunc)(void* This, void* thiscall_garbage, int x, int y, const char* text,
                                            int zero, int length, u8 alpha);

  RenderTextFunc render_text = (RenderTextFunc)(0x442FE0);
  void* This = (void*)(graphics_addr);

  HDC hdc;
  back_surface->GetDC(&hdc);

  // void RenderLine(Vector2f from, Vector2f to, COLORREF color) {
  //   HDC hdc = GetDC(g_hWnd);

  HGDIOBJ obj = SelectObject(hdc, GetStockObject(DC_PEN));

  // SetDCPenColor(hdc, color);

  for (RenderableLine& renderable : renderable_lines) {
    SetDCPenColor(hdc, renderable.color);
    MoveToEx(hdc, (int)renderable.from.x, (int)renderable.from.y, NULL);
    LineTo(hdc, (int)renderable.to.x, (int)renderable.to.y);
  }

  back_surface->ReleaseDC(hdc);

  for (RenderableText& renderable : renderable_texts) {
    u32 x = (u32)renderable.at.x;
    u32 y = (u32)renderable.at.y;

    if (renderable.flags & RenderText_Centered) {
      x -= (u32)((renderable.text.length() / 2.0f) * 8.0f);
    }

    render_text(This, 0, x, y, renderable.text.c_str(), (int)renderable.color, -1, 1);
  }
}

void RenderState::RenderDebugText(const char* fmt, ...) {
  if (!kDisplayDebugText) return;

  char buffer[2048];

  va_list args;

  va_start(args, fmt);
  vsprintf_s(buffer, fmt, args);
  va_end(args);

  RenderText(std::string(buffer), Vector2f(GetWindowCenter().x + 150.0f, debug_y), TextColor::Pink, 0);
  debug_y += 12.0f;
}

void RenderText(std::string text, Vector2f at, TextColor color, int flags) {
  RenderableText renderable;

  renderable.text = std::string(text);
  renderable.at = at;
  renderable.color = color;
  renderable.flags = flags;

  g_RenderState.renderable_texts.push_back(std::move(renderable));
}

void RenderPlayerPath(Vector2f position, std::vector<Vector2f> path) {
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

void RenderPath(Vector2f position, std::vector<Vector2f> path) {
  if (path.empty()) return;

  for (std::size_t i = 0; i < path.size() - 1; ++i) {
    Vector2f current = path[i];
    Vector2f next = path[i + 1];

    RenderWorldLine(position, current, next, RGB(100, 0, 0));
  }
}

#else

void RenderState::Render() {}

void RenderWorldLine(Vector2f screenCenterWorldPosition, Vector2f from, Vector2f to, COLORREF color) {}
void RenderWorldBox(Vector2f screenCenterWorldPosition, Vector2f box_top_left, Vector2f box_bottom_right,
                    COLORREF color) {}
void RenderWorldText(Vector2f screenCenterWorldPosition, const std::string& text, const Vector2f& at, TextColor color,
                     int flags) {}

void RenderLine(Vector2f from, Vector2f to, COLORREF color) {}

// void RenderText(std::string text, Vector2f at, COLORREF color, int flags) {}

void RenderState::RenderDebugText(const char* fmt, ...) {}

void RenderText(std::string, Vector2f at, TextColor color, int flags) {}

void RenderPath(Vector2f position, std::vector<Vector2f> path) {}

// void WaitForSync() {}
#endif

Vector2f GetWindowCenter() {
  RECT rect;

  GetClientRect(g_hWnd, &rect);

  return Vector2f((float)rect.right / 2.0f, (float)rect.bottom / 2.0f);
}

}  // namespace marvin
