#include "HyperspaceFlagRooms.h"
#include <vector>
#include "..//..//MapCoord.h"


namespace marvin {
namespace hs {


HSFlagRooms::HSFlagRooms() {
#if 0 
  // last coord in vector is base 8, its just a pos near the gate
  goals_.entrances = {MapCoord(827, 339), MapCoord(811, 530), MapCoord(729, 893), MapCoord(444, 757),
                      MapCoord(127, 848), MapCoord(268, 552), MapCoord(181, 330), MapCoord(618, 230)};

  // last coord is base 8, its just a coord near buying area
  goals_.flag_rooms = {MapCoord(826, 229), MapCoord(834, 540), MapCoord(745, 828), MapCoord(489, 832),
                       MapCoord(292, 812), MapCoord(159, 571), MapCoord(205, 204), MapCoord(375,235)};
#endif
}

const std::vector<TeamGoals>& HSFlagRooms::GetTeamGoals() {
  return goals_;
}

bool HSFlagRooms::HasCoords() {
  return true;
}

}  // namespace hs
}  // namespace marvin