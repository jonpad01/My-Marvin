#include "HyperGateNavigator.h"
#include "../../Debug.h"

namespace marvin {
namespace hs {

    HyperGateNavigator::HyperGateNavigator(RegionRegistry& regions) : regions(regions) {}

    Vector2f HyperGateNavigator::GetClosestCenterToTunnelGate(Vector2f pos) {
      float distance0 = pos.Distance(kCenterToTunnelGates[0]);
      float distance1 = pos.Distance(kCenterToTunnelGates[1]);

      if (distance0 < distance1) {
        return kCenterToTunnelGates[0];
      }
      return kCenterToTunnelGates[1];
    }

    Vector2f HyperGateNavigator::GetClosestTunnelToCenterGate(Vector2f pos) {
      float distance0 = pos.Distance(kTunnelToCenterGates[0]);
      float distance1 = pos.Distance(kTunnelToCenterGates[1]);

      if (distance0 < distance1) {
        return kTunnelToCenterGates[0];
      }
      return kTunnelToCenterGates[1];
    }

    Vector2f HyperGateNavigator::GetWayPoint(Vector2f start, Vector2f end) {
      if (regions.IsConnected(start, end)) {
        return end;
      }

      bool start_in_center = regions.IsConnected(start, MapCoord(512, 512));
      bool start_in_tunnel = regions.IsConnected(start, kTunnelToCenterGates[0]);
      bool start_in_base = false;
      std::size_t start_base_index = 0;
      bool end_in_center = regions.IsConnected(end, MapCoord(512, 512));
      bool end_in_tunnel = regions.IsConnected(end, kTunnelToCenterGates[0]);
      bool end_in_base = false;
      std::size_t end_base_index = 0;

      for (std::size_t i = 0; i < kBaseToTunnelGates.size(); i++) {
        if (regions.IsConnected(start, kBaseToTunnelGates[i])) {
          start_in_base = true;
          start_base_index = i;
        }
        if (regions.IsConnected(end, kBaseToTunnelGates[i])) {
          end_in_base = true;
          end_base_index = i;
        }
      }


      // the two positions arent in the same region
      
      if (start_in_center) {
        // start pos is in center so just head for a gate
        return GetClosestCenterToTunnelGate(start);
      } else if (start_in_tunnel) {
        if (end_in_center) {
          return GetClosestTunnelToCenterGate(start);
        } else if (end_in_base) {
          return kTunnelToBaseGates[end_base_index];
        }
      // just head to the base gate
      } else if (start_in_base) {
        return kBaseToTunnelGates[start_base_index];
      }

      // something broke if it got here
      return start;
    }



}  // namespace hs
}  // namespace marvin