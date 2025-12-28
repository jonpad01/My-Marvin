#pragma once

#include "..//..//TeamGoals.h"

namespace marvin {
namespace hs {

class HSFlagRooms : public TeamGoalCreator {
 public:
  HSFlagRooms();

  //const std::unordered_map<std::string, GoalPairs>& GetTeamGoals();
  const std::vector<TeamGoals>& GetTeamGoals();
  bool HasCoords();

 private:
   //std::unordered_map<std::string, GoalPairs> goals_;
  std::vector<TeamGoals> goals_;
};

}  // namespace hs
}  // namespace marvin