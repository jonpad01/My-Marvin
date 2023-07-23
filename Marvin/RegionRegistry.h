#pragma once

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <unordered_map>

#include "Hash.h"
#include "Vector2f.h"
#include "MapCoord.h"



namespace marvin {

class Map;

using RegionIndex = std::size_t;

constexpr static RegionIndex kUndefinedRegion = -1;

}  // namespace marvin

MAKE_HASHABLE(marvin::MapCoord, t.x, t.y);

namespace marvin {

 struct SharedRegionOwnership {
  static constexpr size_t kMaxOwners = 4;

  RegionIndex owners[kMaxOwners];
  size_t count;

  SharedRegionOwnership() : count(0) {}

  bool AddOwner(RegionIndex index) {
    if (count >= kMaxOwners) return false;
    if (HasOwner(index)) return false;

    owners[count++] = index;

    return true;
  }

  bool HasOwner(RegionIndex index) const {
    for (size_t i = 0; i < count; ++i) {
      if (owners[i] == index) {
        return true;
      }
    }

    return false;
  }
};

struct RegionFiller {
 public:
  RegionFiller(const Map& map, float radius, RegionIndex* coord_regions, SharedRegionOwnership* edges, int* region_tile_counts);

  void Fill(RegionIndex index, const MapCoord& coord) {
    this->region_index = index;

    FillEmpty(coord);
    FillSolid();

    highest_coord = MapCoord(9999, 9999);
  }

 private:
  void FillEmpty(const MapCoord& coord);
  void TraverseEmpty(const Vector2f& from, MapCoord to);

  void FillSolid();
  void TraverseSolid(const Vector2f& from, MapCoord to);

  bool IsEmptyBaseTile(const Vector2f& position) const;

  const Map& map;
  RegionIndex region_index;
  float radius;

  RegionIndex* coord_regions;
  SharedRegionOwnership* edges;
  int* region_tile_counts;

  MapCoord highest_coord;

  std::vector<RegionIndex> potential_edges;

  std::vector<MapCoord> stack;
};

class RegionRegistry {
 public:
  RegionRegistry(const Map& map) : region_count_(0) { 
      memset(coord_regions_, 0xFF, sizeof(coord_regions_));
    memset(region_tile_counts_, 0, sizeof(region_tile_counts_));
  }

  bool IsConnected(MapCoord a, MapCoord b) const;
  bool IsEdge(MapCoord coord) const;
  int GetTileCount(MapCoord coord) const;
  void Create(const Map& map, float radius, uint16_t xMin, uint16_t xMax);

  void DebugUpdate(Vector2f position);

 private:
  bool IsRegistered(MapCoord coord) const;
  void Insert(MapCoord coord, RegionIndex index);
  std::size_t GetRegionIndex(MapCoord coord);

  RegionIndex CreateRegion();

  RegionIndex region_count_;

  RegionIndex coord_regions_[1024 * 1024];
  SharedRegionOwnership outside_edges_[1024 * 1024];
  int region_tile_counts_[1024 * 1024];
};
}  // namespace marvin
