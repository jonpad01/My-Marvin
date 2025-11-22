#pragma once
#include <string>

#ifdef UNICODE
#undef UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

HANDLE StartContinuum();
HANDLE GetProcessHandle();
void CloseAllProcess(const std::string& name);