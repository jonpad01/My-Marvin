#include "HyperspaceFlagRooms.h"
#include <vector>
#include "..//..//MapCoord.h"


namespace marvin {
namespace hs {

HSFlagRooms::HSFlagRooms() {
  // base entrances
  goals_.entrances = {MapCoord(827, 339), MapCoord(811, 530), MapCoord(729, 893), MapCoord(444, 757),
                      MapCoord(127, 848), MapCoord(268, 552), MapCoord(181, 330)};

  // flag rooms
  goals_.flag_rooms = {MapCoord(826, 229), MapCoord(834, 540), MapCoord(745, 828), MapCoord(489, 832),
                       MapCoord(292, 812), MapCoord(159, 571), MapCoord(205, 204)};
}

const TeamGoals& HSFlagRooms::GetGoals() {
  return goals_;
}

bool HSFlagRooms::HasCoords() {
  return true;
}

}  // namespace hs
}  // namespace marvin