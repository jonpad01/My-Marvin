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

#define DEV_SWAP 0

typedef void (*InitFunc)();
typedef void (*CleanupFunc)();

struct MonitorContext {
  char marvinPath[MAX_PATH];
  char loadedPath[MAX_PATH];
  HMODULE hModule;
};

struct FindWindowCtx {
  DWORD pid = 0;
  HWND hwnd = nullptr;
  char className[MAX_PATH];
};

//const std::string MARVIN_DLL_FOLDER = "E:\\Code_Projects\\My-Marvin\\bin";

std::ofstream debug_log;

//static HMODULE hModule = NULL;
static HMODULE dsound = NULL;
static std::string g_MarvinPath;
static std::string g_MarvinDirectory;
static std::string g_MarvinDll;
static std::string g_MarvinLoadedPath;

HANDLE g_ThreadHandle = nullptr;

static std::string GetLocalPath() {
  char path[MAX_PATH];

  GetCurrentDirectory(MAX_PATH, path);

  return std::string(path);
}

BOOL CALLBACK EnumProc(HWND hwnd, LPARAM lParam) {
  auto* ctx = reinterpret_cast<FindWindowCtx*>(lParam);

  if (IsWindowVisible(hwnd)) return TRUE;
  DWORD pid = 0;
  GetWindowThreadProcessId(hwnd, &pid);

  if (pid != ctx->pid) return TRUE;

  char title[256]{};
  GetWindowTextA(hwnd, title, sizeof(title));
  char className[256];
  GetClassNameA(hwnd, className, sizeof(className));

  //if (strcmp(title, "Default IME") != 0) return TRUE;
  //if (strcmp(title, "MSCTFIME UI") != 0) return TRUE;
  if (strcmp(className, ctx->className) != 0) return TRUE;

  ctx->hwnd = hwnd;
  return FALSE; // stop enumeration
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

  if (GetFileAttributesExA(filename, GetFileExInfoStandard, &data)) {
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

HMODULE LoadMarvin(const char* source) {
 HMODULE hModule = LoadLibrary(source);

  if (hModule) {
    InitFunc init = (InitFunc)GetProcAddress(hModule, "InitializeMarvin");

    if (init) {
      init();
    }
  }

  return hModule;
}


void HotReloadMarvin(HMODULE& hModule, const char* original, const char* temp) {
  if (hModule) {
     CleanupFunc cleanup = (CleanupFunc)GetProcAddress(hModule, "CleanupMarvin");

      if (cleanup) {
        cleanup();
      }

    FreeLibrary(hModule);
    WaitForUnload(temp);
  }

  hModule = NULL;

  for (int tries = 0; tries < 20; ++tries) {
    if (CopyFile(original, temp, FALSE) != 0) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  hModule = LoadLibrary(temp);

  if (hModule) {
    InitFunc init = (InitFunc)GetProcAddress(hModule, "InitializeMarvin");

    if (init) {
      init();
    }
  }
}

// dsound.dll gets unloaded when continuum exits
// continuum does not call dll_detach
// this thread should not read global variables
// the only method for this thread to detect game state and exit is to watch window hwnd
// everything else is too late and thread is force stopped
// if continuum closes during a call to loadlibrary or freelibrary, 
// im pretty sure it gets stuck in a load lock and never fully teriminates
// the thread grabs a hidden hwnd at startup that remains valid when transitioning to the game
// when transitioning back to the menu the window is destroyed and the hwnd will become invalid
// and this thread will exit
// so if the game is dropped to the menu and reloaded, hot swapping will be disabled
DWORD WINAPI MonitorThread(LPVOID param) {
  FILETIME time{};
  FILETIME lastTime{};
  //HMODULE hModule = NULL;

  //OutputDebugStringA("MonitorThread: thread created\n");

  // thread owns path data and doesnt have to read globals
  std::unique_ptr<MonitorContext> ctx(static_cast<MonitorContext*>(param));

  //HotReloadMarvin(hModule, ctx->marvinPath, ctx->loadedPath);
  GetLastWriteTime(ctx->marvinPath, &lastTime);

  FindWindowCtx window;
  HWND hwnd = nullptr;

  // try to find the window
  for (int i = 0; i < 500; i++) {
    Sleep(100);
    window.pid = GetCurrentProcessId();
    strncpy_s(window.className, "MSCTFIME UI", MAX_PATH);
    EnumWindows(EnumProc, reinterpret_cast<LPARAM>(&window));
    hwnd = window.hwnd;

    if (hwnd) { 
     // OutputDebugStringA("MonitorThread: hwnd found\n");
      break;
    }
  }

  while (true) {

    // game is closing or hwnd was never found
    if (!IsWindow(hwnd)) {
       break;
    }

    if (GetLastWriteTime(ctx->marvinPath, &time)) {
      if (CompareFileTime(&time, &lastTime) > 0) {
        HotReloadMarvin(ctx->hModule, ctx->marvinPath, ctx->loadedPath);
        //OutputDebugStringA("MonitorThread: marvin reloaded.\n");
        lastTime = time;
      }
    }
    Sleep(100);
  }

  //OutputDebugStringA("MonitorThread: thread stopped\n");
  return 0;
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

// parent process will allow but child process wont
bool IsRealProcess() {
  HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());

  if (!h && GetLastError() == ERROR_ACCESS_DENIED) return true;  

  if (h) { CloseHandle(h); }
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
      //debug_log.open("dsound.Log");
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

      // can't do file copies to the C: without elevated access
      // so only allow hot reloading when ran as admin
      if (IsElevated()) {        
        auto* ctx = new MonitorContext{};
        HMODULE module = NULL;
        // if loaded for the first time in the thread it breaks the multicont mutex override for some reason
        HotReloadMarvin(module, g_MarvinPath.c_str(), g_MarvinLoadedPath.c_str());
        ctx->hModule = module;
        strncpy_s(ctx->marvinPath, g_MarvinPath.c_str(), MAX_PATH);
        strncpy_s(ctx->loadedPath, g_MarvinLoadedPath.c_str(), MAX_PATH);
        g_ThreadHandle = CreateThread(nullptr, 0, MonitorThread, ctx, 0, nullptr);

        if (!g_ThreadHandle) { delete ctx; }

        //LoadMarvinWithTempFile(g_MarvinPath.c_str(), g_MarvinLoadedPath.c_str());

      } else {
        LoadMarvin(g_MarvinPath.c_str());
      }
      
     

      //debug_log << "Marvin result : " << (loaded ? " Success " : " Failure ") << std::endl;
      //InitializeLoader();

      //size_t hook_count = fuse::Fuse::Get().GetHooks().size();
     // debug_log << "Hook count: " << hook_count << std::endl;
    } break;
    case DLL_PROCESS_DETACH: {
      // the game does not call this when closing so can't do anything here
    } break;
  }

  return TRUE;
}
