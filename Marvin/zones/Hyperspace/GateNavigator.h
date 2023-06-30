#pragma once
#include "../../Vector2f.h"
#include "../../MapCoord.h"
#include "../../RegionRegistry.h"


namespace marvin {
namespace hs {

	// the gate navigator will take a start position and a destination, and return the coordinate of the gate to move to
	// if in a connected area it will return the destination
class GateNavigator {
 public:
  GateNavigator(RegionRegistry& regions, Map& map);

  Vector2f GetWayPoint(Vector2f start, Vector2f end);
  Vector2f GetClosestCenterToTunnelGate(Vector2f pos);
  Vector2f GetClosestTunnelToCenterGate(Vector2f pos);

 private:
  RegionRegistry& regions;
  Map& map;

  // gates from center to tunnel
  const std::vector<MapCoord> kCenterToTunnelGates = {MapCoord(388, 395), MapCoord(572, 677)};
  // gates to go back to center
  const std::vector<MapCoord> kTunnelToCenterGates = {MapCoord(62, 351), MapCoord(961, 674)};
  // gates to 7 bases plus top area in order
  const std::vector<MapCoord> kTunnelToBaseGates = {MapCoord(961, 63),  MapCoord(960, 351), MapCoord(960, 960),
                                                  MapCoord(512, 959), MapCoord(64, 960),  MapCoord(64, 672),
                                                  MapCoord(65, 65),   MapCoord(512, 64)};
  // gates from bases to tunnel
  const std::vector<MapCoord> kBaseToTunnelGates = {MapCoord(888, 357),  MapCoord(771, 501), MapCoord(671, 852),
                                                  MapCoord(608, 756), MapCoord(150, 894),  MapCoord(261, 459),
                                                  MapCoord(136, 390),   MapCoord(618, 252)};
};

}
}  // namespace marvin