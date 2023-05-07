#pragma once
#include <vector>




namespace marvin {
class Vector2f;
class Map;
namespace path {
struct Pathfinder;
}
namespace deva {

	struct BaseWarps;

class BasePaths {
 public:
  BasePaths(const BaseWarps& spawns, float radius, path::Pathfinder& pathfinder, const Map& map);

  const std::vector<std::vector<Vector2f>>& GetBasePaths() { return base_paths_; }
  const std::vector<Vector2f>& GetBasePath(std::size_t index) { return base_paths_[index]; }

 private:
  std::vector<std::vector<Vector2f>> base_paths_;
};
}  // namespace deva
}  // namespace marvin
