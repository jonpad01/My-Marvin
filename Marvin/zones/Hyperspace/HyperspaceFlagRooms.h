#pragma once

#include "..//..//TeamGoals.h"

namespace marvin {
namespace hs {

class HSFlagRooms : public TeamGoalCreator {
 public:
  HSFlagRooms();

  const std::vector<TeamGoals>& GetTeamGoals();
  bool HasCoords();

 private:
  std::vector<TeamGoals> goals_;
};

}  // namespace hs
}  // namespace marvin