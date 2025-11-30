#pragma once

#ifdef UNICODE
#undef UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <versionhelpers.h>

#include <string>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef GetMessage
#undef GetMessage
#endif

inline std::string GetWorkingDirectory() {
  std::string directory;

  directory.resize(GetCurrentDirectoryA(0, NULL));

  GetCurrentDirectoryA(directory.size(), &directory[0]);

  return directory.substr(0, directory.size() - 1);
}
