#pragma once
#include <vector>
#include <string>

#ifdef UNICODE
#undef UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct MarvinData {
  ~MarvinData() { CloseHandle(handle); }

  std::vector<std::string> profile_data;
  HANDLE handle = NULL;
  std::string name;
};

std::string GetWorkingDirectory();
void RemoveMatchingFiles(const std::string& substring);
std::vector<MarvinData> LoadProfileData();
bool WriteToFile(const std::vector<std::string>& data, const std::string& file_name);