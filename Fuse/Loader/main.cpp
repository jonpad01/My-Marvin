#include "DirectSound.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
//
#include <TlHelp32.h>
#include <psapi.h>

#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <thread>

typedef void (*InitFunc)();
typedef void (*CleanupFunc)();

const std::string MARVIN_DLL_FOLDER = "E:\\Code_Projects\\My-Marvin\\bin";

std::ofstream debug_log;

static HMODULE hModule = NULL;
static std::string g_MarvinPath;
static std::string g_MarvinDirectory;
static std::string g_MarvinDll;
static std::string g_MarvinLoadedPath;
static FILETIME g_LastTime;
static std::thread g_MonitorThread;

static std::string GetLocalPath() {
  char path[MAX_PATH];

  GetCurrentDirectory(MAX_PATH, path);

  return std::string(path);
}

std::vector<std::string> GetArguments() {
  std::vector<std::string> result;

  int argv = 0;
  LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &argv);

  for (int i = 0; i < argv; ++i) {
    size_t size = wcslen(args[0]);

    std::string str;
    str.resize(size);

    wcstombs(&str[0], args[i], size);
    result.push_back(std::move(str));
  }

  return result;
}

void SetMarvinPath() {
  std::string curr_dir = GetLocalPath();
  std::string name = "\\Marvin.dll";
  std::string full_path;
  std::vector<std::string> args = GetArguments();

  // check if path to Marvin.dll was given to continuum as an arg and use that first
  for (std::string arg : args) {
    if (std::filesystem::exists(arg + name)) {
      g_MarvinDirectory = arg;
      g_MarvinPath = arg + name;
      g_MarvinLoadedPath = arg + "\\" + g_MarvinDll;
      return;
    }
  }

  // check if path was manually set as const variable
  if (std::filesystem::exists(MARVIN_DLL_FOLDER + name)) {
    g_MarvinDirectory = MARVIN_DLL_FOLDER;
    g_MarvinPath = MARVIN_DLL_FOLDER + name;
    g_MarvinLoadedPath = MARVIN_DLL_FOLDER + "\\" + g_MarvinDll;
    return;
  }

  // look in current directory (Continuum directory)
  if (std::filesystem::exists(curr_dir + name)) {
    g_MarvinDirectory = curr_dir;
    g_MarvinPath = curr_dir + name;
    g_MarvinLoadedPath = curr_dir + "\\" + g_MarvinDll;
    return;
  }

}

int RemoveMatchingFiles(const std::string& substring) {
  // search for and delete temporary marvin dll files
  for (const auto& entry : std::filesystem::directory_iterator(g_MarvinDirectory)) {
    std::string path = entry.path().generic_string();
    std::size_t found = path.find(substring);
    if (found != std::string::npos) {
      DeleteFile(path.c_str());
    }
  }
  return 0;
}

static std::string GetSystemLibrary(const char* library) {
  std::string result;

  char system_path[MAX_PATH];

  GetSystemDirectory(system_path, MAX_PATH);
  result = std::string(system_path) + "\\" + std::string(library);

  return result;
}

bool GetLastWriteTime(const char* filename, FILETIME* ft) {
  WIN32_FILE_ATTRIBUTE_DATA data;

  if (GetFileAttributesExA(g_MarvinPath.c_str(), GetFileExInfoStandard, &data)) {
    *ft = data.ftLastWriteTime;
    return true;
  }

  return false;
}

bool WaitForUnload() {
  DWORD pid = GetCurrentProcessId();
  HANDLE hProcess = GetCurrentProcess();
  HMODULE hMods[1024];
  DWORD cbNeeded;

  bool loaded = true;

  int loops = 0;

  while (loaded) {
    loaded = false;

    ++loops;

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
      for (std::size_t i = 0; i < (cbNeeded / sizeof(HMODULE)); ++i) {
        std::string module;

        module.resize(MAX_PATH);

        if (GetModuleFileNameEx(hProcess, hMods[i], &module[0], MAX_PATH)) {
          if (module.find(g_MarvinLoadedPath) != std::string::npos) {
            loaded = true;
          }
        }
      }
    }
  }

#if 0
    if (loops > 1) {
        std::string str = "Loops: " + std::to_string(loops);

        MessageBox(NULL, str.c_str(), "A", MB_OK);
    }
#endif

  return true;
}

void PerformReload() {
  if (hModule) {
     CleanupFunc cleanup = (CleanupFunc)GetProcAddress(hModule, "CleanupMarvin");

      if (cleanup) {
        cleanup();
      }

    FreeLibrary(hModule);
    WaitForUnload();
  }

  hModule = NULL;

  for (int tries = 0; tries < 20; ++tries) {
    if (CopyFile(g_MarvinPath.c_str(), g_MarvinLoadedPath.c_str(), FALSE) != 0) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  hModule = LoadLibrary(g_MarvinLoadedPath.c_str());

   if (hModule) {
    InitFunc init = (InitFunc)GetProcAddress(hModule, "InitializeMarvin");

    if (init) {
      init();
    }
   }
}

void MonitorDevFile() {
  FILETIME time;

  while (true) {
    if (GetLastWriteTime(g_MarvinPath.c_str(), &time)) {
      if (CompareFileTime(&time, &g_LastTime) > 0) {
        PerformReload();

        GetLastWriteTime(g_MarvinLoadedPath.c_str(), &time);
        g_LastTime = time;
      }
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
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
      hModule = NULL;
      std::string dsound_path = GetSystemLibrary("dsound.dll");
      dsound = LoadLibrary(dsound_path.c_str());

      if (!dsound) {
        MessageBox(NULL, "Failed to load system dsound.dll", "fuse", MB_ICONERROR | MB_OK);
        return FALSE;
      }

      fuse::g_DirectSound = fuse::DirectSound::Load(dsound);

      

      
      DWORD pid = GetCurrentProcessId();
     // debug_log.open("fuse_loader - " + std::to_string(pid) + ".log", std::ios::out);
      g_MarvinDll = "Marvin-" + std::to_string(pid) + ".dll";
      SetMarvinPath();

      PerformReload();
      GetLastWriteTime(g_MarvinLoadedPath.c_str(), &g_LastTime);

      g_MonitorThread = std::thread(MonitorDevFile);

#if 0
      HMODULE loaded = LoadLibrary(g_MarvinPath.c_str());

      if (!loaded) {
        g_MarvinPath = MARVIN_DLL_FOLDER + "\\Marvin.dll";
        g_MarvinLoadedPath = MARVIN_DLL_FOLDER + "\\" + g_MarvinDll;

        loaded = LoadLibrary(g_MarvinPath.c_str());
        if (loaded) {
          debug_log << "File found in location : " << MARVIN_DLL_FOLDER << std::endl;
        } else {
          debug_log << "Could not find Marvin.dll" << std::endl;
        }
      
        
      } else {
        debug_log << "File found in location : " << (GetLocalPath() + "\\Marvin.dll") << std::endl;
      }
#endif


      //debug_log << "Marvin result : " << (loaded ? " Success " : " Failure ") << std::endl;
      //InitializeLoader();

      //size_t hook_count = fuse::Fuse::Get().GetHooks().size();
     // debug_log << "Hook count: " << hook_count << std::endl;
    } break;
    case DLL_PROCESS_DETACH: {
      FreeLibrary(dsound);
      if (hModule) {
        FreeLibrary(hModule);
      }
      RemoveMatchingFiles("Marvin-");
    } break;
  }

  return TRUE;
}
