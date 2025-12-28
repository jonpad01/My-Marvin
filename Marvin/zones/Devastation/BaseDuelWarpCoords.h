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

  bool HasCoords() { return !safes.empty(); }
  const std::vector<TeamGoals>& GetTeamGoals() { return safes; }

 private:
 
  bool ProcessMapRegions(const Map& map);
  bool LoadBaseDuelFile(const std::string& mapName);
  bool ReadBaseDuelFile(std::ifstream& file);

  std::vector<TeamGoals> safes;
  std::string mapName;
};

}  // namespace deva
}  // namespace marvin