#pragma once

#include <fuse/Platform.h>
#include <fuse/Types.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fuse {

class FUSE_EXPORT ExeProcess {
 public:
  ExeProcess();
  ~ExeProcess();

  ExeProcess(const ExeProcess& other) = delete;
  ExeProcess& operator=(const ExeProcess& other) = delete;

  uint32_t ReadU32(MemoryAddress address) const;
  int32_t ReadI32(MemoryAddress address) const;
  std::string ReadString(MemoryAddress address, size_t length) const;

  bool WriteU32(MemoryAddress address, uint32_t value);

  MemoryAddress GetModuleBase(const char* module_name);

  HANDLE GetHandle();
  DWORD GetId();

 private:
  HANDLE process_handle_;
  DWORD process_id_;
};

}  // namespace fuse
