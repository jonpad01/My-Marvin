
#include <fstream>
#include <filesystem>

#include "..//..//Common.h"
#include "BaseDuelWarpCoords.h"
#include "..//..//MapCoord.h"
#include "..//..//Map.h"
#include "..//..//Debug.h"

namespace marvin {
namespace deva {

BaseDuelWarpCoords::BaseDuelWarpCoords(const Map& map, const std::string& mapName) : mapName(mapName) {

    // process map for regions that point to base safe tiles
  if (map.HasRegions()) {
    //log.Write("has regions");
  }


  foundMapFiles = false;
  LoadBaseDuelFile(mapName);
}

// TODO:  check timestamps on existing file
bool BaseDuelWarpCoords::CheckFiles() {
  
  if (HasCoords()) return false;

  // might be resource heave to constantly check this
  LoadBaseDuelFile(mapName);
  return false;
}

bool BaseDuelWarpCoords::LoadBaseDuelFile(const std::string& mapName) {
  std::string trimmedName = TrimExtension(mapName);
  std::ifstream file;
  bool fileFound = false;
  bool result = false;
  safes.Clear();

  // file name format is basewarp-XXX.ini where XXX = map name
  // searches the Continuum folder in C:\Program Files\ for the basewarp.ini files
  for (const auto& entry : std::filesystem::recursive_directory_iterator(GetWorkingDirectory())) {
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

bool BaseDuelWarpCoords::LoadFile(std::ifstream& file) {
  if (!file.is_open()) {
    return false;
  }

  MapCoord temp;

  while (!file.eof() && file.good()) {
    temp.x = GetIntMatch(file, "X1=");
    temp.y = GetIntMatch(file, "Y1=");

    if (temp.x != -1 && temp.y != -1) {
      safes.t0.push_back(temp);
    }

    temp.x = GetIntMatch(file, "X2=");
    temp.y = GetIntMatch(file, "Y2=");

    if (temp.x != -1 && temp.y != -1) {
      safes.t1.push_back(temp);
    }
  }
  return true;
}

int BaseDuelWarpCoords::GetIntMatch(std::ifstream& file, const std::string& match) {
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

std::string BaseDuelWarpCoords::TrimExtension(const std::string& mapName) {
  std::size_t found = mapName.find(".lvl");
  if (found != std::string::npos) {
    return mapName.substr(0, mapName.size() - 4);
  }
  return mapName;
}

}  // namespace deva
}  // namespace marvin