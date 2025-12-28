#pragma once
#include <vector>
#include <map>
#include <string>

#include "TeamGoals.h"



namespace marvin {

class Vector2f;
class Map;
struct PairedGoals;
struct MapCoord;

namespace path {
struct Pathfinder;
}


class BasePaths {
 public:
  BasePaths(const std::vector<TeamGoals>& spawns, float radius, path::Pathfinder& pathfinder, const Map& map);

  const std::vector<std::vector<Vector2f>>& GetBasePaths() { return base_paths_; }
  const std::vector<Vector2f>& GetPath() { return base_paths_[current_base_]; }
  std::size_t GetBase() { return current_base_; }
  void SetBase(std::size_t index) { current_base_ = index; }

 private:
  std::vector<std::vector<Vector2f>> base_paths_;
  std::size_t current_base_ = 0;
};

}  // namespace marvin
