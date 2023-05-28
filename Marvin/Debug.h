
#pragma once

#include <fstream>
#include <vector>

#include "Vector2f.h"
#include "platform/Platform.h"

#define DEBUG_RENDER 1
#define DEBUG_USER_CONTROL 0

#define DEBUG_RENDER_BASE_PATHS 0

#define DEBUG_RENDER_INFLUENCE 0
#define DEBUG_RENDER_INFLUENCE_TEXT 0

#define DEBUG_RENDER_REGION_REGISTRY 0
#define DEBUG_RENDER_PATHFINDER 0
#define DEBUG_RENDER_PATHNODESEARCH 0

#define DEBUG_RENDER_SHOOTER 0

#define DEBUG_RENDER_FIND_ENEMY_IN_BASE_NODE 0

#define DEBUG_DISABLE_BEHAVIOR 0

extern HWND g_hWnd;

namespace marvin {

class LogFile {
 public:
     ~LogFile() { 
         log.close(); 
     }
  void Open(const std::string& name) {
    if (!log.is_open()) {
      log.open(name + ".log", std::ios::out | std::ios::trunc);
    }
  }
  void Write(const std::string& msg, uint64_t timer) {
    if (log.is_open()) {
      log << msg << " - Time: " << timer << std::endl;
    }
  }
  void Write(const std::string& msg) {
    if (log.is_open()) {
      log << msg << std::endl;
    } 
  }

 private:
  std::ofstream log;
};

//extern std::ofstream debug_log;
extern LogFile log;

enum class TextColor { White, Green, Blue, Red, Yellow, Fuchsia, DarkRed, Pink };

enum RenderTextFlags {
  RenderText_Centered = (1 << 1),
};

struct RenderableText {
  std::string text;
  Vector2f at;
  TextColor color;
  int flags;
};

struct RenderableLine {
  Vector2f from;
  Vector2f to;
  COLORREF color;
};

struct RenderState {
  static const bool kDisplayDebugText;
  float debug_y;

  std::vector<RenderableText> renderable_texts;
  std::vector<RenderableLine> renderable_lines;

  void Render();

  void RenderDebugText(const char* fmt, ...);
};

extern RenderState g_RenderState;

void RenderWorldLine(Vector2f screenCenterWorldPosition, Vector2f from, Vector2f to, COLORREF color);
void RenderDirection(Vector2f screenCenterWorldPosition, Vector2f from, Vector2f direction, float length);
void RenderWorldBox(Vector2f screenCenterWorldPosition, Vector2f position, float size);
void RenderWorldBox(Vector2f screenCenterWorldPosition, Vector2f box_top_left, Vector2f box_bottom_right,
                    COLORREF color);
void RenderWorldText(Vector2f screenCenterWorldPosition, const std::string& text, const Vector2f& at, TextColor color, int flags = 0);
void RenderLine(Vector2f from, Vector2f to, COLORREF color);
// void RenderText(std::string text, Vector2f at, COLORREF color, int flags = 0);
void RenderText(std::string, Vector2f at, TextColor color, int flags = 0);
void RenderPlayerPath(Vector2f position, std::vector<Vector2f> path);
void RenderPath(Vector2f position, std::vector<Vector2f> path);
    // void WaitForSync();

Vector2f GetWindowCenter();

}  // namespace marvin