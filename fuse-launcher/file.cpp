#include "file.h"
#include <iostream>
#include <fstream>
#include <filesystem>

const std::string CONFIG_FILE = "\\Profile-Data-Marvin.cfg";
const std::size_t PROFILE_PROPERTY_COUNT = 6;


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

int GetBotCount() {
  std::string path = GetWorkingDirectory() + CONFIG_FILE;
  std::ifstream file(path);
  std::string buffer;
  int count = 0;

  if (!file.is_open()) {
    std::cout << "Error: Could not open " << path << std::endl;
    std::cout << "This file must be in the same directory as fuse-launcher.exe" << std::endl;
    std::cin.get();
    return 0;
  }

  // ignore empty lines
  while (std::getline(file, buffer)) {
    if (buffer.empty()) continue;
    count++;
  }

  file.close();

  if (count % PROFILE_PROPERTY_COUNT != 0 || count < PROFILE_PROPERTY_COUNT) {
    std::cout << "Error: " << path << " is not configured correctly." << std::endl;
    std::cin.get();
    return 0;
  }

  return count / PROFILE_PROPERTY_COUNT;
}


std::vector<MarvinData> LoadProfileData() {

  std::vector<MarvinData> bot_data;
  MarvinData profile_data;
  std::vector<std::string> file_raw_data;
  std::string path = GetWorkingDirectory() + CONFIG_FILE;
  std::ifstream file(path);
  std::string buffer;

  if (!file.is_open()) {
    std::cout << "Error: Could not open " << path << std::endl; 
    std::cout << "This file must be in the same directory as fuse-launcher.exe" << std::endl;
    std::cin.get();
    return bot_data;
  }

  // copy file into a vector, ignoring empty lines
  while (std::getline(file, buffer)) {
    if (buffer.empty()) continue; 
    file_raw_data.push_back(buffer);
  }

  file.close();

  if (file_raw_data.size() % PROFILE_PROPERTY_COUNT != 0) {
    std::cout << "Error: " << path << " is not configured correctly." << std::endl;
    std::cin.get();
    return bot_data;
  }

  // trim down to just the data
  for (std::size_t i = 0; i < file_raw_data.size(); i++) {
    std::size_t pos = file_raw_data[i].find("=");
    
    if (pos == std::string::npos) {
      std::cout << "Error: " << path << " is not configured correctly." << std::endl;
      std::cin.get();
      return bot_data;
    }

    pos = file_raw_data[i].find_first_not_of(" \t\n\r", pos + 1);

     file_raw_data[i] = file_raw_data[i].substr(pos);  

     pos = file_raw_data[i].find_last_not_of(" \t\n\r\f\v");
     if (std::string::npos != pos) {
       file_raw_data[i].erase(pos + 1);
     } 
  }

  // copy data to struct in chunks
  for (std::size_t i = 0; i < file_raw_data.size(); i += PROFILE_PROPERTY_COUNT) {
    strcpy_s(profile_data.profile_data.name, file_raw_data[i].c_str());
    profile_data.name = file_raw_data[i];
    strcpy_s(profile_data.profile_data.password, file_raw_data[i + 1].c_str());
    profile_data.profile_data.ship = stoi(file_raw_data[i + 2]);
    profile_data.profile_data.window_mode = stoi(file_raw_data[i + 3]);
    strcpy_s(profile_data.profile_data.zone_name, file_raw_data[i + 4].c_str());
    strcpy_s(profile_data.profile_data.chats, file_raw_data[i + 5].c_str());

    bot_data.push_back(profile_data);
  }

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