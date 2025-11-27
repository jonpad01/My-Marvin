#include "file.h"
#include "continuum.h"
#include "pipe.h"
#include <iostream>
#include <fstream>

#ifdef UNICODE
#undef UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

std::vector<MarvinData> bot_data;

HANDLE StartMarvin(const std::vector<std::string>& profile_data, const std::string& name) {
  WriteToFile(profile_data, GetWorkingDirectory() + "\\MarvinData.txt");
  HANDLE handle = StartContinuum();

  std::cout << "Waiting for " << name << " to load..." << std::endl;
  std::size_t i = 0;

  for (i = 0; i < 10; i++) {
    Sleep(2500);
    std::ifstream file("MarvinData.txt");

    if (!file.is_open()) continue;

    std::string buffer;
    getline(file, buffer);
    file.close();

    if (buffer == "OK-" + name) {
      std::cout << name << " loaded." << std::endl;
      break;
    }
  }

  // something failed
  if (i >= 10) {
    TerminateProcess(handle, 0);
    CloseHandle(handle);
    handle = NULL;
    std::cout << "Timeout while waiting for: " << name << " to login." << std::endl;
  }

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
    bot_data[i].handle = StartMarvin(bot_data[i].profile_data, bot_data[i].name);
  }

  while (true) {
    for (std::size_t i = 0; i < bot_data.size(); i++) {
      Sleep(3000);

      ULONG exitcode;
      bool found = GetExitCodeProcess(bot_data[i].handle, &exitcode);

      if (!found) {
        std::cout << "Exit code not found for: " << bot_data[i].name << std::endl;
      }

      if (exitcode != STILL_ACTIVE) {
        std::cout << "Bot has closed: " << bot_data[i].name << std::endl;
        CloseHandle(bot_data[i].handle);
        bot_data[i].handle = StartMarvin(bot_data[i].profile_data, bot_data[i].name);
      }
    }
  }

  return EXIT_SUCCESS;
}