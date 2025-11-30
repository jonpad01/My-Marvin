#include "pipe.h"
#include <iostream>

Pipe::Pipe(const std::string& pipe_name) {

    std::string name = "\\\\.\\pipe\\" + pipe_name;


  hPipe = CreateNamedPipeA(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                  PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 1, 4096, 4096, 0, nullptr);
 
  if (hPipe == INVALID_HANDLE_VALUE) {
    std::cout << "Failed to create named pipe: " << GetLastError() << std::endl;
  }

  DWORD timeoutMs = 5000;
  SetNamedPipeHandleState(hPipe, nullptr, &timeoutMs, nullptr);
}

bool Pipe::WritePipe(const ProfileData& data) {
  OVERLAPPED ov = {};
  ov.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

  DWORD written = 0;
  bool ok = WriteFile(hPipe, &data, sizeof(data), &written, NULL);

    if (!ok && GetLastError() == ERROR_IO_PENDING) {
    // wait max 10 seconds
    DWORD wait = WaitForSingleObject(ov.hEvent, 10000);

    if (wait == WAIT_TIMEOUT) {
      CancelIo(hPipe);
      CloseHandle(hPipe);
      return false;
    }
  }

  return true;
}

bool Pipe::WaitForClient() {
  OVERLAPPED ov = {};
  ov.hEvent = CreateEventA(NULL, TRUE, FALSE, NULL);

  BOOL ok = ConnectNamedPipe(hPipe, &ov);
  //BOOL ok = ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED;

  if (!ok && GetLastError() == ERROR_IO_PENDING) {
    // wait max 10 seconds
    DWORD wait = WaitForSingleObject(ov.hEvent, 10000);

    if (wait == WAIT_TIMEOUT) {
      CancelIo(hPipe);
      CloseHandle(hPipe);
      return false;
    }
  }

  // if (!ok) {
  //   std::cout << "Failed to connect pipe: " << GetLastError() << std::endl;
  //   std::cin.get();
   //  return false;
  // } 

  return true;
}

std::string Pipe::ReadStringFromPipe() {
  char buf[1024];
  DWORD read = 0;
  bool result = ReadFile(hPipe, buf, sizeof(buf) - 1, &read, NULL);
  buf[read] = 0;

  return std::string(buf);
}

int Pipe::ReadIntFromPipe() {
  int data;
  DWORD read = 0;

  DWORD wait = WaitForSingleObject(hPipe, 5000);

  int result = ReadFile(hPipe, &data, sizeof(data), &read, NULL);

  if (!result) data = 0;

  return data;
}