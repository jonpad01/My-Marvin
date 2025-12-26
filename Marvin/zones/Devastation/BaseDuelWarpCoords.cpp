
#include <fstream>
#include <filesystem>
#include <regex>
#include <map>

#include "..//..//Common.h"
#include "BaseDuelWarpCoords.h"
#include "..//..//MapCoord.h"
#include "..//..//Map.h"
#include "..//..//Debug.h"

namespace marvin {
namespace deva {

BaseDuelWarpCoords::BaseDuelWarpCoords(const Map& map, const std::string& mapName) : mapName(mapName) {
  foundMapFiles = false;
  
  // process map for regions that point to base safe tiles
 // if (map.HasRegions()) {
  //  ProcessMapRegions(map);
//  } else {
    LoadBaseDuelFile(mapName);  // fall back to baseduel files
//  } 
}

bool BaseDuelWarpCoords::ProcessMapRegions(const Map& map) {
  /*
   * looking for region names that fit this format EG1_0, EG1_1
   *
   * EG : Extreme Games arena (Devastation has many baseduel style arenas)
   * 1  : Base number (Devastation uses 50+ bases per map)
   * _  : Spacer
   * 0  : Team 0  (there are only two teams, 1 and 0)
   *
   * These regions are used to flag the safe tiles for each team.
   * These tiles are also the warp points used by the baseduel bot.
   *
   * Each region will only have 1 coordinate
   * Each Base will have 2 regions, one for each team
   */

  const std::unordered_map<std::string, std::bitset<1024 * 1024>>& uMap = map.GetRegions();

  for (const auto& [name, tiles] : uMap) {
    // test for the correct pattern
    std::regex base_pattern("^[a-zA-Z0-9]+?([0-9]+)_([0-1])$");
    std::smatch matches;

    // bad match
    if (!std::regex_search(name, matches, base_pattern)) continue;

    // parse team from string
    std::size_t base = std::stoi(matches[1].str());
    int team = std::stoi(matches[2].str());

    // the bitset should only have 1 coordinate so grab the first match
    for (std::size_t i = 0; i < tiles.size(); ++i) {
      if (!tiles.test(i)) continue;

      MapCoord coord{uint16_t(i % 1024), uint16_t(i / 1024)};

      // place them in order
      // probably a slow and wasteful method
      if (team == 0) {
        if (base >= safes.t0.size()) safes.t0.resize(base + 1);
        safes.t0[base] = coord;
      } else {
        if (base >= safes.t1.size()) safes.t1.resize(base + 1);
        safes.t1[base] = coord;
      }

      break;
    }
  }

  // there is no base 0, so index 0 is just MapCoord(0,0)
  // if it is preserved then the index would match base numbers
  // but im deleting it in case there are conflicts with other code that reads it
  safes.t0.erase(safes.t0.begin());
  safes.t1.erase(safes.t1.begin());

  return true;
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
  if (!file) return false;

  MapCoord t0{}, t1{};
  bool has_x1 = false, has_y1 = false;
  bool has_x2 = false, has_y2 = false;

  std::string line;

  while (std::getline(file, line)) {

    auto parse = [](const std::string& s, const char* key, uint16_t& out) {
      auto pos = s.find(key);
      if (pos == std::string::npos) return false;

      out = static_cast<uint16_t>(std::stoi(s.substr(pos + std::strlen(key))));
      return true;
    };

    if (parse(line, "X1=", t0.x)) has_x1 = true;
    if (parse(line, "Y1=", t0.y)) has_y1 = true;

    if (parse(line, "X2=", t1.x)) has_x2 = true;
    if (parse(line, "Y2=", t1.y)) has_y2 = true;

    // When we have a full pair, store and reset
    if (has_x1 && has_y1) {
      safes.t0.push_back(t0);
      has_x1 = has_y1 = false;
    }

    if (has_x2 && has_y2) {
      safes.t1.push_back(t1);
      has_x2 = has_y2 = false;
    }
  }

  return true;
}

#if 0 

bool BaseDuelWarpCoords::LoadFile(std::ifstream& file) {
  if (!file.is_open()) return false;
  
  MapCoord temp;

  while (!file.eof() && file.good()) {
    temp.x = GetIntMatch(file, "X1=");
    temp.y = GetIntMatch(file, "Y1=");

    if (temp.x != 65535 && temp.y != 65535) {
      safes.t0.push_back(temp);
    }

    temp.x = GetIntMatch(file, "X2=");
    temp.y = GetIntMatch(file, "Y2=");

    if (temp.x != 65535 && temp.y != 65535) {
      safes.t1.push_back(temp);
    }
  }
  return true;
}

uint16_t BaseDuelWarpCoords::GetIntMatch(std::ifstream& file, const std::string& match) {
  std::string input;
  uint16_t value = 65535;  // max value, signaling bad data

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

#endif

std::string BaseDuelWarpCoords::TrimExtension(const std::string& mapName) {
  std::size_t found = mapName.find(".lvl");
  if (found != std::string::npos) {
    return mapName.substr(0, mapName.size() - 4);
  }
  return mapName;
}

}  // namespace deva
}  // namespace marvin