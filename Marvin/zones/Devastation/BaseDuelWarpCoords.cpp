
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
  
  safes.reserve(200);
  bool result = false;

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
   * Region names fit this format EG1_0, EG1_1, #_1, @_0 ASWZ_2_0  ASWZ_2_1
   *
   * EG1, @, ASWZ_2 : Anything before the last underscore is just a string name
   * _              : Spacer
   * 0, 1           : Team  (there are only two teams, 1 and 0)
   *
   * These regions are used to flag the safe tiles for each team.
   * These tiles are also the warp points used by the baseduel bot.
   *
   * Each region will only have 1 coordinate
   * Each Base will have 2 regions, one for each team
   */

  safes.clear();
  // need to pair two regions that belong to the same base
  // using a map to insert them together
  std::map<std::string, TeamGoals> temp;

  const std::unordered_map<std::string, std::bitset<1024 * 1024>>& uMap = map.GetRegions();

  for (const auto& [region_name, tiles] : uMap) {

    // find the last instance of "_"
    std::size_t pos = region_name.rfind('_');

    // make sure the next char is the end of the string and only 0 or 1
    if (pos == std::string::npos || pos + 2 != region_name.size()) continue;
    if (region_name[pos + 1] != '0' && region_name[pos + 1] != '1') continue;

    std::string base_name = region_name.substr(0, pos);
    uint16_t team = region_name[pos + 1] - '0';

    // the bitset should only have 1 coordinate so grab the first match
    for (std::size_t i = 0; i < tiles.size(); ++i) {
      if (!tiles.test(i)) continue;

      MapCoord coord{ uint16_t(i % 1024), uint16_t(i / 1024) };

      temp[base_name].base_name = base_name;

      team == 0 ? temp[base_name].t0 = coord : temp[base_name].t1 = coord;

      break;
    }
  }

  // copy into permanent storage
  for (const auto& [name, coords] : temp) {
    safes.push_back(coords);
  }

  return !safes.empty();
}

bool BaseDuelWarpCoords::LoadBaseDuelFile(const std::string& mapName) {

  const std::string kPrefix = "basewarp-";
  const std::string kExtension = ".ini";

  std::string trimmedName = mapName.substr(0, mapName.size() - 4);

  std::ifstream file;
  bool fileFound = false;
  bool result = false;

  

  // file name format is basewarp-XXX.ini where XXX = map name
  // searches the Continuum folder for the basewarp.ini files
  for (const auto& entry : std::filesystem::recursive_directory_iterator(GetWorkingDirectory())) {

    std::string path = entry.path().generic_string();
    std::size_t pos = path.rfind(kPrefix);

    if (pos == std::string::npos || !path.ends_with(kExtension)) continue;

    std::size_t length = path.size() - (pos + kPrefix.size()) - kExtension.size();
    std::string name = path.substr(pos + kPrefix.size(), length);

    if (name == trimmedName) {
      file.open(path);
      fileFound = true;
      break;
    }
  }

  if (fileFound) {
    if (ReadBaseDuelFile(file)) {
      result = true;
    }
  }

  file.close();

  return result;
}

// TODO: Read the correct base name from basewarp file
bool BaseDuelWarpCoords::ReadBaseDuelFile(std::ifstream& file) {
  if (!file) return false;
  safes.clear();

  TeamGoals temp;
  bool has_x1 = false, has_y1 = false;
  bool has_x2 = false, has_y2 = false;
  bool has_name = false;

  std::string line;

  while (std::getline(file, line)) {

    auto ParseCoord = [](const std::string& s, const char* key, uint16_t& out) {
      auto pos = s.find(key);
      if (pos == std::string::npos) return false;

      out = static_cast<uint16_t>(std::stoi(s.substr(pos + std::strlen(key))));
      return true;
    };

    auto ParseName = [](const std::string& s, const char* key, std::string& out) {
      auto pos = s.find(key);
      if (pos == std::string::npos) return false;

      out = s.substr(pos + std::strlen(key)); // name but with ']' still at the end

      pos = out.rfind(']');
      if (pos == std::string::npos) return false;

      out = out.substr(0, pos);
      return true;
     };

    if (ParseName(line, "[base-", temp.base_name)) has_name = true;

    if (ParseCoord(line, "X1=", temp.t0.x)) has_x1 = true;
    if (ParseCoord(line, "Y1=", temp.t0.y)) has_y1 = true;

    if (ParseCoord(line, "X2=", temp.t1.x)) has_x2 = true;
    if (ParseCoord(line, "Y2=", temp.t1.y)) has_y2 = true;

    if (has_name && has_x1 && has_y1 && has_x2 && has_y2) {

      safes.push_back(temp);
      has_name = has_x1 = has_y1 = has_x2 = has_y2 = false;
    }
  }

  return !safes.empty();
}


}  // namespace deva
}  // namespace marvin