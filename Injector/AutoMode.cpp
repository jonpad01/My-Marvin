#include "AutoMode.h"

HWND hmenu = NULL;
HWND hprofile = NULL;
HWND hgame = NULL;
HWND hinjected = NULL;
HWND hinformation = NULL;
HWND herror = NULL;
HWND happlication = NULL;

void AutoMode() {

    std::vector<DWORD> pids;
    
    std::size_t bots = 0;
    std::string hide_window;
    bool hide_windows;

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

    if (hide_window == "y") hide_windows = true;
    else hide_windows = false;

    //loop through the awnser and start bots
    for (int i = 0; i < bots; i++) {

        //start bot with index i
        DWORD pid = StartBot(i, hide_windows);

        //grab pid and address result and push into vector
        pids.push_back(pid);
        Sleep(100);
    }

    std::cout << "\nBot starting loop has finished, this process will monitor and restart bots that get disconected until closed.\n";

    Sleep(5000);
    //if it makes it this far this function never exits as long as the window remains open
    MonitorBots(pids, hide_windows);
}


    DWORD StartBot(std::size_t index, bool hide_window) {

        hmenu = NULL;
        hprofile = NULL;
        hgame = NULL;
        hinformation = NULL;
        HWND hlist = NULL;

        DWORD pid = 0;
        ProcessResults result = { 0 };

        result = RunMulticont();
        //if multicont fails return an invalid pid so it can retry later
        if (result.success == 0) return 0;
        
        //return pid to caller
        pid = result.pid;

        //find the menus handle by using the pid
        while (hmenu == NULL) {
            Sleep(100);
            //this function will assign the hwnd using the pid inserted
            EnumWindows(menu::MyEnumWindowsProc, pid);
        }

        if (hide_window) ShowWindow(hmenu, 0);

        //the code to open the profile menu
        PostMessageA(hmenu, WM_COMMAND, 40011, 0);

        //find the profile menu handle
        while (hprofile == NULL) {
            Sleep(100);
            EnumWindows(profile::MyEnumWindowsProc, pid);
        }

        if (hide_window) ShowWindow(hprofile, 0);

        //grab the handle for the list box
        hlist = FindWindowExA(hprofile, NULL, "ListBox", NULL);

        //send the selector to the top
        PostMessage(hlist, WM_KEYDOWN, (WPARAM)VK_PRIOR, 0); PostMessage(hlist, WM_KEYUP, (WPARAM)VK_PRIOR, 0);
        PostMessage(hlist, WM_KEYDOWN, (WPARAM)VK_PRIOR, 0); PostMessage(hlist, WM_KEYUP, (WPARAM)VK_PRIOR, 0);
        PostMessage(hlist, WM_KEYDOWN, (WPARAM)VK_PRIOR, 0); PostMessage(hlist, WM_KEYUP, (WPARAM)VK_PRIOR, 0);
        PostMessage(hlist, WM_KEYDOWN, (WPARAM)VK_PRIOR, 0); PostMessage(hlist, WM_KEYUP, (WPARAM)VK_PRIOR, 0);
        PostMessage(hlist, WM_KEYDOWN, (WPARAM)VK_PRIOR, 0); PostMessage(hlist, WM_KEYUP, (WPARAM)VK_PRIOR, 0);

        //press down key to select a player profile
        for (std::size_t i = 0; i < index; i++) {
            Sleep(10);
            PostMessage(hlist, WM_KEYDOWN, (WPARAM)VK_DOWN, 0); PostMessage(hlist, WM_KEYUP, (WPARAM)VK_DOWN, 0);
        }

        //close profile
        PostMessage(hlist, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0); PostMessage(hlist, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
        
        //Start Game
        PostMessage(hmenu, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0); PostMessage(hmenu, WM_KEYUP, (WPARAM)(VK_RETURN), 0);

        //wait for the game window to exist and grab the handle
        //if it timed out close informationa and try again
        while (hgame == NULL) {
            Sleep(100);
            EnumWindows(game::MyEnumWindowsProc, pid);
            EnumWindows(information::MyEnumWindowsProc, pid);
            EnumWindows(error::MyEnumWindowsProc, pid);

            if (herror != NULL) {
                PostMessage(herror, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0); PostMessage(herror, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
                herror = NULL;
                return 0;
            }

            if (hinformation != NULL) {
                PostMessage(hinformation, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0); PostMessage(hinformation, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
                Sleep(100);
                PostMessage(hmenu, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0); PostMessage(hmenu, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
            }
            hinformation = NULL;   
        } 

       //grab path and access to Process
        std::string inject_path = marvin::GetWorkingDirectory() + "\\" + INJECT_MODULE_NAME;
        auto process = std::make_unique<marvin::Process>(pid);
        HANDLE handle = process->GetHandle();

        //wait for the client to log in by waiting for the chat address to return an enter message
        std::string enter_msg = "";

        while (enter_msg == "") {
            Sleep(100);
            std::size_t module_base = 0;
            while (module_base == 0) {
                Sleep(100);
                module_base = process->GetModuleBase("Continuum.exe");
            }
            enter_msg = ReadChatEntry(module_base, handle);


            EnumWindows(error::MyEnumWindowsProc, pid);
            if (herror != NULL) {
                PostMessage(herror, WM_KEYDOWN, (WPARAM)(VK_RETURN), 0); PostMessage(herror, WM_KEYUP, (WPARAM)(VK_RETURN), 0);
                herror = NULL;
                return 0;
            }
            
        }

     //need a second after getting chat or game will likely crash when trying to minimize or hide window
        Sleep(1000);

        //6 to minimize or 0 to hide the game
        if (hide_window) ShowWindow(hgame, 0);
        else ShowWindow(hgame, 6);

        Sleep(200);
        //inject the loader and marvin dll
        if (!process->InjectModule(inject_path)) TerminateProcess(handle, 0);
    
        CloseHandle(handle);

        return pid;
    }

void MonitorBots(std::vector<DWORD> pids, bool hide_window) {
    
    //continuous non stop party time loop
    while (true) {
        Sleep(1000);

        //cycle through each pid and see if it matches to an existing window
        for (std::size_t i = 0; i < pids.size(); i++) {
            Sleep(10);
            EnumWindows(injected::MyEnumWindowsProc, pids[i]);     
            EnumWindows(game::MyEnumWindowsProc, pids[i]);
            EnumWindows(application::MyEnumWindowsProc, pids[i]);

            if (happlication != NULL) {
                auto process = std::make_unique<marvin::Process>(pids[i]);
                HANDLE handle = process->GetHandle();
                TerminateProcess(handle, 0);
                CloseHandle(handle);
            }
            happlication = NULL;

            //if the pid didnt return a handle, start a new bot
            if (hinjected == NULL) {
               
                //if injector failed to inject bot just kill the process
                
                if (hgame != NULL) {
                    auto process = std::make_unique<marvin::Process>(pids[i]);
                    HANDLE handle = process->GetHandle();
                    TerminateProcess(handle, 0);
                    CloseHandle(handle);
                }
                hgame = NULL;

                DWORD pid = StartBot(i, hide_window);

                pids[i] = pid;
            }
        }
    }
}

namespace profile {
    BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam) {
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);

        if (pid == (DWORD)lParam) {
            char title[1024];
            GetWindowTextA(hwnd, title, 1024);

            if (strcmp(title, "Select/Edit Profile") == 0) {
                hprofile = hwnd;
                return FALSE;
            }
        }
        return TRUE;
    }
}

namespace menu {
    BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam) {
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);

        if (pid == (DWORD)lParam) {
            char title[1024];
            GetWindowTextA(hwnd, title, 1024);

            if (strcmp(title, "Continuum 0.40") == 0) {
                hmenu = hwnd;
                return FALSE;
            }
        }
        return TRUE;
    }
}

namespace information {
    BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam) {
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);

        if (pid == (DWORD)lParam) {
            char title[1024];
            GetWindowTextA(hwnd, title, 1024);
            
            if (strcmp(title, "Information") == 0) {
                hinformation = hwnd;
                return FALSE;
            }   
        }
        return TRUE;
    }
}

namespace error {
    BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam) {
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);

        if (pid == (DWORD)lParam) {
            char title[1024];
            GetWindowTextA(hwnd, title, 1024);

            if (strcmp(title, "Error") == 0) {
                herror = hwnd;
                return FALSE;
            }
        }
        return TRUE;
    }
}

namespace game {
    BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam) {
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);

        if (pid == (DWORD)lParam) {
            char title[1024];
            GetWindowTextA(hwnd, title, 1024);

            if (strcmp(title, "Continuum") == 0) {
                hgame = hwnd;
                return FALSE;
            }
        }
        hgame = NULL;
        return TRUE;
    }
}

namespace injected {
    BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam) {
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);

        if (pid == (DWORD)lParam) {
            char title[1024];
            GetWindowTextA(hwnd, title, 1024);

            if (strcmp(title, "Continuum (enabled)") == 0 || strcmp(title, "Continuum (disabled)") == 0) {
                hinjected = hwnd;
                return FALSE;
            }
        }
        hinjected = NULL;
        return TRUE;
    }
}

namespace application {
    BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam) {
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);

        if (pid == (DWORD)lParam) {
            char title[1024];
            GetWindowTextA(hwnd, title, 1024);

            if (strcmp(title, "Continuum (enabled): Windows - Application Error") == 0) {
                happlication = hwnd;
                return FALSE;
            }
        }
        happlication = NULL;
        return TRUE;
    }
}

std::string ReadChatEntry(uint32_t module_base, HANDLE handle) {

    //uint32_t module_base = process.GetModuleBase("Continuum.exe");
    uint32_t game_addr = marvin::memory::ReadU32(handle, module_base + 0xC1AFC);
    uint32_t entry_ptr_addr = game_addr + 0x2DD08;
    uint32_t entry_count_addr = entry_ptr_addr + 8;

    uint32_t entry_ptr = marvin::memory::ReadU32(handle, entry_ptr_addr);
    uint32_t entry_count = 0;

    // Client isn't in game
    if (entry_ptr == 0) return "";

    ReadProcessMemory(handle, (LPVOID)entry_count_addr, &entry_count, sizeof(uint32_t), nullptr);

    uint32_t last_entry_addr = entry_ptr + (entry_count - 1) * sizeof(ChatEntry);
    ChatEntry entry;

    ReadProcessMemory(handle, (LPVOID)last_entry_addr, &entry, sizeof(ChatEntry), nullptr);

    //printf("%s> %s\n", entry.player, entry.message);

    return entry.message;
}
