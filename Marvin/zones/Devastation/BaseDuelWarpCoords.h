#pragma once

#include <vector>
#include "..//..//Vector2f.h"
#include "..//..//TeamGoals.h"

namespace marvin {
class Map;
namespace deva {

class BaseDuelWarpCoords : public TeamGoalCreator {
 public:
  BaseDuelWarpCoords(const Map& map, const std::string& mapName);

  bool CheckFiles();
  bool FoundMapFiles() { return foundMapFiles; }
  bool HasCoords() { return !safes.Empty(); }
  const TeamGoals& GetGoals() { return safes; }

 private:
  std::string TrimExtension(const std::string& mapName);
  bool LoadBaseDuelFile(const std::string& mapName);
  bool LoadFile(std::ifstream& file);
  int GetIntMatch(std::ifstream& file, const std::string& match);

  bool foundMapFiles;
  TeamGoals safes;
  std::string mapName;
};

}  // namespace deva
}  // namespace marvin