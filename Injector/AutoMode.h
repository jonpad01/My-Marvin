#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#ifdef UNICODE
#undef UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

namespace marvin {

 struct WindowInfo {
  DWORD pid;
  HWND hwnd;
  std::string title;
};

class AutoBot {
 public:
  AutoBot();
 

  DWORD StartBot(std::size_t index);
  void MonitorBots();
  int IsErrorWindow(std::string title, DWORD pid, HWND hwnd);
  WindowInfo GrabWindow(std::string title, DWORD pid, bool match_pid, bool exact_match, bool show_log, int timeout);
  bool WaitForWindowState(HWND hwnd, std::string title, int state, int timeout);
  bool FetchEnterMessage(HANDLE handle, std::size_t module_base, DWORD pid, int timeout);
  void FetchWindows();
  void TerminateCont(HANDLE handle);

 private:
  std::vector<WindowInfo> windows_;
  std::vector<DWORD> pids_;

  int window_state_;
};

BOOL __stdcall GrabWindows(HWND hwnd, LPARAM lParam);

}  // namespace marvin
