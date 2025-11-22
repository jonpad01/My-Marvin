#include "continuum.h"
#include "file.h"
#include <iostream>

#include <TlHelp32.h>
//#include <sddl.h>
//#include <stdint.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <tchar.h>



HANDLE StartContinuum() {
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(STARTUPINFO);

  std::string path = GetWorkingDirectory() + "\\Continuum.exe";
  LPSTR szCmdline = _strdup(path.c_str());

  if (!CreateProcessA(NULL, szCmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    std::cout << "Error: Could not start Continuum." << std::endl;
    return 0;
  }

  // the handle hProcess and hThread signals as soon as continuum is launched.
  // GetExitCodeProcess says its not active so this handle can be closed. 
  // probably smart to still wait for it to signal.
  WaitForSingleObject(pi.hProcess, INFINITE);  
  free(szCmdline);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  return GetProcessHandle();  // find the real handle and return it
}

HANDLE GetProcessHandle() {
  HANDLE pHandle = NULL;
  PROCESSENTRY32 pe32;
  HANDLE hProcSnap;
  DWORD pid = 0;

  hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

  if (hProcSnap == INVALID_HANDLE_VALUE) return NULL;

  pe32.dwSize = sizeof(PROCESSENTRY32);

  if (!Process32First(hProcSnap, &pe32)) {
    CloseHandle(hProcSnap);
    return NULL;
  }

  /* 
  Loop through running processes to find continuum 
  Seems to work well if called directly after the process is created
  */
  do {
    if (strcmp(pe32.szExeFile, "Continuum.exe") == 0) {
      pid = pe32.th32ProcessID;
    }
  } while (Process32Next(hProcSnap, &pe32));

  pHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);

  return pHandle;
}

void CloseAllProcess(const std::string& name) {
  PROCESSENTRY32 pe32;
  HANDLE hProcSnap;

  hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

  if (hProcSnap == INVALID_HANDLE_VALUE) return;

  pe32.dwSize = sizeof(PROCESSENTRY32);

  if (!Process32First(hProcSnap, &pe32)) {
    CloseHandle(hProcSnap);
    return;
  }

  /* Loop through running processes to find continuum */
  do {
    if (strcmp(pe32.szExeFile, name.c_str()) == 0) {
      HANDLE handle = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
      TerminateProcess(handle, 0);
      CloseHandle(handle);
    }
  } while (Process32Next(hProcSnap, &pe32));
}
