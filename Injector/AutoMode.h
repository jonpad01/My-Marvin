#pragma once

#include "injector.h"

struct BotInfo {
    DWORD pid;
    size_t address;
    HANDLE handle;
};

void AutoMode();

DWORD StartBot(std::size_t index, bool hide_window);

std::vector<DWORD> StartBots(std::size_t bots, bool hide_window);

void MonitorBots(std::vector<DWORD> pids, bool hide_window);

std::string ReadChatEntry(uint32_t module_base, HANDLE handle);

namespace menu {
    BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam);
}
namespace profile {
    BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam);
}
namespace information {
    BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam);
}
namespace game {
    BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam);
}
namespace injected {
    BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam);
}
namespace error {
    BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam);
}
namespace application {
    BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam);
}