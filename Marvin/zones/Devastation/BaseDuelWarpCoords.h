#pragma once

#include <vector>
#include <map>
#include "..//..//Vector2f.h"
#include "..//..//TeamGoals.h"

namespace marvin {
class Map;
namespace deva {



class BaseDuelWarpCoords : public TeamGoalCreator {
 public:
  BaseDuelWarpCoords(const Map& map, const std::string& mapName);

  bool CheckFiles();
  //bool FoundMapFiles() { return foundMapFiles; }
  bool HasCoords() { return !safes.empty(); }
   //bool HasCoords() { return !coords.empty(); }
  const std::vector<TeamGoals>& GetTeamGoals() { return safes; }
  //const std::unordered_map<std::string, GoalPairs>& GetTeamGoals() { return coords; }


 private:
  std::string TrimExtension(const std::string& mapName);
  bool LoadBaseDuelFile(const std::string& mapName);
  bool ProcessMapRegions(const Map& map);
  bool LoadFile(std::ifstream& file);
  uint16_t GetIntMatch(std::ifstream& file, const std::string& match);

  bool foundMapFiles;
  std::vector<TeamGoals> safes;
  std::string mapName;
};

}  // namespace deva
}  // namespace marvin