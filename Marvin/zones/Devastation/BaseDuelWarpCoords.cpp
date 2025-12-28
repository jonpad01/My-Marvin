
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
  
  bool result = false;
  //safes.t1.reserve(200);
  //safes.t0.reserve(200);

  // process map for regions that point to base safe tiles
  if (map.HasRegions()) {
    result = ProcessMapRegions(map);
  }

  if (!result) {
     LoadBaseDuelFile(mapName);  // fall back to baseduel files
  }
}

bool BaseDuelWarpCoords::ProcessMapRegions(const Map& map) {
  /*
   * looking for region names that fit this format EG1_0, EG1_1, #_1, @_0
   *
   * EG1 : Anything before the underscore is just a string name
   * _  : Spacer
   * 0  : Team 0  (there are only two teams, 1 and 0)
   *
   * These regions are used to flag the safe tiles for each team.
   * These tiles are also the warp points used by the baseduel bot.
   *
   * Each region will only have 1 coordinate
   * Each Base will have 2 regions, one for each team
   */

  //coords.clear();
  //safes = std::make_unique<std::vector<DevastationBDSafes>>();

  safes.clear();
  std::map<std::string, TeamGoals> temp;

  const std::unordered_map<std::string, std::bitset<1024 * 1024>>& uMap = map.GetRegions();

  //int bases = 0;
  //PairedCoord key;

  for (const auto& [region_name, tiles] : uMap) {

    std::size_t pos = region_name.find("_");

    if (pos == std::string::npos || pos + 2 != region_name.size()) continue;
    if (region_name[pos + 1] != '0' && region_name[pos + 1] != '1') continue;

   
    std::string base_name = region_name.substr(0, pos);
    int team = region_name[pos + 1] - '0';

    // the bitset should only have 1 coordinate so grab the first match
    for (std::size_t i = 0; i < tiles.size(); ++i) {
      if (!tiles.test(i)) continue;

      MapCoord coord{ uint16_t(i % 1024), uint16_t(i / 1024) };

      temp[base_name].base_name = base_name;

      if (team == 0) {
        //safes.t0.push_back(coord);
        temp[base_name].t0 = coord;
        //bases++;
      } else {
        //safes.t1.push_back(coord);
        temp[base_name].t1 = coord;
      }

      break;
    }
  }

  // copy into permanent storage type
  for (const auto& [name, coords] : temp) {
    safes.push_back(coords);
  }

  //if (bases > 0) {
    // there is no base 0, so index 0 is just MapCoord(0,0)
    // if it is preserved then the index would match base numbers
    // but im deleting it now in case there are conflicts with other code that reads it
    //safes.t0.erase(safes.t0.begin());
   // safes.t1.erase(safes.t1.begin());
  //}

  return !safes.empty();
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
  //safes.Clear();
  safes.clear();

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

// TODO: Read the correct base name from basewarp file
bool BaseDuelWarpCoords::LoadFile(std::ifstream& file) {
  if (!file) return false;

  MapCoord t0{}, t1{};
  bool has_x1 = false, has_y1 = false;
  bool has_x2 = false, has_y2 = false;

  std::string line;
  uint16_t generic_key = 0;

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

    if (has_x1 && has_y1 && has_x2 && has_y2) {
      std::string base_name = std::to_string(generic_key);
      
      TeamGoals temp;
      temp.t0 = t0;
      temp.t1 = t1;
      temp.base_name = base_name;

      safes.push_back(temp);

      //coords[base_name].t0 = t0;
      //coords[base_name].t1 = t1;
      generic_key++;
      has_x1 = has_y1 = has_x2 = has_y2 = false;
    }

    // When we have a full pair, store and reset
   // if (has_x1 && has_y1) {
   //   safes.t0.push_back(t0);
    //  has_x1 = has_y1 = false;
   // }

    //if (has_x2 && has_y2) {
     // safes.t1.push_back(t1);
     // has_x2 = has_y2 = false;
   // }
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