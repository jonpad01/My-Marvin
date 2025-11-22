#pragma once
#include <string>

#ifdef UNICODE
#undef UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

enum class Info {Running, Waiting};


class Pipe {
 public:
  Pipe(std::string pipe_name);
  ~Pipe() { 
	  DisconnectNamedPipe(handle);
	  CloseHandle(handle); 
  }
  
  bool WritePipe(std::string msg);
  std::string ReadPipe();

 private:
  HANDLE handle;
};

