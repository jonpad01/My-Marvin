#include "MemoryMap.h"
#include "platform//platform.h"

const char SHARED_NAME[] = "Local\\MyMarvinSharedMemory";

bool MemoryMap::OpenMap() {
  hMap = hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, SHARED_NAME);

  if (hMap) {
    shm = (SharedMemory*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemory));
    if (shm) {
      return true;
    }
  }
  return false;
}

MemoryMap::~MemoryMap() {
  if (hMap) {
    CloseHandle(hMap);
  }
  if (shm) {
    UnmapViewOfFile(shm);
  }
}

void MemoryMap::WriteU32(uint32_t num) {
  shm->data = num;
  shm->state = ComState::LauncherReads;
}

uint32_t MemoryMap::ReadU32() {
  return shm->data;
}

bool MemoryMap::HasData() {

  if (shm->state == ComState::MarvinReads) {
    return true;
  }
  
  return false;
}