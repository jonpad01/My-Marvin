#pragma once
#include "file.h"
#include <string>

#ifdef UNICODE
#undef UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

enum class Info {Running, Waiting};


class Pipe {
 public:
  Pipe(const std::string& pipe_name);
  ~Pipe() { 
	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe); 
  }

  bool WaitForClient();
  
  bool WritePipe(const ProfileData& data);
  int ReadIntFromPipe();
  std::string ReadStringFromPipe();

 private:
  HANDLE hPipe;
};

