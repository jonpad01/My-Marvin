#pragma once
#include <vector>




namespace marvin {
class Vector2f;
class Map;
struct TeamGoals;

namespace path {
struct Pathfinder;
}


class BasePaths {
 public:
  BasePaths(const TeamGoals& spawns, float radius, path::Pathfinder& pathfinder, const Map& map);

  //const std::vector<std::vector<Vector2f>>& GetBasePaths() { return base_paths_; }
  const std::vector<Vector2f>& GetBasePath() { return base_paths_[current_index_]; }
  std::size_t GetBaseIndex() { return current_index_; }
  void SetBase(std::size_t index) { current_index_ = index; }

 private:
  std::vector<std::vector<Vector2f>> base_paths_;
  std::size_t current_index_;
};

}  // namespace marvin
