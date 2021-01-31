#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

/*
  Injector injects the Loader.dll into the Continuum process.
  The loader is the one that loads the Marvin.dll so it can hot swap it.
*/

#ifdef UNICODE
#undef UNICODE
#endif
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <stdio.h>
#include <TlHelp32.h>

#define INJECT_MODULE_NAME "Loader.dll"





DWORD SelectPid(const std::vector<DWORD>& pids, std::string target_player);

DWORD SelectPid(const std::vector<DWORD>& pids);

void InjectExcept(const std::vector<DWORD>& pids, const std::string& exception);


//int main(int argc, char* argv[]);