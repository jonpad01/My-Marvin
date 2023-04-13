#include "AutoMode.h"

#include "Memory.h"
#include "Multicont.h"
#include "Process.h"
#include "Debug.h"

namespace marvin 
{


AutoBot::AutoBot() : pids_() 
{
  num_bots_ = 0;

  std::cout << "How Many Bots?" << std::endl;
  std::cin >> num_bots_;

 while (num_bots_ < 1 || num_bots_ > 50 || !std::cin) 
 {
    if (!std::cin) {
      std::cin.clear();
      std::cin.ignore(10000, '\n');
    }
    std::cout << "Input is range 1-50" << std::endl;
    std::cin >> num_bots_;
 }

  window_state_ = 6;

  CloseErrorWindows();
  CloseContinuumWindows();
  StartBot(num_bots_);

  std::cout << "Bot starting loop has finished, this process will monitor and restart bots that get disconected." << std::endl;
};

AutoBot::AutoBot(int startup_arg) : pids_() 
{
  num_bots_ = startup_arg;
  window_state_ = 6;
  CloseErrorWindows();
  CloseContinuumWindows();
  StartBot(num_bots_);
}

void AutoBot::CloseContinuumWindows()
{
  CloseWindow("Continuum");
}

void AutoBot::CloseWindow(const char* title)
{
  FetchWindows();

  for (WindowInfo window : windows_) 
  {
    std::size_t found_pos = window.title.find(title);
    if (found_pos != std::string::npos) 
    {
      HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, true, window.pid);
      TerminateProcess(handle, 0);
      CloseHandle(handle);
    }
  }
}

void AutoBot::CloseErrorWindows()
{
  FetchWindows();

  for (WindowInfo window : windows_) {
    IsErrorWindow(window.title, window.pid, window.hwnd);
  }
}

DWORD AutoBot::StartBot(std::size_t index) {

  DWORD pid = StartContinuum(index);

  if (!InjectContinuum(pid)) {
    pid = 0;
  }

  std::cout << "Restart successfull" << std::endl << std::endl;

  return pid;
}

void AutoBot::StartBot(int bots) {

    for (int i = 0; i < bots; i++) {

      DWORD pid = StartContinuum(i);

      if (pid == 0) {
        std::cout << "Bot failed to start, this attempt has been terminated." << std::endl;
      }
      else if (!InjectContinuum(pid)) {
        pid = 0;
        std::cout << "Bot failed to inject, this attempt has been terminated." << std::endl;
      }

      pids_.push_back(pid);
      Sleep(1000);
    }
}





// if anything goes wrong, toss the attempt and return 0 (invalid pid)

DWORD AutoBot::StartContinuum(std::size_t index) {
  DWORD pid;

  auto cont = std::make_unique<Multicont>();

  if (!cont->RunMulticont()) {
    std::cout << "Failed to Start Multicont.\n";
    Sleep(3000);
    return 0;
  } else {
    pid = cont->GetPid();
  }

  // grab path and access to Process
  std::string inject_path = marvin::GetWorkingDirectory() + "\\" + INJECT_MODULE_NAME;
  auto process = std::make_unique<marvin::Process>(pid);
  HANDLE handle = process->GetHandle();

  // find the menu handle by using the pid, or time out and return
  WindowInfo iMenu = GrabWindow("Continuum 0.40", pid, true, true, true, 10000);

  if (iMenu.hwnd == 0) {
    TerminateCont(handle);
    return 0;
  }

  // the code to open the profile menu
  PostMessageA(iMenu.hwnd, WM_COMMAND, 40011, 0);

  // wait for the profile handle
  WindowInfo iProfile = GrabWindow("Select/Edit Profile", pid, true, true, true, 10000);

  if (iProfile.hwnd == 0) {
    TerminateCont(handle);
    return 0;
  }

  // grab the handle for the list box
  HWND hListbox = FindWindowExA(iProfile.hwnd, NULL, "ListBox", NULL);

  if (hListbox == 0) {
    TerminateCont(handle);
    return 0;
  }

  // sets the profile
  PostMessageA(hListbox, LB_SETCURSEL, (index), 0);

  // close profile
  PostMessageA(iProfile.hwnd, WM_COMMAND, 1, 0);

  // good method but wont set the default zone when changing profiles
  // process->SetProfile(handle, index);

  // Start Game, allow zone list to update first
  Sleep(2000);
  PostMessageA(iMenu.hwnd, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0);
  PostMessageA(iMenu.hwnd, WM_KEYUP, (WPARAM)(VK_RETURN), 0);

  // wait for the game window to exist and grab the handle
  WindowInfo iGame = GrabWindow("Continuum", pid, true, true, true, 10000);

  if (iGame.hwnd == 0) {
    TerminateCont(handle);
    return 0;
  }

  if (!WaitForWindowState(iGame.hwnd, iGame.title, 1, 10000)) {
    TerminateCont(handle);
    return 0;
  }

  // wait for the client to log in by waiting for the chat address to return an enter message
  // this means the game a successfully connected
  std::size_t module_base = process->GetModuleBase("Continuum.exe");

  if (!FetchEnterMessage(handle, module_base, pid, 10000)) {
    TerminateCont(handle);
    return 0;
  }

  // need a second after getting chat or game will likely crash when trying to minimize or hide window
  Sleep(1000);

  ShowWindow(iGame.hwnd, window_state_);

  if (!WaitForWindowState(iGame.hwnd, iGame.title, 2, 10000)) {
    TerminateCont(handle);
    return 0;
  }
  CloseHandle(handle);
  return pid;
}


bool AutoBot::InjectContinuum(DWORD pid) {

  std::string inject_path = marvin::GetWorkingDirectory() + "\\" + INJECT_MODULE_NAME;
  auto process = std::make_unique<marvin::Process>(pid);
  HANDLE handle = process->GetHandle();

  // inject the loader and marvin dll
  if (!handle || !process->InjectModule(inject_path)) {
    TerminateCont(handle);
    std::cout << "Failed to inject into proccess.\n";
    return false;
  }

  // wait for the game window to exist and grab the handle
  WindowInfo iInjected = GrabWindow("Continuum (enabled) - ", pid, true, false, true, 10000);

  if (iInjected.hwnd == 0) {
    TerminateCont(handle);
    return false;
  } 

  CloseHandle(handle);
  return true;
}











void AutoBot::MonitorBots() {
  // cycle through each pid and see if it matches to an existing window
  for (std::size_t i = 0; i < pids_.size(); i++) {
    Sleep(1000);

    auto process = std::make_unique<marvin::Process>(pids_[i]);
    HANDLE handle = process->GetHandle();

    ULONG exitcode;
    bool bRestart = GetExitCodeProcess(handle, &exitcode);

    if (bRestart) {
      if (exitcode != 259) {
        std::cout << "Exit code found: " << (int)exitcode << std::endl;
        
      }
    }

    CloseHandle(handle);

    FetchWindows();
    for (WindowInfo window : windows_) {
      IsErrorWindow(window.title, window.pid, window.hwnd);
    }
  
    WindowInfo iMenu = GrabWindow("Continuum 0.40", 0, false, true, false, 100);

    while (iMenu.hwnd) {
      PostMessage(iMenu.hwnd, WM_SYSCOMMAND, (WPARAM)(SC_CLOSE), 0);
      iMenu = GrabWindow("Continuum 0.40", 0, false, true, false, 100);
    }

    WindowInfo iInjected = GrabWindow("Continuum (enabled) - ", pids_[i], true, false, false, 1000);

    if (iInjected.hwnd == 0) {
      iInjected = GrabWindow("Continuum (disabled) - ", pids_[i], true, false, false, 1000);
    }
    if (iInjected.hwnd == 0) {
      DWORD pid = StartBot(i);

      if (pid == 0) {
        std::cout << "Bot failed to start, this attempt has been terminated.";
        std::cout << std::endl;
      }
      pids_[i] = pid;
    }
  }
}



int AutoBot::IsErrorWindow(std::string title, DWORD pid, HWND hwnd) {

    int result = 0;

      std::size_t memory_error = title.find(" - Application Error");

      bool load_error = title == "Error";
      bool information = title == "Information";

      if (memory_error != std::string::npos) {
        HWND hText = GetDlgItem(hwnd, 0x0000FFFF);
        char text[1024];
        GetWindowTextA(hText, text, 1024);

        marvin::debug_log << title << " - " << text << std::endl;
        std::cout << "Application error found for process pid: " << pid << " with window title: " << title
                  << "\n" << std::endl;
        
        Sleep(1000);
        PostMessage(hwnd, WM_SYSCOMMAND, (WPARAM)(SC_CLOSE), 0);
        result = 1;
      }

      if (load_error) {
        std::cout << "DirectDraw error found for process pid: " << pid << " with window title: " << title
                  << "\n" << std::endl;
        Sleep(1000);
        PostMessage(hwnd, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0);
        PostMessage(hwnd, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
        result = 2;
      }

      if (information) {
        std::cout << "Information window found for process pid: " << pid << " with window title: " << title << "\n"
                  << std::endl;
        Sleep(1000);
        PostMessage(hwnd, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0);
        PostMessage(hwnd, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
        result = 2;
      }

  return result;
}


bool AutoBot::WaitForWindowState(HWND hwnd, std::string title, int state, int timeout) {
  WINDOWPLACEMENT wp;
  wp.length = sizeof(wp);

  bool result = false;
  int max_trys = (timeout / 100) + 1;

  for (int trys = 0; trys != max_trys; trys++) {
    Sleep(100);   
    if (GetWindowPlacement(hwnd, &wp) && wp.showCmd == state) {
      result = true;
      break;
    }
  }
  if (result == false) {
    std::cout << "Timout while waiting for window change on window: " << title << "\n ";
  } 

  return result;
}


bool AutoBot::FetchEnterMessage(HANDLE handle, std::size_t module_base, DWORD pid, int timeout) {
  
  std::string enter_msg;
  int max_trys = timeout / 100;
  bool result = false;
  int error_level = 0;

  for (int trys = 0; trys <= max_trys; trys++) {
    Sleep(100);
    FetchWindows();
    for (WindowInfo window : windows_) {
      error_level = IsErrorWindow(window.title, window.pid, window.hwnd);
      if (error_level == 2 && window.pid == pid) {
        error_level = 3;
        break;
      }
    }

    if (error_level == 3) {
      break;
    }

    enter_msg = marvin::memory::ReadChatEntry(module_base, handle);

    // skip the log file message
    if (enter_msg.compare(0, 14, "Log file open:") == 0) {
      continue;
    }

    if (!enter_msg.empty()) {
      result = true;
      break;
    }
  }

  if (enter_msg.empty() && error_level != 3) {
    std::cout << "Timeout while trying to find chat message.\n";
  } 

  return result;
}


WindowInfo AutoBot::GrabWindow(std::string title, DWORD pid, bool match_pid, bool exact_match, bool show_output, int timeout) {
  int max_trys = timeout / 100;
  WindowInfo info{0, 0, ""};
  int error_level = 0;

  for (int trys = 0; trys != max_trys; trys++) {
    Sleep(100);
    FetchWindows();
    for (WindowInfo window : windows_) {
      error_level = IsErrorWindow(window.title, window.pid, window.hwnd);
      if (error_level == 2 && window.pid == pid) {
        error_level = 3;
        break;
      }
      std::size_t pos = window.title.find(title);

      if (window.title == title || !exact_match && pos != std::string::npos) {
        if (!match_pid || match_pid && window.pid == pid) {
          info.hwnd = window.hwnd;
          info.pid = window.pid;
          info.title = window.title;
          break;
        }
      }
    }
    if (info.hwnd != 0 || error_level == 3) {
      break;
    }
  }

  if (show_output && info.hwnd == 0 && error_level != 3) {
    std::cout << "Timed out while looking for window:  " << title << std::endl;
  }

  return info;
}



void AutoBot::FetchWindows() {

    windows_.clear();
    EnumWindows(GrabWindows, reinterpret_cast<LPARAM>(&windows_));
}



BOOL __stdcall GrabWindows(HWND hwnd, LPARAM lParam) {

  std::vector<WindowInfo>& windows = *reinterpret_cast<std::vector<WindowInfo>*>(lParam);

  WindowInfo windowinfo;

  if (IsWindowVisible(hwnd)) {
    char title[1024];
    GetWindowTextA(hwnd, title, 1024);

    windowinfo.title = title;

    if (windowinfo.title.empty()) {
      return TRUE;
    }

    GetWindowThreadProcessId(hwnd, &windowinfo.pid);

    windowinfo.hwnd = hwnd;

    windows.push_back(windowinfo);
  }

  return TRUE;
}


void AutoBot::TerminateCont(HANDLE handle) {
  TerminateProcess(handle, 0);
  CloseHandle(handle);
}

}  // namespace marvin