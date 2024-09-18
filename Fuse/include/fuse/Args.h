#pragma once

#include <fuse/Platform.h>
#include <shellapi.h>

#include <string>
#include <vector>

namespace fuse {

using ArgList = std::vector<std::string>;

inline ArgList GetArguments() {
  ArgList result;

  int argv = 0;
  LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &argv);

  for (int i = 0; i < argv; ++i) {
    size_t size = wcslen(args[0]);

    std::string str;
    str.resize(size);

    wcstombs(&str[0], args[i], size);
    result.push_back(std::move(str));
  }

  return result;
}

}  // namespace fuse
