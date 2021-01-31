#include "AutoMode.h"
#include "Process.h"
#include "Multicont.h"
#include "Memory.h"


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
        }


        AutoBot::AutoBot() : pids_() {

            std::size_t bots = 0;
            hidden_ = false;
            std::string hide_window;

            //ask user how many bots
            std::cout << "How Many Bots?\n";
            do {
                std::cin.clear();
                std::cin.ignore();
                std::cin >> bots;
            } while (bots < 1 || bots > 50 || std::cin.fail());

            std::cout << "Hide Windows? (y/n)   Windows Will Be Minimized If No: ";

            do {
                std::cin.clear();
                std::cin.ignore();
                std::cin >> hide_window;
            } while ((hide_window != "y" && hide_window != "n") || std::cin.fail());

            if (hide_window == "y") { hidden_ = true; }
            else { hidden_ = false; }

            //loop through the awnser and start bots
            for (std::size_t i = 0; i < bots; i++) {

                //grab pid and address result and push into vector
                pids_.push_back(StartBot(i));
                Sleep(1000);
            }

            std::cout << "\nBot starting loop has finished, this process will monitor and restart bots that get disconected until closed.\n";

        };

        //if anything goes wrong, toss the attempt and return 0
       DWORD AutoBot::StartBot(std::size_t index) {

            DWORD pid;

            auto cont = std::make_unique<Multicont>();

            if (!cont->RunMulticont()) { return 0; }
            else { pid = cont->GetPid(); }

             //grab path and access to Process
            std::string inject_path = marvin::GetWorkingDirectory() + "\\" + INJECT_MODULE_NAME;
            auto process = std::make_unique<marvin::Process>(pid);
            HANDLE handle = process->GetHandle();

            //find the menu handle by using the pid, or time out and return
            for (std::size_t trys = 0; ; trys++) {
                Sleep(100);
                //this function will assign the hwnd using the pid inserted
                if (!EnumWindows(FindMenu, pid)) { break; }
                if (trys > 100) {
                    TerminateProcess(handle, 0);
                    CloseHandle(handle);
                    return 0;
                }
            }
//#if 0
            //the code to open the profile menu
            if (hwnd::hmenu) { PostMessageA(hwnd::hmenu, WM_COMMAND, 40011, 0); }

            //wait for the profile handle
            for (std::size_t trys = 0; ; trys++) {
                Sleep(100);
               if (!EnumWindows(FindProfile, pid)) { break; }
                if (trys > 100) {
                    TerminateProcess(handle, 0);
                    CloseHandle(handle);
                    return 0;
                }
            }

             //grab the handle for the list box
            for (std::size_t trys = 0; ; trys++) {
                Sleep(200);
                if (hwnd::hlist = FindWindowExA(hwnd::hprofile, NULL, "ListBox", NULL)) { break; }
                if (trys > 100) {
                    TerminateProcess(handle, 0);
                    CloseHandle(handle);
                    return 0;
                }
            }
      
            //sets the profile
            PostMessageA(hwnd::hlist, LB_SETCURSEL, (index), 0);

            //close profile
            PostMessageA(hwnd::hprofile, WM_COMMAND, 1, 0);
//#endif
            //good method but wont set the default zone when changing profiles
            //process->SetProfile(handle, index);

            //Start Game, allow zone list to update first
            Sleep(2000);
            PostMessageA(hwnd::hmenu, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0); PostMessageA(hwnd::hmenu, WM_KEYUP, (WPARAM)(VK_RETURN), 0);       

            //wait for the game window to exist and grab the handle
            for (std::size_t trys = 0; ; trys++) {
                Sleep(1000);
                if (!EnumWindows(FindGame, pid)) { break; }
                else {

                    if (trys > 100) {
                        TerminateProcess(handle, 0);
                        CloseHandle(handle);
                        return 0;
                    }
   
                    if (!EnumWindows(FindError, pid)) {
                        PostMessage(hwnd::herror, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0); PostMessage(hwnd::herror, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
                        TerminateProcess(handle, 0);
                        CloseHandle(handle);
                        return 0;
                    }

                    if (!EnumWindows(FindInformation, pid)) {
                        PostMessage(hwnd::hinformation, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0); PostMessage(hwnd::hinformation, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
                        TerminateProcess(handle, 0);
                        CloseHandle(handle);
                        return 0;
                    }
                }
            }

            //wait for the client to log in by waiting for the chat address to return an enter message
            //this means the game a successfully connected
            std::string enter_msg = "";
            std::size_t module_base = process->GetModuleBase("Continuum.exe");

            for (std::size_t trys = 0; ; trys++) {
                Sleep(100);

                enter_msg = marvin::memory::ReadChatEntry(module_base, handle);
               
                //skip the log file message
                std::string log_file = "Log file open:";
                if (enter_msg.compare(0, 14, log_file) == 0) { continue; }

                if (enter_msg != "") { break; }

                if (!EnumWindows(FindError, pid)) {
                    PostMessage(hwnd::herror, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0); PostMessage(hwnd::herror, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
                    TerminateProcess(handle, 0);
                    CloseHandle(handle);
                    return 0;
                }
                //break from this loop if the loading screen hangs
                if (trys > 100) {
                    TerminateProcess(handle, 0);
                    CloseHandle(handle);
                    return 0;
                }
            }

            //need a second after getting chat or game will likely crash when trying to minimize or hide window
            Sleep(300);


            //inject the loader and marvin dll
            if (!process->InjectModule(inject_path)) {
                TerminateProcess(handle, 0);
                CloseHandle(handle);
                return 0;
            }

            //wait for windows title to change before minimizing, allow this to hang for map downloads
            for (std::size_t trys = 0; ; trys++) {
                Sleep(3000);

                if (!EnumWindows(FindInjectedTitle, pid)) { break; }

                if (trys > 100) {
                    TerminateProcess(handle, 0);
                    CloseHandle(handle);
                    return 0;
                }
            }

            //6 to minimize or 0 to hide the game
            if (hidden_ && hwnd::hgame) ShowWindow(hwnd::hgame, 0);
            else if (hwnd::hgame) { ShowWindow(hwnd::hgame, 6); }

            CloseHandle(handle);

            return pid;
        }


        void AutoBot::MonitorBots() {

                //cycle through each pid and see if it matches to an existing window
                for (std::size_t i = 0; i < pids_.size(); i++) {
                    Sleep(1000);

                    if (!EnumWindows(FindApplication, NULL)) {
                        PostMessage(hwnd::happlication, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0); PostMessage(hwnd::happlication, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
                    }

                    if (!EnumWindows(FindRunTimeError, NULL)) {
                        PostMessage(hwnd::hruntime, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0); PostMessage(hwnd::hruntime, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
                    }

                    //if the pid didnt return a handle, start a new bot
                    if (EnumWindows(FindInjectedTitle, pids_[i])) {
                        Sleep(1000);
                        pids_[i] = StartBot(i);
                    }
                }
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
            DWORD pid;
            GetWindowThreadProcessId(hwnd, &pid);
            char title[1024];
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

        BOOL __stdcall FindApplication(HWND hwnd, LPARAM lParam) {

            char title[1024];
            GetWindowTextA(hwnd, title, 1024);

            if (strcmp(title, "Continuum (enabled): Windows - Application Error") == 0) {
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

    } //namespace marvin