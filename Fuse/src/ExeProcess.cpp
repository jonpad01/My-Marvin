#include <fuse/ExeProcess.h>
//
#include <TlHelp32.h>

namespace fuse {

ExeProcess::ExeProcess() {
  process_handle_ = GetModuleHandleA(NULL);
  process_id_ = GetCurrentProcessId();
}

ExeProcess::~ExeProcess() {}

uint32_t ExeProcess::ReadU32(MemoryAddress address) const {
  return *(uint32_t*)address;
}

int32_t ExeProcess::ReadI32(MemoryAddress address) const {
  return *(int32_t*)address;
}

bool ExeProcess::WriteU32(MemoryAddress address, uint32_t value) {
  uint32_t* data = (uint32_t*)address;
  *data = value;

  return true;
}

std::string ExeProcess::ReadString(MemoryAddress address, size_t length) const {
  std::string value;
  char* data = (char*)address;

  value.resize(length);

  for (size_t i = 0; i < length; ++i) {
    if (data[i] == 0) {
      length = i;
      break;
    }
    value[i] = data[i];
  }

  return value.substr(0, length);
}

MemoryAddress ExeProcess::GetModuleBase(const char* module_name) {
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id_);
  MODULEENTRY32 me = {0};

  me.dwSize = sizeof(me);

  if (hSnapshot == INVALID_HANDLE_VALUE) {
    return 0;
  }

  MemoryAddress module_base = 0;
  BOOL bModule = Module32First(hSnapshot, &me);
  while (bModule) {
    if (strcmp(module_name, me.szModule) == 0) {
      module_base = reinterpret_cast<MemoryAddress>(me.modBaseAddr);
      break;
    }

    bModule = Module32Next(hSnapshot, &me);
  }

  CloseHandle(hSnapshot);
  return module_base;
}

HANDLE ExeProcess::GetHandle() {
  return process_handle_;
}

DWORD ExeProcess::GetId() {
  return process_id_;
}

}  // namespace fuse
