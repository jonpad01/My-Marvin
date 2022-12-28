#include "RegionRegistry.h"

#include <vector>

#include "Debug.h"
#include "Map.h"

namespace marvin {

RegionFiller::RegionFiller(const Map& map, float radius, RegionIndex* coord_regions, SharedRegionOwnership* edges)
    : map(map), radius(radius), coord_regions(coord_regions), edges(edges), highest_coord(9999, 9999) {
  potential_edges.reserve(1024 * 1024);

  for (size_t i = 0; i < 1024 * 1024; ++i) {
    potential_edges.push_back(kUndefinedRegion);
  }
}

void RegionFiller::FillEmpty(const MapCoord& coord) {
  if (!map.CanOverlapTile(Vector2f(coord.x, coord.y), radius)) return;

  coord_regions[coord.y * 1024 + coord.x] = region_index;

  stack.push_back(coord);

  while (!stack.empty()) {
    MapCoord current = stack.back();
    stack.pop_back();

    const MapCoord west(current.x - 1, current.y);
    const MapCoord east(current.x + 1, current.y);
    const MapCoord north(current.x, current.y - 1);
    const MapCoord south(current.x, current.y + 1);
    const Vector2f current_pos((float)current.x + 0.5f, (float)current.y + 0.5f);

    TraverseEmpty(current_pos, west);
    TraverseEmpty(current_pos, east);
    TraverseEmpty(current_pos, north);
    TraverseEmpty(current_pos, south);
  }
}

void RegionFiller::TraverseEmpty(const Vector2f& from, MapCoord to) {
  if (!IsValidPosition(to)) return;

  size_t to_index = (size_t)to.y * 1024 + to.x;

  if (!map.CanOccupyRadius(Vector2f(to.x, to.y), radius)) {
    potential_edges[to_index] = region_index;

    if (to.y < highest_coord.y) {
      highest_coord = to;
    }
  }

  if (coord_regions[to_index] == kUndefinedRegion) {
    Vector2f to_pos((float)to.x + 0.5f, (float)to.y + 0.5f);

    if (map.CanTraverse(from, to_pos, radius)) {
      coord_regions[to_index] = region_index;
      stack.push_back(to);
    }
  }
}

void RegionFiller::FillSolid() {
  MapCoord coord = highest_coord;

  if (!IsValidPosition(Vector2f(coord.x, coord.y))) return;

  stack.clear();

  stack.push_back(coord);

  while (!stack.empty()) {
    MapCoord current = stack.back();
    Vector2f current_pos((float)current.x, (float)current.y);

    stack.pop_back();

    const MapCoord west(current.x - 1, current.y);
    const MapCoord northwest(current.x - 1, current.y - 1);
    const MapCoord southwest(current.x - 1, current.y + 1);
    const MapCoord east(current.x + 1, current.y);
    const MapCoord northeast(current.x + 1, current.y - 1);
    const MapCoord southeast(current.x + 1, current.y + 1);
    const MapCoord north(current.x, current.y - 1);
    const MapCoord south(current.x, current.y + 1);

    TraverseSolid(current_pos, west);
    TraverseSolid(current_pos, northwest);
    TraverseSolid(current_pos, southwest);
    TraverseSolid(current_pos, east);
    TraverseSolid(current_pos, northeast);
    TraverseSolid(current_pos, southeast);
    TraverseSolid(current_pos, north);
    TraverseSolid(current_pos, south);
  }
}

void RegionFiller::TraverseSolid(const Vector2f& from, MapCoord to) {
  if (!IsValidPosition(Vector2f(to.x, to.y))) return;

  size_t to_index = (size_t)to.y * 1024 + to.x;

  if (potential_edges[to_index] == region_index) {
    stack.push_back(to);
    potential_edges[to_index] = kUndefinedRegion;

    // Add an edge if this tile is not traversable and it's not already one of the empty region tiles.
    if (coord_regions[to_index] != region_index && !map.CanOccupy(Vector2f(to.x, to.y), 0.8f)) {
      edges[to_index].AddOwner(region_index);
    }
  }
}

// this method is not working at least for Extreme Games
void RegionRegistry::CreateAll(const Map& map, float radius) {
  RegionFiller filler(map, radius, coord_regions_, outside_edges_);

  for (uint16_t y = 0; y < 1024; ++y) {
    for (uint16_t x = 0; x < 1024; ++x) {
      MapCoord coord(x, y);

      if (map.CanOverlapTile(Vector2f(x, y), radius)) {
        // If the current coord is empty and hasn't been inserted into region
        // map then create a new region and flood fill it
        if (!IsRegistered(coord)) {
          auto region_index = CreateRegion();

          filler.Fill(region_index, coord);
        }
      }
    }
  }
}

void RegionRegistry::DebugUpdate(Vector2f position) {
  RegionIndex index = GetRegionIndex(position);

  if (index == -1) return;

  for (uint16_t y = 0; y < 1024; ++y) {
    for (uint16_t x = 0; x < 1024; ++x) {
      if (outside_edges_[y * 1024 + x].HasOwner(index)) {
        Vector2f check = Vector2f(x, y);
        //RenderWorldLine(position, check, check + Vector2f(1, 1), RGB(255, 255, 255));
        //RenderWorldLine(position, check + Vector2f(0, 1), check + Vector2f(1, 0), RGB(255, 255, 255));
      }
    }
  }

  for (uint16_t y = 0; y < 1024; ++y) {
    for (uint16_t x = 0; x < 1024; ++x) {
      if (coord_regions_[y * 1024 + x] == index) {
        Vector2f check = Vector2f(x, y);
        RenderWorldLine(position, check, check + Vector2f(1, 1), RGB(255, 255, 255));
        RenderWorldLine(position, check + Vector2f(0, 1), check + Vector2f(1, 0), RGB(255, 255, 255));
      }
    }
  }
}

bool RegionRegistry::IsRegistered(MapCoord coord) const {
  // return coord_regions_.find(coord) != coord_regions_.end();
  if (!IsValidPosition(Vector2f(coord.x, coord.y))) return false;
  return coord_regions_[coord.y * 1024 + coord.x] != -1;
}

void RegionRegistry::Insert(MapCoord coord, RegionIndex index) {
  // coord_regions_[coord] = index;
  if (!IsValidPosition(Vector2f(coord.x, coord.y))) return;
  coord_regions_[coord.y * 1024 + coord.x] = index;
}

RegionIndex RegionRegistry::CreateRegion() {
  return region_count_++;
}

RegionIndex RegionRegistry::GetRegionIndex(MapCoord coord) {
  // auto itr = coord_regions_.find(coord);
  // return itr->second;
  if (!IsValidPosition(Vector2f(coord.x, coord.y))) return -1;
  return coord_regions_[coord.y * 1024 + coord.x];
}

bool RegionRegistry::IsConnected(MapCoord a, MapCoord b) const {
  // Only one needs to be checked for invalid because the second line will
  // fail if one only one is invalid
  // auto first = coord_regions_.find(a);
  // if (first == coord_regions_.end()) return false;
  if (!IsValidPosition(Vector2f(a.x, a.y))) return false;
  if (!IsValidPosition(Vector2f(b.x, b.y))) return false;

  RegionIndex first = coord_regions_[a.y * 1024 + a.x];
  if (first == -1) return false;

  // auto second = coord_regions_.find(b);
  RegionIndex second = coord_regions_[b.y * 1024 + b.x];

  // return first->second == second->second;
  return first == second;
}

bool RegionRegistry::IsEdge(MapCoord coord) const {
  // return outside_edges_.find(coord) != outside_edges_.end();
  if (!IsValidPosition(Vector2f(coord.x, coord.y))) return true;
  return outside_edges_[coord.y * 1024 + coord.x].count > 0;
}

bool IsValidPosition(MapCoord coord) {
  return coord.x >= 0 && coord.x < 1024 && coord.y >= 0 && coord.y < 1024;
}

}  // namespace marvin
