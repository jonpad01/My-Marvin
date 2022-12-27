#pragma once

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <unordered_map>

#include "Hash.h"
#include "Vector2f.h"



namespace marvin {

class Map;

using RegionIndex = std::size_t;

constexpr static RegionIndex kUndefinedRegion = -1;

struct MapCoord {
  uint16_t x;
  uint16_t y;

  MapCoord(uint16_t x, uint16_t y) : x(x), y(y) {}
  MapCoord(Vector2f vec) : x((uint16_t)vec.x), y((uint16_t)vec.y) {}

  bool operator==(const MapCoord& other) const { return x == other.x && y == other.y; }
};

bool IsValidPosition(MapCoord coord);

}  // namespace marvin

MAKE_HASHABLE(marvin::MapCoord, t.x, t.y);

namespace marvin {

class RegionRegistry {
 public:
  RegionRegistry(const Map& map) : region_count_(0) { 
	memset(coord_regions_, 0xFF, sizeof(coord_regions_));
    memset(unordered_solids_, 0xFF, sizeof(unordered_solids_));
    memset(outside_edges_, 0xFF, sizeof(outside_edges_));
  }

  bool IsConnected(MapCoord a, MapCoord b) const;
  bool IsEdge(MapCoord coord) const;

  void CreateAll(const Map& map, float radius);
  void CreateRegions(const Map& map, std::vector<Vector2f> seed_points, float radius);

  void DebugUpdate(Vector2f position);

 private:
  bool IsRegistered(MapCoord coord) const;
  void Insert(MapCoord coord, RegionIndex index);
  std::size_t GetRegionIndex(MapCoord coord);

  RegionIndex CreateRegion();

  void FloodFillRegion(const Map& map, const MapCoord& coord, RegionIndex region_index, float radius);
  void FloodFillEmptyRegion(const Map& map, const MapCoord& coord, RegionIndex region_index, bool right_corner_check,
                            bool bottom_corner_check, float radius);
  void FloodFillSolidRegion(const Map& map, const MapCoord& coord, RegionIndex region_index);

  RegionIndex region_count_;

  RegionIndex coord_regions_[1024 * 1024];
  RegionIndex unordered_solids_[1024 * 1024];
  RegionIndex outside_edges_[1024 * 1024];
};
}  // namespace marvin
