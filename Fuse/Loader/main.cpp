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

//const std::string MARVIN_DLL_FOLDER = "E:\\Code_Projects\\My-Marvin\\bin";

std::ofstream debug_log;

static HMODULE hModule = NULL;
static HMODULE dsound = NULL;
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
  //std::vector<std::string> args = GetArguments();

  #if 0
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

  #endif

  // look in current directory (Continuum directory)
  if (std::filesystem::exists(curr_dir + name)) {
    g_MarvinDirectory = curr_dir;
    g_MarvinPath = curr_dir + name;
    g_MarvinLoadedPath = curr_dir + "\\" + g_MarvinDll;
    return;
  }

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

bool WaitForUnload(const std::string& path) {
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
          if (module.find(path) != std::string::npos) {
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

void PerformReload(const char* source, const char* destination) {
  if (hModule) {
      #if 0  // is aleady being porformed in dll dettach
     CleanupFunc cleanup = (CleanupFunc)GetProcAddress(hModule, "CleanupMarvin");

      if (cleanup) {
        cleanup();
      }
      #endif

    FreeLibrary(hModule);
    WaitForUnload(destination);
  }

  hModule = NULL;

  for (int tries = 0; tries < 20; ++tries) {
    if (CopyFile(source, destination, FALSE) != 0) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  hModule = LoadLibrary(destination);

  #if 0 // is aleady being porformed in dll attach
   if (hModule) {
    InitFunc init = (InitFunc)GetProcAddress(hModule, "InitializeMarvin");

    if (init) {
      init();
    }
   }
   #endif
}

void MonitorDevFile() {
  FILETIME time;

  while (true) {
    if (GetLastWriteTime(g_MarvinPath.c_str(), &time)) {
      if (CompareFileTime(&time, &g_LastTime) > 0) {
        PerformReload(g_MarvinPath.c_str(), g_MarvinLoadedPath.c_str());
        GetLastWriteTime(g_MarvinLoadedPath.c_str(), &time);
        g_LastTime = time;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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

// child process wont allow a handle with process_all_access from inside this dll
bool IsRealProcess() {
  HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
  if (!h && GetLastError() == ERROR_ACCESS_DENIED) return true;  
  if (h) CloseHandle(h);
  return false;
}

bool IsElevated() {
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return false;

  TOKEN_ELEVATION elevation;
  DWORD size = 0;
  BOOL result = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size);

  CloseHandle(token);
  return result && elevation.TokenIsElevated;
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID reserved) {
  switch (dwReason) {
    case DLL_PROCESS_ATTACH: {

      // Continuum respawns itself to apply a DACL, so dsound.dll will get loaded twice
      // if loaded into the parent process do nothing because its going to exit anyway
      if (!IsRealProcess()) {
        return TRUE;
      } 

      DWORD pid = GetCurrentProcessId();
     // debug_log.open("fuse_loader - " + std::to_string(pid) + ".log", std::ios::out);

      std::string dsound_path = GetSystemLibrary("dsound.dll");
      dsound = LoadLibrary(dsound_path.c_str());

      if (!dsound) {
        MessageBox(NULL, "Failed to load system dsound.dll", "fuse", MB_ICONERROR | MB_OK);
        return FALSE;
      }

      fuse::g_DirectSound = fuse::DirectSound::Load(dsound);

      g_MarvinDll = "Marvin-" + std::to_string(pid) + ".dll";
      SetMarvinPath();


      if (IsElevated()) {
        PerformReload(g_MarvinPath.c_str(), g_MarvinLoadedPath.c_str());
        g_MonitorThread = std::thread(MonitorDevFile);
      } else {
        hModule = LoadLibraryA(g_MarvinPath.c_str());
      }
      
     

      //debug_log << "Marvin result : " << (loaded ? " Success " : " Failure ") << std::endl;
      //InitializeLoader();

      //size_t hook_count = fuse::Fuse::Get().GetHooks().size();
     // debug_log << "Hook count: " << hook_count << std::endl;
    } break;
    case DLL_PROCESS_DETACH: {
      // there is no good method to delete temp marvin files here because they will still be loaded
      // an option might be memory mapping or reflective loading
      // dont need to unload libraries here as that is already handled when the process exits.
      // dont need to exit the thead that was created as it seems to exit automatically.
    } break;
  }

  return TRUE;
}
