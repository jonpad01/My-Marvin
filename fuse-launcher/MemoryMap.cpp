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

// not realy accurate if timeout is less than 100 milliseconds
bool MemoryMap::WaitForData(float timeout) {

    float num = timeout * 0.01f;

   for (float i = 0; i < num; i++) {
      Sleep(100);
     if (shm->state == ComState::LauncherReads) {
       return true;
     }
   } 

  return false;
}