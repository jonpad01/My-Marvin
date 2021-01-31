
#include <tchar.h>
#include <windows.h>
#include <sddl.h>
#include <stdio.h>
#include <stdint.h>
#include <TlHelp32.h>

#include "Multicont.h"

typedef LONG(__stdcall* _NtQuerySystemInformation)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength);

typedef LONG(__stdcall* _NtDuplicateObject)(
    HANDLE SourceProcessHandle,
    HANDLE SourceHandle,
    HANDLE TargetProcessHandle,
    PHANDLE TargetHandle,
    ACCESS_MASK DesiredAccess,
    ULONG Attributes,
    ULONG Options);

typedef LONG(__stdcall* _NtQueryObject)(
    HANDLE ObjectHandle,
    ULONG ObjectInformationClass,
    PVOID ObjectInformation,
    ULONG ObjectInformationLength,
    PULONG ReturnLength);

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef struct _SYSTEM_HANDLE {
    ULONG ProcessId;
    BYTE ObjectTypeNumber;
    BYTE Flags;
    USHORT Handle;
    PVOID Object;
    ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE, * PSYSTEM_HANDLE;

typedef struct _SYSTEM_HANDLE_INFORMATION {
    ULONG HandleCount;
    SYSTEM_HANDLE Handles[1];
} SYSTEM_HANDLE_INFORMATION, * PSYSTEM_HANDLE_INFORMATION;

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS
} POOL_TYPE, * PPOOL_TYPE;

typedef struct _OBJECT_TYPE_INFORMATION {
    UNICODE_STRING Name;
    ULONG TotalNumberOfObjects;
    ULONG TotalNumberOfHandles;
    ULONG TotalPagedPoolUsage;
    ULONG TotalNonPagedPoolUsage;
    ULONG TotalNamePoolUsage;
    ULONG TotalHandleTableUsage;
    ULONG HighWaterNumberOfObjects;
    ULONG HighWaterNumberOfHandles;
    ULONG HighWaterPagedPoolUsage;
    ULONG HighWaterNonPagedPoolUsage;
    ULONG HighWaterNamePoolUsage;
    ULONG HighWaterHandleTableUsage;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccess;
    BOOLEAN SecurityRequired;
    BOOLEAN MaintainHandleCount;
    USHORT MaintainTypeList;
    POOL_TYPE PoolType;
    ULONG PagedPoolUsage;
    ULONG NonPagedPoolUsage;
} OBJECT_TYPE_INFORMATION, * POBJECT_TYPE_INFORMATION;


#define NT_SUCCESS(x) ((x) >= 0)




//this starts a continuum process and uses a mutex to bypass the limit on how many clients can be opened at once
//the injector has to give itself debug priveleges to work, but continuum cant inherit it when it is created (crashes)
//make a duplicate token with debug removed and then use it to launch continuum


bool Multicont::RunMulticont() {

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
                    pid_ = pi.dwProcessId;
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

        if (handle.ProcessId != pid_)
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


HANDLE Multicont::GetProcessHandle() {
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
            pid_ = pe32.th32ProcessID;
    } while (Process32Next(hProcSnap, &pe32));

    pHandle = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid_);

    return pHandle;
}


bool Multicont::RemoveDebugPrivileges(HANDLE token) {

    bool success = false;

    TOKEN_PRIVILEGES privileges;

    LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &privileges.Privileges[0].Luid);
    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_REMOVED;

    if (AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(TOKEN_PRIVILEGES), 0, 0))
        success = true;

    return success;
}