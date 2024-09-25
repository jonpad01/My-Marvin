#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#ifdef UNICODE
#undef UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>



std::string GetLocalPath() {
  std::string directory;

  directory.resize(GetCurrentDirectory(0, NULL));

  GetCurrentDirectory(directory.size(), &directory[0]);

  return directory.substr(0, directory.size() - 1);
}


int main(int argc, char* argv[]) {

  std::string current_path = GetLocalPath();
  std::string destination_path = "C:\\Program Files (x86)\\Continuum";
  std::string alternative_path = "C:\\Program Files\\Continuum";

  std::string marvin_file = "\\Marvin.dll";
  std::string dsound_file = "\\dsound.dll";

  bool result = CopyFile((current_path + marvin_file).c_str(), (destination_path + marvin_file).c_str(), FALSE);
  
  if (!result) {
    std::cout << "Failed to copy file. You may need to run this program as administrator. \n"; 
    std::cin.get();
  }

  std::cout << current_path;
  
  int test;
  std::cin >> test;
}

