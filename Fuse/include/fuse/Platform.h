#pragma once

#include <fuse/Types.h>

#ifdef UNICODE
#undef UNICODE
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef GetMessage
#undef GetMessage
#endif

namespace fuse {

enum class MouseButton {
  Left,
  Right,
  Shift,
  Control,
  Middle,
  XButton1,
  XButton2,
};

struct FUSE_EXPORT MouseButtons {
  MouseButtons(u32 wParam) : wParam(wParam) {}

  bool IsDown(MouseButton button) { return wParam & (1 << (u32)button); }

  u32 wParam;
};

}  // namespace fuse
