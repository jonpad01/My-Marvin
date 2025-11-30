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

bool ProfileParser::LoadProfileData() {
  std::vector<std::string> file_raw_data;
  std::string path = GetWorkingDirectory() + CONFIG_FILE;
  std::ifstream file(path);
  std::string buffer;

  if (!file.is_open()) {
    return false;
  }

  // copy file into a vector, ignoring empty lines
  while (std::getline(file, buffer)) {
    if (buffer.empty()) continue;
    file_raw_data.push_back(buffer);
  }

  file.close();

  if (file_raw_data.size() % PROFILE_PROPERTY_COUNT != 0) {
    return false;
  }

  // trim down to just the data
  for (std::size_t i = 0; i < file_raw_data.size(); i++) {
    std::size_t pos = file_raw_data[i].find("=");

    if (pos == std::string::npos) {
      return false;
    }

    pos = file_raw_data[i].find_first_not_of(" \t\n\r", pos + 1);

    file_raw_data[i] = file_raw_data[i].substr(pos);

    pos = file_raw_data[i].find_last_not_of(" \t\n\r\f\v");
    if (std::string::npos != pos) {
      file_raw_data[i].erase(pos + 1);
    }
  }

  // index 0 needs lines 1-6, index 1 lines 7-12 are needed
  std::size_t required_size = (profile_index + 1) * PROFILE_PROPERTY_COUNT;
  std::size_t start = profile_index * PROFILE_PROPERTY_COUNT;
  if (file_raw_data.size() < required_size) {
      return false;
  }

  // select data using the index
  profile_data.name = file_raw_data[start];
  profile_data.password = file_raw_data[start + 1];
  profile_data.ship = stoi(file_raw_data[start + 2]);
  profile_data.window_mode = stoi(file_raw_data[start + 3]);
  profile_data.zone_name = file_raw_data[start + 4];
  profile_data.chats = file_raw_data[start + 5];

  return true;
}
}  // namespace marvin