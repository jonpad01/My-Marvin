#include "BasePaths.h"

#include "Vector2f.h"
#include "path/Pathfinder.h"
#include "Map.h"
#include "TeamGoals.h"
#include "MapCoord.h"


namespace marvin {

BasePaths::BasePaths(const TeamGoals& spawns, float radius, path::Pathfinder& pathfinder, const Map& map) {
  current_index_ = 0;

  if (!spawns.t0.empty()) {
    BuildPaths(spawns.t0, spawns.t1, radius, pathfinder, map);
  } else {
    BuildPaths(spawns.entrances, spawns.flag_rooms, radius, pathfinder, map);
  }
}

void BasePaths::BuildPaths(const std::vector<MapCoord>& start, const std::vector<MapCoord>& end, float radius,
                           path::Pathfinder& pathfinder, const Map& map) {
  for (std::size_t i = 0; i < start.size(); i++) {
    Vector2f position_1 = start[i];
    Vector2f position_2 = end[i];

    std::vector<Vector2f> base_path = pathfinder.FindPath(map, position_1, position_2, radius);

    base_paths_.push_back(base_path);
  }
}

}  // namespace marvin