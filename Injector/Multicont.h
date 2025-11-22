#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>


namespace marvin {

	typedef LONG(__stdcall* _NtQuerySystemInformation)(ULONG SystemInformationClass, PVOID SystemInformation,
                                                   ULONG SystemInformationLength, PULONG ReturnLength);

typedef LONG(__stdcall* _NtDuplicateObject)(HANDLE SourceProcessHandle, HANDLE SourceHandle, HANDLE TargetProcessHandle,
                                            PHANDLE TargetHandle, ACCESS_MASK DesiredAccess, ULONG Attributes,
                                            ULONG Options);

typedef LONG(__stdcall* _NtQueryObject)(HANDLE ObjectHandle, ULONG ObjectInformationClass, PVOID ObjectInformation,
                                        ULONG ObjectInformationLength, PULONG ReturnLength);

typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _SYSTEM_HANDLE {
  ULONG ProcessId;
  BYTE ObjectTypeNumber;
  BYTE Flags;
  USHORT Handle;
  PVOID Object;
  ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE, *PSYSTEM_HANDLE;

typedef struct _SYSTEM_HANDLE_INFORMATION {
  ULONG HandleCount;
  SYSTEM_HANDLE Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef enum _POOL_TYPE {
  NonPagedPool,
  PagedPool,
  NonPagedPoolMustSucceed,
  DontUseThisType,
  NonPagedPoolCacheAligned,
  PagedPoolCacheAligned,
  NonPagedPoolCacheAlignedMustS
} POOL_TYPE,
    *PPOOL_TYPE;

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
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;


class Multicont {
 public:
  Multicont() : process_id_(0), thread_id_(0) {};

  bool RunMulticont();
  DWORD GetProcessID() { return process_id_; }
  DWORD GetThreadID() { return thread_id_; }

 private:
  bool RemoveDebugPrivileges(HANDLE token);
  HANDLE GetProcessHandle();
  PVOID GetLibraryProcAddress(LPSTR LibraryName, LPSTR ProcName);
  void GetLibraryAddresses(_NtQuerySystemInformation* ntqsi, _NtDuplicateObject* ntdo, _NtQueryObject* ntqo);

  DWORD process_id_;
  DWORD thread_id_;
};

}  // namespace marvin
