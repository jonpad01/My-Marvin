#include "continuum.h"
#include "file.h"
#include <iostream>

#include <TlHelp32.h>


/* 
* Continuum relaunches itself to apply a DACL, and exits the parent process
* This method creates a job object then adds the parent process to the job
* Then queries the pid of the child process spawned by the parent process
*/
HANDLE StartContinuum() {
  HANDLE hChild = NULL;
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(STARTUPINFO);
  HANDLE hJob = CreateJobObjectA(nullptr, nullptr);
  
  if (!hJob) {
    std::cout << "CreateJobObject failed: " << GetLastError() << "\n";
    return 0;
  }

  #if 0
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
  jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_BREAKAWAY_OK; 

  if (!SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli))) {
    std::cout << "SetInformationJobObject failed: " << GetLastError() << "\n";
    return 0;
  }
  #endif

  std::string path = GetWorkingDirectory() + "\\Continuum.exe";
  LPSTR szCmdline = _strdup(path.c_str());

  if (!CreateProcessA(NULL, szCmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    std::cout << "Error: Could not start Continuum." << std::endl;
    free(szCmdline);
    return 0;
  }

  free(szCmdline);

  if (!AssignProcessToJobObject(hJob, pi.hProcess)) {
    std::cout << "AssignProcessToJobObject failed: " << GetLastError() << "\n";
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
  }

  // the handle hProcess and hThread signals as soon as continuum is launched.
  WaitForSingleObject(pi.hProcess, INFINITE);  
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  DWORD bufferSize = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + sizeof(ULONG_PTR) * 32;
  std::vector<BYTE> buffer(bufferSize);

  JOBOBJECT_BASIC_PROCESS_ID_LIST* pidList = (JOBOBJECT_BASIC_PROCESS_ID_LIST*)buffer.data();

  BOOL gotList = QueryInformationJobObject(hJob, JobObjectBasicProcessIdList, pidList, bufferSize, &bufferSize);

  if (!gotList) {
    std::cout << "QueryInformationJobObject failed: " << GetLastError() << "\n";
    return 0;
  }

  // there should only be one process in this list
  for (std::size_t i = 0; i < pidList->NumberOfProcessIdsInList; i++) {
    DWORD pid = (DWORD)pidList->ProcessIdList[i];

    // This is the real Continuum game process.
    hChild = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, pid);
    if (!hChild) {
      std::cout << "Failed to open child process: " << GetLastError() << "\n";
    }
  }

  CloseHandle(hJob);

  return hChild;
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
  Might not work correctly when running multiple bots
  */
  do {
    if (strcmp(pe32.szExeFile, "Continuum.exe") == 0) {
      pid = pe32.th32ProcessID;
    }
  } while (Process32Next(hProcSnap, &pe32));

  pHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, pid);

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
