#pragma once

#include <cstdint>

using HANDLE = void*;

enum class ComState : uint32_t { None, LauncherReads, MarvinReads };

#pragma pack(push, 1)
struct SharedMemory {
  uint32_t data = 0;
  ComState state = ComState::None;
};
#pragma pack(pop)

static_assert(sizeof(SharedMemory) == 8, "SharedMemory size mismatch!");

class MemoryMap {
 public:
  ~MemoryMap();
  bool OpenMap();
  void WriteU32(uint32_t num);
  uint32_t ReadU32();
  bool HasData();

 private:
  HANDLE hMap = NULL;
  SharedMemory* shm = nullptr;
};