#include "DirectSound.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <fuse/Fuse.h>

std::ofstream debug_log;

static std::string GetLocalPath() {
  char path[MAX_PATH];

  GetCurrentDirectory(MAX_PATH, path);

  return std::string(path);
}

static std::string GetSystemLibrary(const char* library) {
  std::string result;

  char system_path[MAX_PATH];

  GetSystemDirectory(system_path, MAX_PATH);
  result = std::string(system_path) + "\\" + std::string(library);

  return result;
}

static std::vector<std::string> GetLoadRequests(const char* filename) {
  std::vector<std::string> requests;

  std::ifstream in(filename, std::ios::in);

  if (!in.good()) return requests;

  std::string line;
  line.reserve(256);

  while (std::getline(in, line)) {
    if (line.length() > 0) {
      requests.push_back(line);
    }
  }

  return requests;
}

void InitializeLoader() {
  std::string base_path = GetLocalPath();

  debug_log.open("fuse_loader.log", std::ios::out);

  debug_log << "Reading config file." << std::endl;

  std::vector<std::string> requests = GetLoadRequests("fuse.cfg");

  size_t load_count = requests.size();
  debug_log << "Attempting to load " << load_count << (load_count == 1 ? " library" : " libraries") << std::endl;

  for (size_t i = 0; i < load_count; ++i) {
    std::string full_path = base_path + "\\" + requests[i];

    HMODULE loaded_module = LoadLibrary(full_path.c_str());
    debug_log << full_path << ": " << (loaded_module ? "Success" : "Failure") << std::endl;
  }
}

static HMODULE dsound;

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID reserved) {
  switch (dwReason) {
    case DLL_PROCESS_ATTACH: {
      std::string dsound_path = GetSystemLibrary("dsound.dll");
      dsound = LoadLibrary(dsound_path.c_str());

      if (!dsound) {
        MessageBox(NULL, "Failed to load system dsound.dll", "fuse", MB_ICONERROR | MB_OK);
        return FALSE;
      }

      fuse::g_DirectSound = fuse::DirectSound::Load(dsound);

      fuse::Fuse::Get().Inject();

      InitializeLoader();

      size_t hook_count = fuse::Fuse::Get().GetHooks().size();
      debug_log << "Hook count: " << hook_count << std::endl;
    } break;
    case DLL_PROCESS_DETACH: {
      FreeLibrary(dsound);
    } break;
  }

  return TRUE;
}
