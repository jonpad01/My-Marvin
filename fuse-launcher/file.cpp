#include "file.h"
#include <iostream>
#include <fstream>
#include <filesystem>



std::string GetWorkingDirectory() {
  std::string directory;

  directory.resize(GetCurrentDirectory(0, NULL));

  GetCurrentDirectory(directory.size(), &directory[0]);

  return directory.substr(0, directory.size() - 1);
}

void RemoveMatchingFiles(const std::string& substring) {
  // search for and delete temporary marvin dll files
  for (const auto& entry : std::filesystem::directory_iterator(GetWorkingDirectory())) {
    std::string path = entry.path().generic_string();
    std::size_t found = path.find(substring);
    if (found != std::string::npos) {
      DeleteFile(path.c_str());
    }
  }
}

std::vector<MarvinData> LoadProfileData() {

  std::vector<MarvinData> bot_data;
  MarvinData profile_data;
  std::string path = GetWorkingDirectory() + "\\Profile-Data-Marvin.cfg";
  std::ifstream file(path);
  std::string buffer;
  int line_count = 0;

  if (!file.is_open()) {
    std::cout << "Error: Could not open Profile-Data-Marvin.cfg" << std::endl; 
    std::cout << "This file must be in the same directory as fuse-launcher.exe" << std::endl;
    std::cin.get();
    return bot_data;
  }

  while (!file.eof() && file.good()) {
    getline(file, buffer);

    if (buffer.empty()) continue; // ignore empty lines

    profile_data.profile_data.push_back(buffer);

    if (line_count == 0) {
      std::size_t pos = buffer.find("=");

      if (pos == std::string::npos) {
        std::cout << "marvin-profile-data.cfg is not formated correctly" << std::endl;
        std::cout << buffer;
        std::cin.get();
        return bot_data;
      }

      profile_data.name = buffer.substr(pos + 2);
    }

    if (line_count >= 5) {
      line_count = 0;
      bot_data.push_back(profile_data);
      profile_data.profile_data.clear();
    } else {
      line_count++;
    }
  }

  file.close();

  return bot_data;
}

bool WriteToFile(const std::vector<std::string>& data, const std::string& file_name) {
  std::ofstream file(file_name);

  if (!file.is_open()) {
    std::cout << "Error: could not write to MarvinData.txt" << std::endl;
    std::cin.get();
    return false;
  }

  for (std::size_t i = 0; i < data.size(); i++) {
    file << data[i] << std::endl;
  }

  file.close();

  return true;
}