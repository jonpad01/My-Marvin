#include "pipe.h"
#include <iostream>

Pipe::Pipe(std::string pipe_name) {

    std::string name = "\\\\.\\pipe\\" + pipe_name;


  handle = CreateNamedPipeA("\\\\.\\pipe\\marvinbots", PIPE_ACCESS_DUPLEX,
                                  PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 4096, 4096, 0, nullptr);
 
  if (handle == INVALID_HANDLE_VALUE) {
    std::cout << "Error: Pipe returned invalid handle." << std::endl;
    return;
  }

 // BOOL ok = ConnectNamedPipe(handle, NULL) || GetLastError() == ERROR_PIPE_CONNECTED;

 // if (!ok) {
 //   CloseHandle(handle);
//    std::cout << "Error: Could not connect pipe." << std::endl;
//  } else {
 //   std::cout << "Pipe Created." << std::endl;
 // }
}

bool Pipe::WritePipe(std::string msg) {

  DWORD written = 0;
  bool result = WriteFile(handle, msg.c_str(), (DWORD)msg.size(), &written, NULL);

  return result;
}

std::string Pipe::ReadPipe() {
  char buf[1024];
  DWORD read = 0;
  bool result = ReadFile(handle, buf, sizeof(buf) - 1, &read, NULL);
  buf[read] = 0;

  return std::string(buf);
}