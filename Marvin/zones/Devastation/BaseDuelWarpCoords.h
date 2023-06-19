#pragma once

#include <vector>
#include "..//..//Vector2f.h"
#include "..//..//TeamGoals.h"

namespace marvin {
namespace deva {

class BaseDuelWarpCoords : public TeamGoalCreator {
 public:
  BaseDuelWarpCoords(const std::string& mapName);

  bool FoundMapFiles() { return foundMapFiles; }
  bool HasCoords() { return !warps_.Empty(); }
  const TeamGoals& GetGoals() { return warps_; }

 private:
  std::string TrimExtension(const std::string& mapName);
  bool LoadBaseDuelFile(const std::string& mapName);
  bool LoadFile(std::ifstream& file);
  int GetIntMatch(std::ifstream& file, const std::string& match);

  bool foundMapFiles;
  TeamGoals warps_;
};

}  // namespace deva
}  // namespace marvin