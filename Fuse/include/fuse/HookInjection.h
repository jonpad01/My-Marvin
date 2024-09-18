#pragma once

#include <fuse/Math.h>
#include <fuse/Platform.h>

#include <string_view>

namespace fuse {

struct KeyState {

  //KeyState() : forced(false), pressed(false) {}

  bool forced;
  bool pressed;
};

class FUSE_EXPORT HookInjection {
 public:
  virtual const char* GetHookName() = 0;

  // This runs before GetMessage is called. If it returns true then it will bypass GetMessage so a custom message can be
  // returned.
  virtual bool OnGetMessage(LPMSG lpMsg, HWND hWnd) { return false; };

  // This runs before PeekMessage is called. If it returns true then it will bypass PeekMessage so a custom message can
  // be returned.
  virtual bool OnPeekMessage(LPMSG lpMsg, HWND hWnd) { return false; };

  virtual void OnUpdate(){};

  // This is called after OnUpdate and PeekMessage complete.
  virtual bool OnPostUpdate(BOOL hasMsg, LPMSG lpMsg, HWND hWnd) { return false; };

  virtual bool OnMenuUpdate(BOOL hasMsg, LPMSG lpMsg, HWND hWnd) { return false; };

  virtual void OnMouseMove(const Vector2i& position, MouseButtons buttons) {}
  virtual void OnMouseDown(const Vector2i& position, MouseButton button) {}
  virtual void OnMouseUp(const Vector2i& position, MouseButton button) {}

  virtual void OnQuit() {}

  // Return true if the key should be pressed
  virtual KeyState OnGetAsyncKeyState(int vKey) { return {}; }

  // This is a handler that gets called when a messagebox popup happens.
  // Return true to suppress the actual messagebox popup.
  virtual bool OnMessageBox(std::string_view text, std::string_view caption, UINT type) { return false; }
};

}  // namespace fuse
