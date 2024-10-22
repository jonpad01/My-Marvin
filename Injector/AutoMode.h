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

namespace marvin 
{

 struct WindowInfo 
 {
  DWORD pid;
  HWND hwnd;
  std::string title;
};

class AutoBot 
{
 private:
  struct ProcessInfo {
    DWORD pid = 0;
    HANDLE handle = nullptr;
  };

 public:
  AutoBot();
  AutoBot(int startup_arg);

  ~AutoBot() {
    for (std::size_t i = 0; i < process_list_.size(); i++) {
      CloseHandle(process_list_[i].handle);
    }
  }
 
  void StartBot(int bots);
  //DWORD StartBot(std::size_t index);
  ProcessInfo StartBot(std::size_t index);
  DWORD StartContinuum(std::size_t index);
  bool InjectContinuum(DWORD pid);
  void MonitorBots();
  int IsErrorWindow(std::string title, DWORD pid, HWND hwnd);
  WindowInfo GrabWindow(std::string title, DWORD pid, bool match_pid, bool exact_match, bool show_output, int timeout);
  bool WaitForWindowState(HWND hwnd, std::string title, int state, int timeout);
  bool WaitForFile(const std::string& filename, int timeout);
  bool FetchEnterMessage(HANDLE handle, std::size_t module_base, DWORD pid, int timeout);
  void FetchWindows();
  void CloseContinuumWindows();
  void CloseErrorWindows();
  void CloseWindow(const char* name);
  void TerminateCont(HANDLE handle);

 private:

  std::vector<WindowInfo> windows_;
  //std::vector<DWORD> pids_;
  std::vector<ProcessInfo> process_list_;
  ProcessInfo process_list__[1000];
  int num_bots_;
  int window_state_;
};

BOOL __stdcall GrabWindows(HWND hwnd, LPARAM lParam);

}  // namespace marvin
