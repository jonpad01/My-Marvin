#pragma once

#include "..//..//TeamGoals.h"

namespace marvin {
namespace hs {

class HSFlagRooms : public TeamGoalCreator {
 public:
  HSFlagRooms();

  const TeamGoals& GetGoals();
  bool HasCoords();

 private:
  TeamGoals goals_;
};

}  // namespace hs
}  // namespace marvin