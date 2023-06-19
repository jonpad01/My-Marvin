#pragma once
#include <vector>


namespace marvin {

struct MapCoord;

// 2 sets is not needed but is more readable when working with specific zones
// only 1 set will be used depending on what zone is loaded
struct TeamGoals {
  bool Empty();
  void Clear();

  // paired set for devastation
  std::vector<MapCoord> t0;
  std::vector<MapCoord> t1;

  // paired set for hyperspace
  std::vector<MapCoord> entrances;
  std::vector<MapCoord> flag_rooms;
};

class TeamGoalCreator {
 public:
  virtual bool HasCoords() = 0;
  virtual const TeamGoals& GetGoals() = 0;
};
}  // namespace marvin