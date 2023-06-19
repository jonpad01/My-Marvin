#include "TeamGoals.h"
#include "MapCoord.h"

namespace marvin {

// only 1 of the 2 sets will be used at any time so just check if all are empty
bool TeamGoals::Empty() {
  return t0.empty() && t1.empty() && entrances.empty() && flag_rooms.empty();
}

void TeamGoals::Clear() {
  t0.clear();
  t1.clear();
  entrances.clear();
  flag_rooms.clear();
}

}  // namespace marvin