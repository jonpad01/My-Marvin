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

    class AutoBot {

    public:

        AutoBot();
        ~AutoBot() {
            for (std::size_t i = 0; i < handles_.size(); i++) {
                if (handles_[i]) { CloseHandle(handles_[i]); }
            }
        }

        DWORD StartBot(std::size_t index);
        void MonitorBots();

    private:

        std::vector<DWORD> pids_;
        std::vector<HANDLE> handles_;
        std::vector<std::size_t> addrs_;

        bool hidden_;
    };

    BOOL __stdcall FindMenu(HWND hwnd, LPARAM lParam);
    BOOL __stdcall FindProfile(HWND hwnd, LPARAM lParam);
    BOOL __stdcall FindInformation(HWND hwnd, LPARAM lParam);
    BOOL __stdcall FindGame(HWND hwnd, LPARAM lParam);
    BOOL __stdcall FindInjectedTitle(HWND hwnd, LPARAM lParam);
    BOOL __stdcall FindError(HWND hwnd, LPARAM lParam);
    BOOL __stdcall FindApplication(HWND hwnd, LPARAM lParam);
    BOOL __stdcall FindRunTimeError(HWND hwnd, LPARAM lParam);

}






