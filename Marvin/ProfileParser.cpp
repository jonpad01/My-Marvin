#include "ProfileParser.h"
#include <vector>
#include <fstream>
#include <windows.h>
#include "Debug.h"


namespace marvin {
const std::string CONFIG_FILE = "\\Profile-Data-Marvin.cfg";
const std::size_t PROFILE_PROPERTY_COUNT = 6;

std::string ProfileParser::GetWorkingDirectory() {
  std::string directory;

  directory.resize(GetCurrentDirectoryA(0, NULL));
  GetCurrentDirectoryA(directory.size(), &directory[0]);

  return directory.substr(0, directory.size() - 1);
}

bool ProfileParser::GetStringData(std::vector<std::string>& data) {
  data.clear();
  std::string path = GetWorkingDirectory() + CONFIG_FILE;
  std::ifstream file(path);
  std::string buffer;

  if (!file.is_open()) {
    return false;
  }

  // copy file into a vector, ignoring empty lines
  while (std::getline(file, buffer)) {
    if (buffer.empty()) continue;
    if (buffer[0] == ';') continue;
    data.push_back(buffer);
  }

  file.close();

  if (data.size() % PROFILE_PROPERTY_COUNT != 0) {
    return false;
  }

  // trim down to just the data
  for (std::size_t i = 0; i < data.size(); i++) {
    std::size_t pos = data[i].find("=");

    if (pos == std::string::npos) return false;

    pos = data[i].find_first_not_of(" \t\n\r", pos + 1);

    data[i] = data[i].substr(pos);

    pos = data[i].find_last_not_of(" \t\n\r\f\v");
    if (std::string::npos != pos) data[i].erase(pos + 1);
  }

  return true;
}


bool ProfileParser::GetProfileNames(std::vector<std::string>& list) {
  std::vector<std::string> data;

  bool result = GetStringData(data);

  if (!result) return false;

  for (std::size_t i = 0; i < data.size(); i++) {
    if (i % 6 != 0) continue;

    list.push_back(data[i]);
  }

  return true;
}



bool ProfileParser::GetProfileData(uint32_t index, ProfileData& profile_data) {
  std::vector<std::string> raw_data;

  bool result = GetStringData(raw_data);

  if (!result) return false;


  // index 0 needs lines 1-6, index 1 lines 7-12 are needed
  std::size_t required_size = (index + 1) * PROFILE_PROPERTY_COUNT;
  std::size_t start = index * PROFILE_PROPERTY_COUNT;

  if (raw_data.size() < required_size) return false;

  // select data using the index
  profile_data.name = raw_data[start];
  profile_data.password = raw_data[start + 1];
  profile_data.ship = stoi(raw_data[start + 2]);
  profile_data.window_mode = stoi(raw_data[start + 3]);
  profile_data.zone_name = raw_data[start + 4];
  profile_data.chats = raw_data[start + 5];

  return true;
}
}  // namespace marvin