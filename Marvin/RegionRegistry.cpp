#include "RegionRegistry.h"

#include <vector>
#include "Debug.h"
#include "Map.h"

namespace marvin {

void RegionRegistry::FloodFillEmptyRegion(const Map& map, const MapCoord& coord, RegionIndex region_index, bool store_solids) {

  if (!map.CanOccupy(Vector2f(coord.x, coord.y), 0.8f)) return;

   //coord_regions_[coord] = region_index;
  coord_regions2_[coord.y * 1024 + coord.x] = region_index;

  std::vector<MapCoord> stack;

  stack.push_back(coord);

  while (!stack.empty()) {
    MapCoord current = stack.back();
    stack.pop_back();

    const MapCoord west(current.x - 1, current.y);
    const MapCoord east(current.x + 1, current.y);
    const MapCoord north(current.x, current.y - 1);
    const MapCoord south(current.x, current.y + 1);

    if (map.CanOccupy(Vector2f(west.x, west.y), 0.8f)) {
      if (coord_regions2_[west.y * 1024 + west.x] == kUndefinedRegion) {
        // if (coord_regions_.find(west) == coord_regions_.end()) {
        coord_regions2_[west.y * 1024 + west.x] = region_index;
        // coord_regions_[west] = region_index;
        stack.push_back(west);
      }
    }
    if (store_solids && IsValidPosition(Vector2f(west.x, west.y)) && !map.CanOccupyRadius(Vector2f(west.x, west.y), 0.8f)) {
      if (unordered_solids2_[west.y * 1024 + west.x] == kUndefinedRegion) {
      //if (unordered_solids_.find(west) == unordered_solids_.end()) {
        //unordered_solids_[west] = region_index;
        unordered_solids2_[west.y * 1024 + west.x] = region_index;
      }
    }

    if (map.CanOccupy(Vector2f(east.x, east.y), 0.8f)) {
      if (coord_regions2_[east.y * 1024 + east.x] == kUndefinedRegion) {
      //if (coord_regions_.find(east) == coord_regions_.end()) {
        //coord_regions_[east] = region_index;
        coord_regions2_[east.y * 1024 + east.x] = region_index;
        stack.push_back(east);
      }
    }
    if (store_solids && IsValidPosition(Vector2f(east.x, east.y)) &&
        !map.CanOccupyRadius(Vector2f(east.x, east.y), 0.8f)) {
      if (unordered_solids2_[east.y * 1024 + east.x] == kUndefinedRegion) {
      //if (unordered_solids_.find(east) == unordered_solids_.end()) {
        //unordered_solids_[east] = region_index;
        unordered_solids2_[east.y * 1024 + east.x] = region_index;
      }
    }

    if (map.CanOccupy(Vector2f(north.x, north.y), 0.8f)) {
      if (coord_regions2_[north.y * 1024 + north.x] == kUndefinedRegion) {
      //if (coord_regions_.find(north) == coord_regions_.end()) {
        //coord_regions_[north] = region_index;
        coord_regions2_[north.y * 1024 + north.x] = region_index;
        stack.push_back(north);
      }
    }
    if (store_solids && IsValidPosition(Vector2f(north.x, north.y)) &&
        !map.CanOccupyRadius(Vector2f(north.x, north.y), 0.8f)) {
      if (unordered_solids2_[north.y * 1024 + north.x] == kUndefinedRegion) {
      //if (unordered_solids_.find(north) == unordered_solids_.end()) {
        //unordered_solids_[north] = region_index;
        unordered_solids2_[north.y * 1024 + north.x] = region_index;
      }
    }

    if (map.CanOccupy(Vector2f(south.x, south.y), 0.8f)) {
      if (coord_regions2_[south.y * 1024 + south.x] == kUndefinedRegion) {
      //if (coord_regions_.find(south) == coord_regions_.end()) {
       // coord_regions_[south] = region_index;
        coord_regions2_[south.y * 1024 + south.x] = region_index;
        stack.push_back(south);
      }
    }
    
    if (store_solids && IsValidPosition(Vector2f(south.x, south.y)) &&
        !map.CanOccupyRadius(Vector2f(south.x, south.y), 0.8f)) {
      if (unordered_solids2_[south.y * 1024 + south.x] == kUndefinedRegion) {
      //if (unordered_solids_.find(south) == unordered_solids_.end()) {
        //unordered_solids_[south] = region_index;
        unordered_solids2_[south.y * 1024 + south.x] = region_index;
      }
    }
  }
}

// this will only work on tiles stored by the floodfillemptyregion function, to prevent it from spreading into
// regions where the walls are shared or touching each other
void RegionRegistry::FloodFillSolidRegion(const Map& map, const MapCoord& coord, RegionIndex region_index) {
 
    if (!IsValidPosition(Vector2f(coord.x, coord.y))) return;

  //outside_edges_[coord] = region_index;
  outside_edges2_[coord.y * 1024 + coord.x] = region_index;
  std::vector<MapCoord> stack;

  stack.push_back(coord);

  while (!stack.empty()) {
    MapCoord current = stack.back();
    stack.pop_back();

    const MapCoord west(current.x - 1, current.y);
    const MapCoord northwest(current.x - 1, current.y - 1);
    const MapCoord southwest(current.x - 1, current.y + 1);
    const MapCoord east(current.x + 1, current.y);
    const MapCoord northeast(current.x + 1, current.y - 1);
    const MapCoord southeast(current.x + 1, current.y + 1);
    const MapCoord north(current.x, current.y - 1);
    const MapCoord south(current.x, current.y + 1);

    if (IsValidPosition(Vector2f(west.x, west.y)) && !map.CanOccupyRadius(Vector2f(west.x, west.y), 0.8f)) {
      if (outside_edges2_[west.y * 1024 + west.x] == kUndefinedRegion &&
          unordered_solids2_[west.y * 1024 + west.x] == region_index) {
      //if (outside_edges_.find(west) == outside_edges_.end() && unordered_solids_.find(west) != unordered_solids_.end()) {
        //outside_edges_[west] = region_index;
          outside_edges2_[west.y * 1024 + west.x] = region_index;
        stack.push_back(west);
      }
    }

    if (IsValidPosition(Vector2f(east.x, east.y)) && !map.CanOccupyRadius(Vector2f(east.x, east.y), 0.8f)) {
      if (outside_edges2_[east.y * 1024 + east.x] == kUndefinedRegion &&
          unordered_solids2_[east.y * 1024 + east.x] == region_index) {
        // if (outside_edges_.find(east) == outside_edges_.end() &&
        // unordered_solids_.find(east) != unordered_solids_.end()) {
        // outside_edges_[east] = region_index;
        outside_edges2_[east.y * 1024 + east.x] = region_index;
        stack.push_back(east);
      }
    }

    if (IsValidPosition(Vector2f(south.x, south.y)) && !map.CanOccupyRadius(Vector2f(south.x, south.y), 0.8f)) {
      if (outside_edges2_[south.y * 1024 + south.x] == kUndefinedRegion &&
          unordered_solids2_[south.y * 1024 + south.x] == region_index) {
        // if (outside_edges_.find(south) == outside_edges_.end() &&
        // unordered_solids_.find(south) != unordered_solids_.end()) {
        // outside_edges_[south] = region_index;
        outside_edges2_[south.y * 1024 + south.x] = region_index;
        stack.push_back(south);
      }
    }

     if (IsValidPosition(Vector2f(southwest.x, southwest.y)) &&
        !map.CanOccupyRadius(Vector2f(southwest.x, southwest.y), 0.8f)) {
      if (outside_edges2_[southwest.y * 1024 + southwest.x] == kUndefinedRegion &&
          unordered_solids2_[southwest.y * 1024 + southwest.x] == region_index) {
        // if (outside_edges_.find(southwest) == outside_edges_.end() &&
        // unordered_solids_.find(southwest) != unordered_solids_.end()) {
        // outside_edges_[southwest] = region_index;
        outside_edges2_[southwest.y * 1024 + southwest.x] = region_index;
        stack.push_back(southwest);
      }
    }

      if (IsValidPosition(Vector2f(southeast.x, southeast.y)) &&
        !map.CanOccupyRadius(Vector2f(southeast.x, southeast.y), 0.8f)) {
      if (outside_edges2_[southeast.y * 1024 + southeast.x] == kUndefinedRegion &&
          unordered_solids2_[southeast.y * 1024 + southeast.x] == region_index) {
        // if (outside_edges_.find(southeast) == outside_edges_.end() &&
        //  unordered_solids_.find(southeast) != unordered_solids_.end()) {
        // outside_edges_[southeast] = region_index;
        outside_edges2_[southeast.y * 1024 + southeast.x] = region_index;
        stack.push_back(southeast);
      }
    }


    //break;
    if (IsValidPosition(Vector2f(north.x, north.y)) && !map.CanOccupyRadius(Vector2f(north.x, north.y), 0.8f)) {
      if (outside_edges2_[north.y * 1024 + north.x] == kUndefinedRegion &&
          unordered_solids2_[north.y * 1024 + north.x] == region_index) {
        // if (outside_edges_.find(north) == outside_edges_.end() &&
        // unordered_solids_.find(north) != unordered_solids_.end()) {
        // outside_edges_[north] = region_index;
        outside_edges2_[north.y * 1024 + north.x] = region_index;
        stack.push_back(north);
      }
    }


    if (IsValidPosition(Vector2f(northwest.x, northwest.y)) &&
        !map.CanOccupyRadius(Vector2f(northwest.x, northwest.y), 0.8f)) {
      if (outside_edges2_[northwest.y * 1024 + northwest.x] == kUndefinedRegion &&
          unordered_solids2_[northwest.y * 1024 + northwest.x] == region_index) {
      //if (outside_edges_.find(northwest) == outside_edges_.end() &&
         // unordered_solids_.find(northwest) != unordered_solids_.end()) {
       // outside_edges_[northwest] = region_index;
      //outside_edges2_[northwest.y * 1024 + northwest.x] = region_index;
        stack.push_back(northwest);
      }
    }
    
    if (IsValidPosition(Vector2f(northeast.x, northeast.y)) &&
        !map.CanOccupyRadius(Vector2f(northeast.x, northeast.y), 0.8f)) {
      if (outside_edges2_[northeast.y * 1024 + northeast.x] == kUndefinedRegion &&
          unordered_solids2_[northeast.y * 1024 + northeast.x] == region_index) {
      //if (outside_edges_.find(northeast) == outside_edges_.end() &&
         // unordered_solids_.find(northeast) != unordered_solids_.end()) {
        //outside_edges_[northeast] = region_index;
      outside_edges2_[northeast.y * 1024 + northeast.x] = region_index;
        stack.push_back(northeast);
      }
    }
  }
}

void RegionRegistry::CreateRegions(const Map& map, std::vector<Vector2f> seed_points) {

  //unordered_solids_.clear();

  // use a seed point to find walls on only the desired regions
 // store tiles the ship cant fit on (needs work) to help determine the regions boundries

  for (Vector2f seed : seed_points) {
    RegionIndex index = CreateRegion();

    FloodFillEmptyRegion(map, seed, index, true);

    MapCoord top_tile = MapCoord(9999, 9999);

    // find a tile in the set with the lowest Y coordinate, this must be part of the outside edge
    for (uint16_t y = 0; y < 1024; ++y) {
      for (uint16_t x = 0; x < 1024; ++x) {
        if (unordered_solids2_[y * 1024 + x] == index) {
          // for (auto itr = unordered_solids_.begin(); itr != unordered_solids_.end(); itr++) {
          // itr works as a pointer to pair<string, double>
          // type itr->first stores the key part  and
          // itr->second stores the value part
          // MapCoord current = itr->first;
          if (y < top_tile.y) {
            top_tile = Vector2f(x, y);
          }

          // if (itr->second == index && top_tile.y > current.y) {
          // top_tile = current;
          //}
        }
      }
    }
    // use the found tile to flood fill this regions outside edge
    FloodFillSolidRegion(map, top_tile, index);
  }

}

void RegionRegistry::DebugUpdate(Vector2f position) {

    RegionIndex index = GetRegionIndex(position);

    for (uint16_t y = 0; y < 1024; ++y) {
      for (uint16_t x = 0; x < 1024; ++x) {
        if (index != -1 && unordered_solids2_[y * 1024 + x] == index) {
          Vector2f check = Vector2f(x, y);
          //RenderWorldLine(position, check, check + Vector2f(1, 1), RGB(255, 255, 255));
          //RenderWorldLine(position, check + Vector2f(0, 1), check + Vector2f(1, 0), RGB(255, 255, 255));
        }
      }
    }

  for (uint16_t y = 0; y < 1024; ++y) {
    for (uint16_t x = 0; x < 1024; ++x) {
      if (index != -1 && outside_edges2_[y * 1024 + x] == index) {
        Vector2f check = Vector2f(x, y);
         RenderWorldLine(position, check, check + Vector2f(1, 1), RGB(255, 255, 255));
         RenderWorldLine(position, check + Vector2f(0, 1), check + Vector2f(1, 0), RGB(255, 255, 255));
      }
    }
  }
  for (uint16_t y = 0; y < 1024; ++y) {
    for (uint16_t x = 0; x < 1024; ++x) {
      if (index != -1 && coord_regions2_ [y * 1024 + x] == index) {
        Vector2f check = Vector2f(x,y);
        //RenderWorldLine(position, check, check + Vector2f(1, 1), RGB(255, 255, 255));
        //RenderWorldLine(position, check + Vector2f(0, 1), check + Vector2f(1, 0), RGB(255, 255, 255));
      }
    }
  }
}

void RegionRegistry::CreateAll(const Map& map) {
  for (uint16_t y = 0; y < 1024; ++y) {
    for (uint16_t x = 0; x < 1024; ++x) {
      MapCoord coord(x, y);

      if (map.CanOccupy(Vector2f(x, y), 0.8f)) {
        // if (!map.IsSolid(x, y)) {
        // If the current coord is empty and hasn't been inserted into region
        // map then create a new region and flood fill it
        if (!IsRegistered(coord)) {
          auto region_index = CreateRegion();

          FloodFillEmptyRegion(map, coord, region_index, false);
        }
      }
    }
  }
}



bool RegionRegistry::IsRegistered(MapCoord coord) const {
 // return coord_regions_.find(coord) != coord_regions_.end();
  if (!IsValidPosition(Vector2f(coord.x, coord.y))) return false;
  return coord_regions2_[coord.y * 1024 + coord.x] != -1;
}

void RegionRegistry::Insert(MapCoord coord, RegionIndex index) {
  //coord_regions_[coord] = index;
  if (!IsValidPosition(Vector2f(coord.x, coord.y))) return;
  coord_regions2_[coord.y * 1024 + coord.x] = index;
}

RegionIndex RegionRegistry::CreateRegion() {
  return region_count_++;
}

RegionIndex RegionRegistry::GetRegionIndex(MapCoord coord) {
  //auto itr = coord_regions_.find(coord);
  //return itr->second;
  if (!IsValidPosition(Vector2f(coord.x, coord.y))) return -1;
  return coord_regions2_[coord.y * 1024 + coord.x];
}

bool RegionRegistry::IsConnected(MapCoord a, MapCoord b) const {
  // Only one needs to be checked for invalid because the second line will
  // fail if one only one is invalid
  //auto first = coord_regions_.find(a);
  //if (first == coord_regions_.end()) return false;
  if (!IsValidPosition(Vector2f(a.x, a.y))) return false;
  if (!IsValidPosition(Vector2f(b.x, b.y))) return false;

RegionIndex first = coord_regions2_[a.y * 1024 + a.x];
  if (first == -1) return false;

  //auto second = coord_regions_.find(b);
  RegionIndex second = coord_regions2_[b.y * 1024 + b.x];

  //return first->second == second->second;
  return first == second;
}

bool RegionRegistry::IsEdge(MapCoord coord) const {
  //return outside_edges_.find(coord) != outside_edges_.end();
  if (!IsValidPosition(Vector2f(coord.x, coord.y))) return true;
  return outside_edges2_[coord.y * 1024 + coord.x] != -1;
}

}  // namespace marvin
