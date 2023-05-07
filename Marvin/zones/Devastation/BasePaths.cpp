#include "BasePaths.h"

#include "..//..//Vector2f.h"
#include "..//..//path/Pathfinder.h"
#include "..//..//Map.h"
#include "BaseDuelWarpCoords.h"


namespace marvin {
namespace deva {

BasePaths::BasePaths(const BaseWarps& spawns, float radius, path::Pathfinder& pathfinder, const Map& map) {

  for (std::size_t i = 0; i < spawns.t0.size(); i++) {
    Vector2f position_1 = spawns.t0[i];
    Vector2f position_2 = spawns.t1[i];

    std::vector<Vector2f> base_path = pathfinder.FindPath(map, std::vector<Vector2f>(), position_1, position_2, radius);

    base_paths_.push_back(base_path);
  }

}
}  // namespace deva
}  // namespace marvin