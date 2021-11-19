#include "AutoMode.h"

#include "Memory.h"
#include "Multicont.h"
#include "Process.h"

namespace marvin {


AutoBot::AutoBot() : pids_() {
  std::size_t bots = 0;
  std::string hide_window;

  std::cout << "This program will terminate any open continuum processes before operation.\n";
  std::cout << std::endl;

  // ask user how many bots
  std::cout << "How Many Bots?\n";

  do {
    std::cin.clear();
    std::cin.ignore();
    std::cin >> bots;
  } while (bots < 1 || bots > 50 || std::cin.fail());

  std::cout << "Hide Windows? (y/n)   Windows Will Be Minimized If No: \n";

  do {
    std::cin.clear();
    std::cin.ignore();
    std::cin >> hide_window;
  } while ((hide_window != "y" && hide_window != "n") || std::cin.fail());

  if (hide_window == "y") {
    window_state_ = 0;
  } else {
    window_state_ = 6;
  }

  FetchWindows();
  for (WindowInfo window : windows_) {
    IsErrorWindow(window.title, window.pid, window.hwnd);

    std::size_t found_pos = window.title.find("Continuum (enabled) - ");
    if (found_pos == std::string::npos) {
      found_pos = window.title.find("Continuum (disabled) - ");
    }

    bool found = window.title == "Continuum" || window.title == "Continuum 0.40";

    if (found || found_pos != std::string::npos) {
      HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, true, window.pid);
      TerminateProcess(handle, 0);
      CloseHandle(handle);
    }
  }

  // loop through the awnser and start bots
  for (std::size_t i = 0; i < bots; i++) {
     //grab pid and address result and push into vector
    DWORD pid = StartBot(i);

    if (pid == 0) {
      std::cout << "Bot failed to start, this attempt has been terminated.";
      std::cout << std::endl;
    }
    pids_.push_back(pid);
    Sleep(1000);
  }


  std::cout << "\nBot starting loop has finished, this process will monitor and restart bots that get disconected "
               "until closed.\n";
};





// if anything goes wrong, toss the attempt and return 0

DWORD AutoBot::StartBot(std::size_t index) {
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

  // inject the loader and marvin dll
  if (!process->InjectModule(inject_path)) {
    TerminateCont(handle);
    std::cout << "Failed to inject into proccess.\n";
    return 0;
  }

  // wait for the game window to exist and grab the handle
  WindowInfo iInjected = GrabWindow("Continuum (enabled) - ", pid, true, false, true, 10000);

  if (iInjected.hwnd == 0) {
    TerminateCont(handle);
    return 0;
  } 


  ShowWindow(iInjected.hwnd, window_state_);

  if (!WaitForWindowState(iInjected.hwnd, iInjected.title, 2, 10000)) {
    TerminateCont(handle);
    return 0;
  }

  std::cout << "Bot started successfully\n";
  std::cout << std::endl;

  CloseHandle(handle);

  return pid;
}











void AutoBot::MonitorBots() {
  // cycle through each pid and see if it matches to an existing window
  for (std::size_t i = 0; i < pids_.size(); i++) {
    Sleep(1000);

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

    int found = 0;


      std::size_t memory_error = title.find(" - Application Error");

      bool load_error = title == "Error";

      bool information = title == "Information";

      if (memory_error != std::string::npos) {
        std::cout << "Application error found for process pid: " << pid << " with window title: " << title
                  << "\n" << std::endl;
        Sleep(1000);
        PostMessage(hwnd, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0);
        PostMessage(hwnd, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
        found = 1;
      }

      if (load_error) {
        std::cout << "DirectDraw error found for process pid: " << pid << " with window title: " << title
                  << "\n" << std::endl;
        Sleep(1000);
        PostMessage(hwnd, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0);
        PostMessage(hwnd, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
        found = 2;
      }

      if (information) {
        std::cout << "Information window found for process pid: " << pid << " with window title: " << title << "\n"
                  << std::endl;
        Sleep(1000);
        PostMessage(hwnd, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0);
        PostMessage(hwnd, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
        found = 2;
      }

  return found;
}


bool AutoBot::WaitForWindowState(HWND hwnd, std::string title, int state, int timeout) {
  WINDOWPLACEMENT wp;
  wp.length = sizeof(wp);

  bool result = false;
  int max_trys = timeout / 100;

  for (int trys = 0; trys != max_trys; trys++) {
    Sleep(100);   
    if (GetWindowPlacement(hwnd, &wp) && wp.showCmd == state) {
      result = true;
      break;
    }
  }
  if (result == false) {
    std::cout << "Timout while waiting for window change on window: " << title << "\n ";
  } else {
    std::cout << "Window change found on window: " << title << "\n ";
  }
  return result;
}


bool AutoBot::FetchEnterMessage(HANDLE handle, std::size_t module_base, DWORD pid, int timeout) {
  
  std::string enter_msg;
  int max_trys = timeout / 100;
  bool result = false;
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
  } else {
    std::cout << "Chat message found.\n";
  }

  return result;
}


WindowInfo AutoBot::GrabWindow(std::string title, DWORD pid, bool match_pid, bool exact_match, bool show_log, int timeout) {

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

  if (show_log) {
    if (info.hwnd == 0 && error_level != 3) {
      std::cout << "Timed out while looking for window:  " << title << std::endl;
    } else {
      std::cout << "HWND found for window:  " << title << std::endl;
    }
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