#include "AutoMode.h"

#include "Memory.h"
#include "Multicont.h"
#include "Process.h"

namespace marvin {
namespace hwnd {

HWND hmenu = NULL;
HWND hprofile = NULL;
HWND hlist = NULL;
HWND hgame = NULL;
HWND hinjected = NULL;

HWND hinformation = NULL;
HWND herror = NULL;
HWND happlication = NULL;
HWND hruntime = NULL;

}  // namespace hwnd



AutoBot::AutoBot() : pids_() {
  std::size_t bots = 0;
  std::string hide_window;

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
      HWND hMenu = GrabWindow("Continuum 0.40", pid, true, true, 10000);

      if (hMenu == 0) {
        TerminateCont(handle);
        return 0;
      }

#if 0
      // find the menu handle by using the pid, or time out and return
      for (std::size_t trys = 0;; trys++) {
        Sleep(100);
        // this function will assign the hwnd using the pid inserted
        if (!EnumWindows(FindMenu, pid)) {
          break;
        }
        if (trys > 100) {
          TerminateProcess(handle, 0);
          CloseHandle(handle);
          std::cout << "Failed to find menu.\n";
          return 0;
        }
      }


      //#if 0
      // the code to open the profile menu
      if (hwnd::hmenu) {
        PostMessageA(hwnd::hmenu, WM_COMMAND, 40011, 0);
      }
#endif
      // the code to open the profile menu
      PostMessageA(hMenu, WM_COMMAND, 40011, 0);

      // wait for the profile handle
      HWND hProfile = GrabWindow("Select/Edit Profile", pid, true, true, 10000);

      if (hProfile == 0) {
        TerminateCont(handle);
        return 0;
      }

      #if 0
      // wait for the profile handle
      for (std::size_t trys = 0;; trys++) {
        Sleep(100);
        if (!EnumWindows(FindProfile, pid)) {
          break;
        }

        if (trys > 100) {
          TerminateProcess(handle, 0);
          CloseHandle(handle);
          std::cout << "Failed to find profile handle.\n";
          return 0;
        }
      }




      // grab the handle for the list box
      for (std::size_t trys = 0;; trys++) {
        Sleep(200);

        if (hwnd::hlist = FindWindowExA(hwnd::hprofile, NULL, "ListBox", NULL)) {
          break;
        }

        if (trys > 100) {
          TerminateProcess(handle, 0);
          CloseHandle(handle);
          std::cout << "Failed to find listbox handle.\n";
          return 0;
        }
      }
#endif



      // grab the handle for the list box
      HWND hListbox = FindWindowExA(hProfile, NULL, "ListBox", NULL);

      if (hListbox == 0) {
        TerminateCont(handle);
        return 0;
      } 

      // sets the profile
      PostMessageA(hListbox, LB_SETCURSEL, (index), 0);

      // close profile
      PostMessageA(hProfile, WM_COMMAND, 1, 0);

      // good method but wont set the default zone when changing profiles
      // process->SetProfile(handle, index);

      // Start Game, allow zone list to update first
      Sleep(2000);
      PostMessageA(hMenu, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0);
      PostMessageA(hMenu, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
    

      // wait for the game window to exist and grab the handle
      HWND hGame = GrabWindow("Continuum", pid, true, true, 10000);

      if (hGame == 0) {
        TerminateCont(handle);
        return 0;
      } 

      if (!WaitForWindowState(hGame, "Continuum", 1, 10000)) {
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

#if 0
      for (std::size_t trys = 0;; trys++) {
        Sleep(1000);
        if (!EnumWindows(FindGame, pid)) {
          break;
        } else {
          if (trys > 100) {
            TerminateProcess(handle, 0);
            CloseHandle(handle);
            std::cout << "Failed to find continuum window handle.\n";
            return 0;
          }

          if (FindErrorWindows(pid) == 2) { 
            TerminateProcess(handle, 0);
            CloseHandle(handle);
            std::cout << "Error window found while looking for continuum window.\n";
            return 0;
          }

          if (!EnumWindows(FindInformation, pid)) {
            PostMessage(hwnd::hinformation, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0);
            PostMessage(hwnd::hinformation, WM_KEYUP, (WPARAM)(VK_RETURN), 0);

            TerminateProcess(handle, 0);
            CloseHandle(handle);
            std::cout << "Connection timeout while logging in (information window popup).\n";
            return 0;
          }
        }
      }
#endif
     


  // need a second after getting chat or game will likely crash when trying to minimize or hide window
  Sleep(1000);

  // inject the loader and marvin dll
  if (!process->InjectModule(inject_path)) {
    TerminateCont(handle);
    std::cout << "Failed to inject into proccess.\n";
    return 0;
  }

  // wait for the game window to exist and grab the handle
  HWND hInjected = GrabWindow("Continuum (enabled)", pid, true, true, 10000);

  if (hInjected == 0) {
    TerminateCont(handle);
    return 0;
  } 

  #if 0
  // wait for windows title to change before minimizing, allow this to hang for map downloads
  for (std::size_t trys = 0;; trys++) {
    Sleep(3000);

    if (!EnumWindows(FindInjectedTitle, pid)) {
      break;
    }

    if (trys > 100) {
      TerminateProcess(handle, 0);
      CloseHandle(handle);
      std::cout << "Continuum title failed to change to injected title.\n";
      return 0;
    }
  }
  

  // 6 to minimize or 0 to hide the game
  if (hidden_ && hwnd::hgame)
    ShowWindow(hwnd::hgame, 0);
  else if (hwnd::hgame) {
    ShowWindow(hwnd::hgame, 6);
  }
#endif

  ShowWindow(hInjected, window_state_);

  if (!WaitForWindowState(hInjected, "Continuum (enabled)", 2, 10000)) {
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
  
    HWND hMenu = GrabWindow("Continuum 0.40", 0, false, false, 100);

    while (hMenu) {
      PostMessage(hMenu, WM_SYSCOMMAND, (WPARAM)(SC_CLOSE), 0);
      hMenu = GrabWindow("Continuum 0.40", 0, false, false, 100);
    }

    #if 0
    if (!EnumWindows(FindExtraMenu, NULL)) {
      PostMessage(hwnd::hmenu, WM_SYSCOMMAND, (WPARAM)(SC_CLOSE), 0);
    }
    #endif

    HWND hInjected = GrabWindow("Continuum (enabled)", pids_[i], true, false, 1000);

    if (hInjected == 0) {
      hInjected = GrabWindow("Continuum (disabled)", pids_[i], true, false, 1000);
    }
    if (hInjected == 0) {
      DWORD pid = StartBot(i);

      if (pid == 0) {
        std::cout << "Bot failed to start, this attempt has been terminated.";
        std::cout << std::endl;
      }
      pids_[i] = pid;
    }

    #if 0 
    // if the pid didnt return a handle, start a new bot
    if (EnumWindows(FindInjectedTitle, pids_[i])) {
      Sleep(1000);

      if (!EnumWindows(FindMenu, pids_[i])) {
        PostMessage(hwnd::hmenu, WM_SYSCOMMAND, (WPARAM)(SC_CLOSE), 0);
      }

      pids_[i] = StartBot(i);
    }
    #endif
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
  
#if 0
  if (!EnumWindows(FindApplicationError, NULL)) {
    PostMessage(hwnd::happlication, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0);
    PostMessage(hwnd::happlication, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
    found = 1;
  }

  if (!EnumWindows(FindContinuumApplicationError, NULL)) {
    PostMessage(hwnd::happlication, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0);
    PostMessage(hwnd::happlication, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
    found = 1;
  }

  if (!EnumWindows(FindRunTimeError, NULL)) {
    PostMessage(hwnd::hruntime, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0);
    PostMessage(hwnd::hruntime, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
    found = 1;
  }

  if (!EnumWindows(FindError, pid)) {
    PostMessage(hwnd::herror, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0);
    PostMessage(hwnd::herror, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
    found = 2;
  }
  #endif

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


HWND AutoBot::GrabWindow(std::string title, DWORD pid, bool match_pid, bool show_log, int timeout) {

  int max_trys = timeout / 100;
  HWND hwnd = 0;
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
      if (window.title == title) {
        if (!match_pid || match_pid && window.pid == pid) {
          hwnd = window.hwnd;
          break;
        }
      }
    }
    if (hwnd != 0 || error_level == 3) {
      break;
    }
  }

  if (show_log) {
    if (hwnd == 0 && error_level != 3) {
      std::cout << "Timed out while looking for window:  " << title << std::endl;
    } else {
      std::cout << "HWND found for window:  " << title << std::endl;
    }
  }

  return hwnd;
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






BOOL __stdcall FindMenu(HWND hwnd, LPARAM lParam) {
  DWORD pid;
  GetWindowThreadProcessId(hwnd, &pid);

  if (pid == (DWORD)lParam) {
    char title[1024];
    GetWindowTextA(hwnd, title, 1024);

    if (strcmp(title, "Continuum 0.40") == 0) {
      hwnd::hmenu = hwnd;

      return FALSE;
    }
  }

  return TRUE;
}

BOOL __stdcall FindExtraMenu(HWND hwnd, LPARAM lParam) {
  char title[1024];
  GetWindowTextA(hwnd, title, 1024);

  if (strcmp(title, "Continuum 0.40") == 0) {
    hwnd::hmenu = hwnd;

    return FALSE;
  }

  return TRUE;
}

BOOL __stdcall FindProfile(HWND hwnd, LPARAM lParam) {
  DWORD pid;
  GetWindowThreadProcessId(hwnd, &pid);

  if (pid == (DWORD)lParam) {
    char title[1024];
    GetWindowTextA(hwnd, title, 1024);

    if (strcmp(title, "Select/Edit Profile") == 0) {
      hwnd::hprofile = hwnd;

      return FALSE;
    }
  }

  return TRUE;
}

BOOL __stdcall FindInformation(HWND hwnd, LPARAM lParam) {
  DWORD pid;
  GetWindowThreadProcessId(hwnd, &pid);

  if (pid == (DWORD)lParam) {
    char title[1024];
    GetWindowTextA(hwnd, title, 1024);

    if (strcmp(title, "Information") == 0) {
      hwnd::hinformation = hwnd;

      return FALSE;
    }
  }

  return TRUE;
}

BOOL __stdcall FindGame(HWND hwnd, LPARAM lParam) {
  DWORD pid;
  GetWindowThreadProcessId(hwnd, &pid);

  if (pid == (DWORD)lParam) {
    char title[1024];
    GetWindowTextA(hwnd, title, 1024);

    if (strcmp(title, "Continuum") == 0) {
      hwnd::hgame = hwnd;

      return FALSE;
    }
  }

  return TRUE;
}

BOOL __stdcall FindInjectedTitle(HWND hwnd, LPARAM lParam) {
  char title[1024];
  DWORD pid;

  GetWindowThreadProcessId(hwnd, &pid);
  GetWindowTextA(hwnd, title, 1024);

  if (pid == (DWORD)lParam) {
    if (strcmp(title, "Continuum (enabled)") == 0 || strcmp(title, "Continuum (disabled)") == 0) {
      hwnd::hinjected = hwnd;

      return FALSE;
    }
  }

  return TRUE;
}

BOOL __stdcall FindError(HWND hwnd, LPARAM lParam) {
  DWORD pid;
  GetWindowThreadProcessId(hwnd, &pid);

  if (pid == (DWORD)lParam) {
    char title[1024];
    GetWindowTextA(hwnd, title, 1024);

    if (strcmp(title, "Error") == 0) {
      hwnd::herror = hwnd;

      return FALSE;
    }
  }

  return TRUE;
}

BOOL __stdcall FindContinuumApplicationError(HWND hwnd, LPARAM lParam) {
  char title[1024];
  GetWindowTextA(hwnd, title, 1024);

  

  if (strcmp(title, "Continuum (enabled): Windows - Application Error") == 0) {
  
    hwnd::happlication = hwnd;

    return FALSE;
  }

  return TRUE;
}

BOOL __stdcall FindApplicationError(HWND hwnd, LPARAM lParam) {
  char title[1024];
  GetWindowTextA(hwnd, title, 1024);

  std::string sTitle = title;

  //if (strcmp(title, "Windows - Application Error") == 0) {
    if (sTitle.find("Application Error")) {
    hwnd::happlication = hwnd;

    return FALSE;
  }

  return TRUE;
}

BOOL __stdcall FindRunTimeError(HWND hwnd, LPARAM lParam) {
  char title[1024];
  GetWindowTextA(hwnd, title, 1024);

  if (strcmp(title, "Microsoft Visual C++ Runtime Library") == 0) {
    hwnd::hruntime = hwnd;

    return FALSE;
  }

  return TRUE;
}

}  // namespace marvin