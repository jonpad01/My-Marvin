#include "file.h"
#include "continuum.h"
#include "MemoryMap.h"
#include "pipe.h"
#include <iostream>
#include <fstream>

#ifdef UNICODE
#undef UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

std::vector<MarvinData> bot_data;

HANDLE StartMarvin(uint32_t index, const ProfileData& profile_data, const std::string& name) {

  MemoryMap memory_map;

  if (!memory_map.CreateMap()) {
    std::cout << "Failed to create memory map: " << GetLastError() << std::endl;
    std::cin.get();
  }

  memory_map.WriteU32(index);
  HANDLE handle = StartContinuum();

  std::cout << "Waiting for " << name << " to load..." << std::endl;

  // could be downloading map, so wait a long time
  if (!memory_map.WaitForData(100000)) {
    std::cout << "Timeout while waiting for: " << name << " to connect: " << GetLastError() << std::endl;
    TerminateProcess(handle, 0);
    CloseHandle(handle);
    return NULL;
  }

  std::cout << name << " loaded." << std::endl;

  return handle;
}


int main(int argc, char* argv[]) {
  std::ifstream continuum_exe(GetWorkingDirectory() + "\\Continuum.exe");
  std::ifstream dsound_dll(GetWorkingDirectory() + "\\dsound.dll");

  if (!continuum_exe.good() || !dsound_dll.good()) {
    continuum_exe.close();
    dsound_dll.close();
    std::cout << "Error: the fuse-launcher, marvin.dll, and dsound.dll need to be inside the continuum folder with continuum.exe";
    std::cin.get();
    return EXIT_FAILURE;
  }

  continuum_exe.close();
  dsound_dll.close();

  std::cout << "WARNING: This bot will overwrite your menu profile data." << std::endl;
  std::cout << "Close the window to exit." << std::endl;
  
  for (std::size_t i = 5; i >= 0 && i <= 5; i--) {
    Sleep(1000);
    std::cout << "Proceeding in: " << std::to_string(i) << std::endl;
  }

  RemoveMatchingFiles("Marvin-");
  CloseAllProcess("Continuum.exe");  // make sure continuum is not running

  bot_data = LoadProfileData();

  if (bot_data.empty()) {
    std::cout << "Error: profile data was not found, you need at least one profile." << std::endl;
    std::cin.get();
    return EXIT_FAILURE;
  }

  for (std::size_t i = 0; i < bot_data.size(); i++) {
    bot_data[i].handle = StartMarvin((uint32_t)i, bot_data[i].profile_data, bot_data[i].name);
  }

  while (true) {
    for (std::size_t i = 0; i < bot_data.size(); i++) {
      Sleep(3000);

      ULONG exitcode;
      bool found = GetExitCodeProcess(bot_data[i].handle, &exitcode);

      if (!found) {
        std::cout << "Exit code not found for: " << bot_data[i].name << std::endl;
      }

      if (exitcode != STILL_ACTIVE || !found) {
        std::cout << "Bot has closed: " << bot_data[i].name << std::endl;
        CloseHandle(bot_data[i].handle);
        bot_data[i].handle = StartMarvin((uint32_t)i, bot_data[i].profile_data, bot_data[i].name);
      }
    }
  }

  return EXIT_SUCCESS;
}