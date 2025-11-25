#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#include "Multicont.h"
#include <TlHelp32.h>
#include <sddl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#ifndef NTAPI
#define NTAPI __stdcall
#endif

#ifndef NTSTATUS
#define NTSTATUS LONG
#endif

#define STATUS_INFO_LENGTH_MISMATCH 0xC0000004

#define SystemHandleInformation 16
#define ObjectBasicInformation 0
#define ObjectNameInformation 1
#define ObjectTypeInformation 2

#define NT_SUCCESS(x) ((x) >= 0)

// this starts a continuum process and uses a mutex to bypass the limit on how many clients can be opened at once
// the injector has to give itself debug priveleges to work, but continuum cant inherit it when it is created (crashes)
// make a duplicate token with debug removed and then use it to launch continuum

namespace marvin {

bool Multicont::RunMulticont() {
  _NtQuerySystemInformation NtQuerySystemInformation = NULL;
  _NtDuplicateObject NtDuplicateObject = NULL;
  _NtQueryObject NtQueryObject = NULL;

  PSYSTEM_HANDLE_INFORMATION handleInfo;

  ULONG handleInfoSize = 0x10000;
  LONG status = 0;
  HANDLE hMutex = NULL, token_cur = NULL, token_dup = NULL, hProcess = NULL;

  GetLibraryAddresses(&NtQuerySystemInformation, &NtDuplicateObject, &NtQueryObject);

  /* Get the current process' security token as a starting point, then modify
     a duplicate so that it runs with a fixed integrity level. */

  if (OpenProcessToken(GetCurrentProcess(),
          TOKEN_DUPLICATE | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY,
          &token_cur)) {
    if (DuplicateTokenEx(token_cur, 0, NULL, SecurityImpersonation, TokenPrimary, &token_dup)) {
      if (RemoveDebugPrivileges(token_dup)) {
        PROCESS_INFORMATION pi;
        STARTUPINFO si;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(STARTUPINFO);
        // files paths for windows x86 compatibility
        if (CreateProcessAsUserW(token_dup, TEXT("C:\\Program Files (x86)\\Continuum\\Continuum.exe"), NULL, NULL, NULL,
                                 FALSE, 0, NULL, NULL, &si, &pi) ||
            CreateProcessAsUserW(token_dup, TEXT("C:\\Program Files\\Continuum\\Continuum.exe"), NULL, NULL, NULL,
                                 FALSE, 0, NULL, NULL, &si, &pi)) {
          // Process has been created; work with the process and wait for it to terminate
          // the handles and pid this return is all junk, jsut close the handles.
          WaitForSingleObject(pi.hProcess, INFINITE);
          CloseHandle(pi.hThread);
          CloseHandle(pi.hProcess);
        }
      }
      CloseHandle(token_dup);
    }
    CloseHandle(token_cur);
  }

  /* Sleep while waiting for mutex to be created */
  for (size_t trys = 0;; trys++) {
    Sleep(10);
    if (hMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, L"mtxsscont"))
      break;
    else if (trys > 1000) {
      return false;
    }
  }

  CloseHandle(hMutex);

  char text[] = "Continuum.exe";
  wchar_t wtext[20];
  mbstowcs_s(NULL, wtext, text, strlen(text) + 1);  // Plus null
  LPWSTR ptr = wtext;

  /* Get process handle for respawned process */
  if (!(hProcess = GetProcessHandle())) {
    return false;
  }

  handleInfo = (PSYSTEM_HANDLE_INFORMATION)malloc(handleInfoSize);

  /* Get all of the system's handles */
  while ((status = NtQuerySystemInformation(16, handleInfo, handleInfoSize, NULL)) == 0xC0000004) {
    handleInfo = (PSYSTEM_HANDLE_INFORMATION)realloc(handleInfo, handleInfoSize *= 2);
  }

  if (!NT_SUCCESS(status)) {
    return false;
  }

  /* Loop through the handles */
  for (ULONG i = 0; i < handleInfo->HandleCount; i++) {
    SYSTEM_HANDLE handle = handleInfo->Handles[i];
    POBJECT_TYPE_INFORMATION objectTypeInfo;
    UNICODE_STRING objectName;
    HANDLE dupHandle = NULL;
    PVOID objectNameInfo;
    ULONG returnLength;
    uintptr_t pHandle = (uintptr_t)handle.Handle;

    if (handle.ProcessId != process_id_) continue;

    /* Duplicate the handle so we can query it. */
    if (!NT_SUCCESS(NtDuplicateObject(hProcess, (HANDLE)pHandle, GetCurrentProcess(), &dupHandle, 0, 0, 0))) continue;

    /* Query the object type. */
    objectTypeInfo = (POBJECT_TYPE_INFORMATION)malloc(0x1000);
    if (!NT_SUCCESS(NtQueryObject(dupHandle, 2, objectTypeInfo, 0x1000, NULL))) {
      CloseHandle(dupHandle);
      continue;
    }

    /* Skip handles with 0x0012019F flag (could hang)*/
    if (handle.GrantedAccess == 0x0012019f) {
      free(objectTypeInfo);
      CloseHandle(dupHandle);
      continue;
    }

    objectNameInfo = malloc(0x1000);

    /* Query object name */
    if (!NT_SUCCESS(NtQueryObject(dupHandle, 1, objectNameInfo, 0x1000, &returnLength))) {
      objectNameInfo = realloc(objectNameInfo, returnLength);

      if (!NT_SUCCESS(NtQueryObject(dupHandle, 1, objectNameInfo, returnLength, NULL))) {
        free(objectTypeInfo);
        free(objectNameInfo);
        CloseHandle(dupHandle);
        continue;
      }
    }

    objectName = *(PUNICODE_STRING)objectNameInfo;

    if (objectName.Length) {
      /* Close the mutex if its name is MUTEXNAME */
      if (wcsstr(objectName.Buffer, L"mtxsscont") != NULL) {
        DuplicateHandle(hProcess, (HANDLE)pHandle, NULL, NULL, 0, FALSE, 1);
      }
    }

    free(objectTypeInfo);
    free(objectNameInfo);
    CloseHandle(dupHandle);
  }

  free(handleInfo);
  CloseHandle(hProcess);

  return true;
}

HANDLE Multicont::GetProcessHandle() {
  HANDLE pHandle = NULL;
  PROCESSENTRY32 pe32;
  HANDLE hProcSnap;

  hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

  if (hProcSnap == INVALID_HANDLE_VALUE) return NULL;

  pe32.dwSize = sizeof(PROCESSENTRY32);

  if (!Process32First(hProcSnap, &pe32)) {
    CloseHandle(hProcSnap);
    return NULL;
  }

  /* Loop through running processes to find continuum */
  do {
    if (wcscmp(pe32.szExeFile, L"Continuum.exe") == 0) {
      process_id_ = pe32.th32ProcessID;
    }
  } while (Process32Next(hProcSnap, &pe32));

  pHandle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, process_id_);

  return pHandle;
}

bool Multicont::RemoveDebugPrivileges(HANDLE token) {
  bool success = false;

  TOKEN_PRIVILEGES privileges;

  LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &privileges.Privileges[0].Luid);
  privileges.PrivilegeCount = 1;
  privileges.Privileges[0].Attributes = SE_PRIVILEGE_REMOVED;

  if (AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(TOKEN_PRIVILEGES), 0, 0)) success = true;

  return success;
}

PVOID Multicont::GetLibraryProcAddress(LPSTR LibraryName, LPSTR ProcName) {
  return GetProcAddress(GetModuleHandleA(LibraryName), ProcName);
}

void Multicont::GetLibraryAddresses(_NtQuerySystemInformation* ntqsi, _NtDuplicateObject* ntdo, _NtQueryObject* ntqo) {
  const char* ntdll = "ntdll.dll";
  LPSTR ntdll_ = const_cast<LPSTR>(ntdll);

  const char* query = "NtQuerySystemInformation";
  LPSTR query_ = const_cast<LPSTR>(query);

  const char* duplicate = "NtDuplicateObject";
  LPSTR duplicate_ = const_cast<LPSTR>(duplicate);

  const char* object = "NtQueryObject";
  LPSTR object_ = const_cast<LPSTR>(object);

  *ntqsi = (_NtQuerySystemInformation)GetLibraryProcAddress(ntdll_, query_);
  *ntdo = (_NtDuplicateObject)GetLibraryProcAddress(ntdll_, duplicate_);
  *ntqo = (_NtQueryObject)GetLibraryProcAddress(ntdll_, object_);
}

}  // namespace marvin
