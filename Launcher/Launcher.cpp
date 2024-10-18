#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

#ifdef UNICODE
#undef UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using std::filesystem::exists;

const std::string CONTINUUM_FOLDER = "";

std::string GetLocalPath() {
  std::string directory;

  directory.resize(GetCurrentDirectory(0, NULL));

  GetCurrentDirectory(directory.size(), &directory[0]);

  return directory.substr(0, directory.size() - 1);
}

bool FolderExists(std::string path) {
  return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

// folder should have the continuum executable (steam folder will still exist when the game is uninstalled)
std::string FindContinuumDirectory() {
  const std::string path1 = "Program Files (x86)\\Continuum";
  const std::string path2 = "Program Files\\Continuum";
  const std::string path3 = "SteamLibrary\\steamapps\\common\\Continuum";
  const std::string path4 = "Continuum";
  const std::vector<std::string> paths{path1, path2, path3, path4};
  const std::string cont_exe = "\\Continuum.exe";

  DWORD dwSize = MAX_PATH;
  TCHAR szDrives[MAX_PATH] = {0};

  // Check if path was manually added first
  if (exists(CONTINUUM_FOLDER + cont_exe)) {
    return CONTINUUM_FOLDER;
  }

  // check if Launcher is in the same directory as Continuum
  if (exists(GetLocalPath() + cont_exe)) {
    return GetLocalPath();
  }

   if (GetLogicalDriveStrings(dwSize, szDrives)) {
    TCHAR* pDrive = szDrives;
    while (*pDrive) {
      for (std::string path : paths) {
        std::string directory = (std::string)pDrive + path;
        if (exists(directory + cont_exe)) {
          return directory;
        }
      }
      pDrive += strlen(pDrive) + 1;
    }
  } else {
    std::cerr << "GetLogicalDriveStrings failed with error: " << GetLastError() << std::endl;
  }

  return "";
}


int main(int argc, char* argv[]) {

  std::string curr_dir = GetLocalPath();
  std::string cont_dir = FindContinuumDirectory();

  std::string marvin = "\\Marvin.dll";
  std::string dsound = "\\dsound.dll";

  if (cont_dir.empty()) {
    std::cout << "Could not find Continuum directory. You may need to manually add the folder path. Press enter to exit.\n";
    std::cin.get();
    return 0;
  } else {
    std::cout << "Continuum directory found in: " << cont_dir << std::endl;
  }

  if (!exists(curr_dir + marvin) || !exists(curr_dir + dsound)) {
    std::cout
        << "Could not find Marvin.dll and/or dsound.dll. Both files must be in the same folder as Launcher.exe. Press enter to exit.\n";
    std::cin.get();
    return 0;
  }

  // don't copy dsound if Launcher is in the Continuum directory
  if (curr_dir != cont_dir) {
    bool result1 = CopyFile((curr_dir + marvin).c_str(), (cont_dir + marvin).c_str(), FALSE);
    bool result2 = CopyFile((curr_dir + dsound).c_str(), (cont_dir + dsound).c_str(), FALSE);

    if (!result1 || !result2) {
      std::cout << "Failed to copy dsound.dll to Continuum directory. You may need to run Launcher.exe as admin. \n";
      std::cin.get();
    } else {
      std::cout << "dsound.dll has been copied to Continuum directory" << std::endl;
    }
  }
  

  std::cin.get();
}

