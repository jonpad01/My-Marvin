
#include <tchar.h>
#include <windows.h>
#include <sddl.h>
#include <stdio.h>
#include <stdint.h>
#include <TlHelp32.h>

#include "Multicont.h"

#define NT_SUCCESS(x) ((x) >= 0)


bool RunMulticont(bool hide_window, DWORD *pid) {   

    _NtQuerySystemInformation NtQuerySystemInformation = (_NtQuerySystemInformation)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQuerySystemInformation");
    _NtDuplicateObject NtDuplicateObject = (_NtDuplicateObject)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtDuplicateObject");
    _NtQueryObject NtQueryObject = (_NtQueryObject)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryObject");
    
    PSYSTEM_HANDLE_INFORMATION handleInfo;
  
    ULONG handleInfoSize = 0x10000;
    LONG status;
    HANDLE hMutex, token_cur, token_dup, hProcess;

    /* Get the current process' security token as a starting point, then modify
       a duplicate so that it runs with a fixed integrity level. */

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_ADJUST_DEFAULT | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY, &token_cur)) {
        
        if (DuplicateTokenEx(token_cur, 0, NULL, SecurityImpersonation, TokenPrimary, &token_dup)) {
            if (RemoveDebugPrivileges(token_dup)) {
                
                PROCESS_INFORMATION pi;
                STARTUPINFO si;

                ZeroMemory(&si, sizeof(si));
                si.cb = sizeof(STARTUPINFO);
                //files paths for windows x86 compatibility
                if (CreateProcessAsUserW(token_dup, TEXT("C:\\Program Files (x86)\\Continuum\\Continuum.exe"), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi) ||
                    CreateProcessAsUserW(token_dup, TEXT("C:\\Program Files\\Continuum\\Continuum.exe"), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                    
                    // Process has been created; work with the process and wait for it to terminate
                    hProcess = pi.hProcess;
                    *pid = pi.dwProcessId;
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
    for (size_t trys = 0; ; trys++) {
        Sleep(10);
        if (hMutex = OpenMutexW(MUTEX_ALL_ACCESS, FALSE, L"mtxsscont")) break;
        else if (trys > 1000) {
            return false;
        }
    }

    CloseHandle(hMutex);

    char text[] = "Continuum.exe";
    wchar_t wtext[20];
    mbstowcs_s(NULL, wtext, text, strlen(text) + 1);//Plus null
    LPWSTR ptr = wtext;

    /* Get process handle for respawned process */
    if (!(hProcess = GetProcessHandle(pid))) 
        return false;
    

    handleInfo = (PSYSTEM_HANDLE_INFORMATION)malloc(handleInfoSize);

    /* Get all of the system's handles */
    while ((status = NtQuerySystemInformation(16, handleInfo, handleInfoSize, NULL)) == 0xC0000004) {

        handleInfo = (PSYSTEM_HANDLE_INFORMATION)realloc(handleInfo, handleInfoSize *= 2);
    }
    
    if (!NT_SUCCESS(status)) 
        return false;
    

    /* Loop through the handles */
    for (ULONG i = 0; i < handleInfo->HandleCount; i++) {
        SYSTEM_HANDLE handle = handleInfo->Handles[i];
        POBJECT_TYPE_INFORMATION objectTypeInfo;
        UNICODE_STRING objectName;
        HANDLE dupHandle = NULL;
        PVOID objectNameInfo;
        ULONG returnLength;
        uintptr_t pHandle = (uintptr_t)handle.Handle;

        if (handle.ProcessId != *pid)
            continue;

        /* Duplicate the handle so we can query it. */
        if (!NT_SUCCESS(NtDuplicateObject(hProcess, (HANDLE)pHandle, GetCurrentProcess(), &dupHandle, 0, 0, 0)))
            continue;

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



HANDLE GetProcessHandle(DWORD *pid) {
    HANDLE pHandle = NULL;
    PROCESSENTRY32 pe32;
    HANDLE hProcSnap;

    hProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcSnap == INVALID_HANDLE_VALUE)
        return NULL;

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcSnap, &pe32)) {
        CloseHandle(hProcSnap);
        return NULL;
    }

    /* Loop through running processes to find continuum */
    do {
        if (wcscmp(pe32.szExeFile, L"Continuum.exe") == 0)
            *pid = pe32.th32ProcessID;
    } while (Process32Next(hProcSnap, &pe32));

    pHandle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, *pid);

    return pHandle;
}


bool RemoveDebugPrivileges(HANDLE token) {

    bool success = false;

        TOKEN_PRIVILEGES privileges;

        LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &privileges.Privileges[0].Luid);
        privileges.PrivilegeCount = 1;
        privileges.Privileges[0].Attributes = SE_PRIVILEGE_REMOVED;

        if (AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(TOKEN_PRIVILEGES), 0, 0))
            success = true;

    return success;
}

