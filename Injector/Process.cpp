#include "Process.h"

#include "Memory.h"

namespace marvin {

std::size_t Process::GetModuleBase(const char* module_name) {
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id_);
  MODULEENTRY32 me = {0};

  me.dwSize = sizeof(me);

  if (hSnapshot == INVALID_HANDLE_VALUE) {
    return 0;
  }

  std::size_t module_base = 0;
  BOOL bModule = Module32First(hSnapshot, &me);
  while (bModule) {
    if (strcmp(module_name, me.szModule) == 0) {
      module_base = reinterpret_cast<std::size_t>(me.modBaseAddr);
      break;
    }

    bModule = Module32Next(hSnapshot, &me);
  }

  CloseHandle(hSnapshot);
  return module_base;
}

bool Process::HasModule(const char* module_name) {
  HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id_);
  MODULEENTRY32 me = {0};

  me.dwSize = sizeof(me);

  if (hSnapshot == INVALID_HANDLE_VALUE) {
    return false;
  }

  bool has_module = false;
  BOOL bModule = Module32First(hSnapshot, &me);

  while (bModule) {
    if (strcmp(module_name, me.szModule) == 0) {
      has_module = true;
      break;
    }

    bModule = Module32Next(hSnapshot, &me);
  }

  CloseHandle(hSnapshot);

  return has_module;
}

bool Process::InjectModule(const std::string& module_path) {
  bool injected = false;
  HMODULE hModule = GetModuleHandleA("kernel32.dll");
  HANDLE hProcess = GetHandle();

  while (!hProcess) {
    hProcess = GetHandle();
  }
  if (!hModule) {
    std::cout << "hModule null\n";
  }

  if (!hProcess) {
    std::cout << "Failed to get handle\n";
  }

  if (hProcess && hModule) {
    // Get the address to LoadLibrary in kernel32.dll
    LPVOID load_addr = (LPVOID)GetProcAddress(hModule, "LoadLibraryA");
    // Allocate some memory in Continuum's process to store the path to the
    // DLL to load.
    LPVOID path_addr = VirtualAllocEx(hProcess, NULL, module_path.size(), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    while (!path_addr) {
      path_addr = VirtualAllocEx(hProcess, NULL, module_path.size(), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    }

    if (!load_addr) {
      std::cout << "bad load addr\n";
    }

    if (!path_addr) {
      std::cout << "Bad allocation\n";
    }

    if (load_addr && path_addr) {
      // Write the path to the DLL to load into the recently allocated area.
      if (WriteProcessMemory(hProcess, path_addr, module_path.data(), module_path.size(), NULL)) {
        // Start a remote thread in the Continuum process that immediately
        // kicks off the load.
        injected =
            CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)load_addr, path_addr, 0, NULL) != NULL;
      } else {
        std::cout << "Bad WriteProcessMemory\n";
      }
    }
  }

  return injected;
}

HANDLE Process::GetHandle() {
  if (process_handle_) {
    return process_handle_;
  }

  const DWORD desired_access = PROCESS_ALL_ACCESS;

  process_handle_ = OpenProcess(desired_access, FALSE, process_id_);

  return process_handle_;
}

bool Process::SetProfile(HANDLE handle, std::size_t index) {
  std::size_t menu_base = GetModuleBase("menu040.dll");

  if (memory::WriteU32(handle, (menu_base + 0x47FA0), index)) {
    return true;
  }
  return false;
}

std::string ContinuumGameProxy::GetName() const {
  const size_t ProfileStructSize = 2860;
  DWORD pid = process_->GetId();
  HANDLE handle = process_->GetHandle();
  size_t menu_base = process_->GetModuleBase("menu040.dll");

  if (menu_base == 0) {
    return "";
  }

  uint16_t profile_index = memory::ReadU32(handle, menu_base + 0x47FA0) & 0xFFFF;
  size_t addr = memory::ReadU32(handle, menu_base + 0x47A38) + 0x15;

  if (addr == 0) {
    return "";
  }

  addr += profile_index * ProfileStructSize;

  std::string name = memory::ReadString(handle, addr, 23);

  name = name.substr(0, strlen(name.c_str()));

  return name;
}

std::string GetWorkingDirectory() {
  std::string directory;

  directory.resize(GetCurrentDirectory(0, NULL));

  GetCurrentDirectory(directory.size(), &directory[0]);

  return directory.substr(0, directory.size() - 1);
}

std::vector<DWORD> GetProcessIds(const char* exe_file) {
  std::vector<DWORD> pids;
  PROCESSENTRY32 pe32 = {};

  pe32.dwSize = sizeof(PROCESSENTRY32);

  HANDLE hTool32 = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
  BOOL bProcess = Process32First(hTool32, &pe32);

  while (bProcess) {
    if (strcmp(pe32.szExeFile, "Continuum.exe") == 0) {
      pids.push_back(pe32.th32ProcessID);
    }

    bProcess = Process32Next(hTool32, &pe32);
  }

  CloseHandle(hTool32);

  return pids;
}

std::string Lowercase(const std::string& str) {
  std::string result;

  result.resize(str.size());

  std::transform(str.begin(), str.end(), result.begin(), ::tolower);

  return result;
}

DWORD SelectPid(const std::vector<DWORD>& pids, std::string target_player) {
  std::transform(target_player.begin(), target_player.end(), target_player.begin(), tolower);

  for (std::size_t i = 0; i < pids.size(); ++i) {
    auto pid = pids[i];
    auto game = marvin::ContinuumGameProxy(std::make_unique<marvin::Process>(pid));

    std::string name = game.GetName();

    std::transform(name.begin(), name.end(), name.begin(), tolower);

    if (name == target_player) {
      return pid;
    }
  }

  std::cerr << "Failed to find process with that name." << std::endl;

  return 0;
}

DWORD SelectPid(const std::vector<DWORD>& pids) {
  std::cout << "0"
            << ": "
            << "Auto Mode\n";
  for (std::size_t i = 0; i < pids.size(); ++i) {
    auto pid = pids[i];
    auto game = marvin::ContinuumGameProxy(std::make_unique<marvin::Process>(pid));

    std::string name = game.GetName();

    std::cout << (i + 1) << ": " << pid << " (" << name << ")";

    auto& process = game.GetProcess();

    if (process.HasModule(INJECT_MODULE_NAME)) {
      std::cout << " - Already loaded." << std::endl;
    } else {
      std::cout << std::endl;
    }
  }

  std::cout << "> ";

  std::string input;
  std::cin >> input;

  auto selection = strtol(input.c_str(), nullptr, 10);

  if (selection < 0 || selection > (long)pids.size()) {
    std::cerr << "Invalid selection." << std::endl;
    return 0;
  }
  if (input == "0") return 1;
  return pids[selection - 1];
}

void InjectExcept(const std::vector<DWORD>& pids, const std::string& exception) {
  std::string inject_path = marvin::GetWorkingDirectory() + "\\" + INJECT_MODULE_NAME;

  std::size_t count = 0;

  std::string lower_exception = Lowercase(exception);

  for (DWORD pid : pids) {
    auto game = marvin::ContinuumGameProxy(std::make_unique<marvin::Process>(pid));
    auto& process = game.GetProcess();

    std::string name = Lowercase(game.GetName());

    if (name == lower_exception) continue;

    if (process.HasModule(INJECT_MODULE_NAME)) {
      continue;
    }

    if (process.InjectModule(inject_path)) {
      ++count;
    }
  }

  std::cout << "Successfully injected into " << count << " processes." << std::endl;
}

bool GetDebugPrivileges() {
  HANDLE token = nullptr;
  bool success = false;

  if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token)) {
    TOKEN_PRIVILEGES privileges;

    LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &privileges.Privileges[0].Luid);
    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(TOKEN_PRIVILEGES), 0, 0)) success = true;

    CloseHandle(token);
  }

  return success;
}

}  // namespace marvin
