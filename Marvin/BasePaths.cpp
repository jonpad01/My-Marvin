#include "BasePaths.h"

#include "Vector2f.h"
#include "path/Pathfinder.h"
#include "Map.h"
//#include "TeamGoals.h"
#include "MapCoord.h"


namespace marvin {

  BasePaths::BasePaths(const std::vector<TeamGoals>& spawns, float radius, path::Pathfinder& pathfinder, const Map& map) {

    for (const auto& coords : spawns) {
      Vector2f position_1 = coords.t0;
      Vector2f position_2 = coords.t1;

      if (coords.t0.x == 0 && coords.t0.y == 0) {
        position_1 = coords.hyper_gate;
        position_2 = coords.flag_room;
      }

      std::vector<Vector2f> base_path = pathfinder.FindPath(map, position_1, position_2, radius);
      base_paths_.push_back(base_path);
    }
  }

}  // namespace marvin