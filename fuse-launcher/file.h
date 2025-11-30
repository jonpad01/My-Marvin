#pragma once
#include <vector>
#include <string>

#ifdef UNICODE
#undef UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct ProfileData {
  char name[50];
  char password[50];
  char zone_name[50];
  char chats[300];
  short ship = 0;
  short window_mode = 0;
};

struct MarvinData {
  ~MarvinData() { CloseHandle(handle); }

  HANDLE handle = NULL;
  std::string name;
  ProfileData profile_data;
};

std::string GetWorkingDirectory();
void RemoveMatchingFiles(const std::string& substring);
std::vector<MarvinData> LoadProfileData();
bool WriteToFile(const std::vector<std::string>& data, const std::string& file_name);