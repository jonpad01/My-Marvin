
#include <fstream>
#include <filesystem>

#include "..//..//Common.h"
#include "BaseDuelSpawnCoords.h"

namespace marvin {
namespace deva {

BaseDuelSpawnCoords::BaseDuelSpawnCoords(const std::string& mapName) {
  foundMapFiles = false;
  LoadBaseDuelFile(mapName);
}

bool BaseDuelSpawnCoords::LoadBaseDuelFile(const std::string& mapName) {
  std::string trimmedName = TrimExtension(mapName);
  std::ifstream file;
  bool fileFound = false;
  bool result = false;
  spawns_.clear();

  // file name format is basewarp-XXX.ini where XXX = map name
  // searches the Continuum folder in C:\Program Files\ for the basewarp.ini files
  for (const auto& entry : std::filesystem::directory_iterator(GetWorkingDirectory())) {
    std::string path = entry.path().generic_string();
    std::size_t found = path.find("basewarp-");
    if (found != std::string::npos) {
      foundMapFiles = true;
      // subtract from path size the found position, length of "basewarp-" match (9),
      // and length of ".ini" file extension (4)
      std::size_t length = path.size() - (found + 9) - 4;
      std::string name = path.substr(found + 9, length);
      if (name == trimmedName) {
        file.open(path);
        fileFound = true;
        break;
      }
    }
  }

  if (fileFound) {
    if (LoadFile(file)) {
      result = true;
    }
  }
  file.close();
  return result;
}

bool BaseDuelSpawnCoords::LoadFile(std::ifstream& file) {
  if (!file.is_open()) {
    return false;
  }

  Vector2f temp;

  while (!file.eof() && file.good()) {
    temp.x = GetIntMatch(file, "X1=");
    temp.y = GetIntMatch(file, "Y1=");

    if (temp.x != -1 && temp.y != -1) {
      spawns_.t0.push_back(temp);
    }

    temp.x = GetIntMatch(file, "X2=");
    temp.y = GetIntMatch(file, "Y2=");

    if (temp.x != -1 && temp.y != -1) {
      spawns_.t1.push_back(temp);
    }
  }
  return true;
}

int BaseDuelSpawnCoords::GetIntMatch(std::ifstream& file, const std::string& match) {
  std::string input;
  int value = -1;

  while (!file.eof()) {
    getline(file, input);
    std::size_t found = input.find(match);
    if (found != std::string::npos) {
      input = input.substr(match.size(), input.size() - match.size());
      value = atoi(input.c_str());
      return value;
    }
  }
  return value;
}

std::string BaseDuelSpawnCoords::TrimExtension(const std::string& mapName) {
  std::size_t found = mapName.find(".lvl");
  if (found != std::string::npos) {
    return mapName.substr(0, mapName.size() - 4);
  }
}

}  // namespace deva
}  // namespace marvin