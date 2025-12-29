#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <variant>

#include "MapCoord.h"

namespace marvin {

  // 2 sets is not needed but is more readable when working with specific zones
  // only 1 set will be used depending on what zone is loaded
  struct TeamGoals {
    // devastation set
    std::string base_name;
    MapCoord t0;
    MapCoord t1;
    // hyperspace set
    MapCoord hyper_gate;
    MapCoord flag_room;
  };

  class TeamGoalCreator {
  public:
    virtual bool HasCoords() = 0;
    virtual const std::vector<TeamGoals>& GetTeamGoals() = 0;
  };
}  // namespace marvin