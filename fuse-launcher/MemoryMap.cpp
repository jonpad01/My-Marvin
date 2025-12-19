#include "MemoryMap.h"
#include <Windows.h>


const char SHARED_NAME[] = "Local\\MyMarvinSharedMemory";

bool MemoryMap::CreateMap() {
  hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(SharedMemory), SHARED_NAME);

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
  shm->state = ComState::MarvinReads;
}

uint32_t MemoryMap::ReadU32() {
  return shm->data;
}

