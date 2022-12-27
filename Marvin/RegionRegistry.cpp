#include "RegionRegistry.h"

#include <vector>
#include "Debug.h"
#include "Map.h"

namespace marvin {

/*
This method sweeps the fill area 4 times.  For each sweep, it takes the ships radius, and pins the orientation of the ship
to each of its 4 corners, then flood fills the map.  Pinning the orientation for each sweep is important because it wont allow
the fill to jump past diagonal gaps in walls.  The downside to pinning the orientation is that it leaves a space along 
wall.  The fix to fill that space is to run a pass for each corner of the ship with the orientation pinned for each pass.
*/
void RegionRegistry::FloodFillRegion(const Map& map, const MapCoord& coord, RegionIndex region_index,
                                          float radius) {
  FloodFillEmptyRegion(map, coord, 9000, false, false, radius);
  FloodFillEmptyRegion(map, coord, 9001, false, true, radius);
  FloodFillEmptyRegion(map, coord, 9002, true, false, radius);
  FloodFillEmptyRegion(map, coord, 9003, true, true, radius);

  for (std::size_t i = 0; i < 1024 * 1024; i++) {
    for (size_t j = 9000; j <= 9003; j++) {
      if (coord_regions_[i] == j) {
        coord_regions_[i] = region_index;
      }

       if (unordered_solids_[i] == j) {
        unordered_solids_[i] = region_index;
       }

       if (outside_edges_[i] == j) {
         outside_edges_[i] = region_index;
       }
    }
  }

  MapCoord top_tile = MapCoord(9999, 9999);

  // find a tile in the set with the lowest Y coordinate, this must be part of the outside edge
  for (uint16_t y = 0; y < 1024; ++y) {
    for (uint16_t x = 0; x < 1024; ++x) {
      if (unordered_solids_[y * 1024 + x] == region_index) {
        if (y < top_tile.y) {
          top_tile = Vector2f(x, y);
        }
      }
    }
  }
  // use the found tile to flood fill this regions outside edge
  FloodFillSolidRegion(map, top_tile, region_index);
}

void RegionRegistry::FloodFillEmptyRegion(const Map& map, const MapCoord& coord, RegionIndex region_index,
                                          bool right_corner_check, bool bottom_corner_check, float radius) {

  if (!map.CornerPointCheck(Vector2f(coord.x, coord.y), right_corner_check, bottom_corner_check, radius)) return;

  coord_regions_[coord.y * 1024 + coord.x] = region_index;

  std::vector<MapCoord> stack;

  stack.push_back(coord);

  while (!stack.empty()) {
    MapCoord current = stack.back();
    stack.pop_back();

    const MapCoord west(current.x - 1, current.y);
    const MapCoord east(current.x + 1, current.y);
    const MapCoord north(current.x, current.y - 1);
    const MapCoord south(current.x, current.y + 1);

    if (IsValidPosition(west)) {
      if (map.CornerPointCheck(Vector2f(west.x, west.y), right_corner_check, bottom_corner_check, radius)) {
        if (coord_regions_[west.y * 1024 + west.x] != region_index) {
          coord_regions_[west.y * 1024 + west.x] = region_index;
          stack.push_back(west);
        }
      } else if (unordered_solids_[west.y * 1024 + west.x] == kUndefinedRegion) {
        unordered_solids_[west.y * 1024 + west.x] = region_index;
      }
    }

    if (IsValidPosition(east)) {
      if (map.CornerPointCheck(Vector2f(east.x, east.y), right_corner_check, bottom_corner_check, radius)) {
        if (coord_regions_[east.y * 1024 + east.x] != region_index) {
          coord_regions_[east.y * 1024 + east.x] = region_index;
          stack.push_back(east);
        }
      } else if (unordered_solids_[east.y * 1024 + east.x] == kUndefinedRegion) {
        unordered_solids_[east.y * 1024 + east.x] = region_index;
      }
    }

    if (IsValidPosition(north)) {
      if (map.CornerPointCheck(Vector2f(north.x, north.y), right_corner_check, bottom_corner_check, radius)) {
        if (coord_regions_[north.y * 1024 + north.x] != region_index) {
          coord_regions_[north.y * 1024 + north.x] = region_index;
          stack.push_back(north);
        }
      } else if (unordered_solids_[north.y * 1024 + north.x] == kUndefinedRegion) {
        unordered_solids_[north.y * 1024 + north.x] = region_index;
      }
    }

    if (IsValidPosition(south)) {
      if (map.CornerPointCheck(Vector2f(south.x, south.y), right_corner_check, bottom_corner_check, radius)) {
        if (coord_regions_[south.y * 1024 + south.x] != region_index) {
          coord_regions_[south.y * 1024 + south.x] = region_index;
          stack.push_back(south);
        }
      } else if (unordered_solids_[south.y * 1024 + south.x] == kUndefinedRegion) {
        unordered_solids_[south.y * 1024 + south.x] = region_index;
      }
    }
  }
}

// this will only work on tiles stored by the floodfillemptyregion function, to prevent it from spreading into
// regions where the walls are shared or touching each other
void RegionRegistry::FloodFillSolidRegion(const Map& map, const MapCoord& coord, RegionIndex region_index) {
 
    if (!IsValidPosition(Vector2f(coord.x, coord.y))) return;

  //outside_edges_[coord] = region_index;
  outside_edges_[coord.y * 1024 + coord.x] = region_index;
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

    if (IsValidPosition(Vector2f(west.x, west.y))) {
      if (unordered_solids_[west.y * 1024 + west.x] == region_index) {

        stack.push_back(west);
        unordered_solids_[west.y * 1024 + west.x] = 9999;

        if (outside_edges_[west.y * 1024 + west.x] == kUndefinedRegion && !map.CanOccupy(Vector2f(west.x, west.y), 0.8f)) {
          outside_edges_[west.y * 1024 + west.x] = region_index;
        }
      }
    }

    if (IsValidPosition(Vector2f(northwest.x, northwest.y))) {
      if (unordered_solids_[northwest.y * 1024 + northwest.x] == region_index) {

        stack.push_back(northwest);
        unordered_solids_[northwest.y * 1024 + northwest.x] = 9999;

        if (outside_edges_[northwest.y * 1024 + northwest.x] == kUndefinedRegion &&
            !map.CanOccupy(Vector2f(northwest.x, northwest.y), 0.8f)) {
          outside_edges_[northwest.y * 1024 + northwest.x] = region_index;
        }
      }
    }

    if (IsValidPosition(Vector2f(southwest.x, southwest.y))) {
      if (unordered_solids_[southwest.y * 1024 + southwest.x] == region_index) {

        stack.push_back(southwest);
        unordered_solids_[southwest.y * 1024 + southwest.x] = 9999;

        if (outside_edges_[southwest.y * 1024 + southwest.x] == kUndefinedRegion &&
            !map.CanOccupy(Vector2f(southwest.x, southwest.y), 0.8f)) {
          outside_edges_[southwest.y * 1024 + southwest.x] = region_index;
        }
      }
    }

    if (IsValidPosition(Vector2f(east.x, east.y))) {
      if (unordered_solids_[east.y * 1024 + east.x] == region_index) {

        stack.push_back(east);
        unordered_solids_[east.y * 1024 + east.x] = 9999;

        if (outside_edges_[east.y * 1024 + east.x] == kUndefinedRegion &&
            !map.CanOccupy(Vector2f(east.x, east.y), 0.8f)) {
          outside_edges_[east.y * 1024 + east.x] = region_index;
        }
      }
    }

    if (IsValidPosition(Vector2f(northeast.x, northeast.y))) {
      if (unordered_solids_[northeast.y * 1024 + northeast.x] == region_index) {

        stack.push_back(northeast);
        unordered_solids_[northeast.y * 1024 + northeast.x] = 9999;

        if (outside_edges_[northeast.y * 1024 + northeast.x] == kUndefinedRegion &&
            !map.CanOccupy(Vector2f(northeast.x, northeast.y), 0.8f)) {
          outside_edges_[northeast.y * 1024 + northeast.x] = region_index;
        }
      }
    }

    if (IsValidPosition(Vector2f(southeast.x, southeast.y))) {
      if (unordered_solids_[southeast.y * 1024 + southeast.x] == region_index) {

        stack.push_back(southeast);
        unordered_solids_[southeast.y * 1024 + southeast.x] = 9999;

        if (outside_edges_[southeast.y * 1024 + southeast.x] == kUndefinedRegion &&
            !map.CanOccupy(Vector2f(southeast.x, southeast.y), 0.8f)) {
          outside_edges_[southeast.y * 1024 + southeast.x] = region_index;
        }
      }
    }

    if (IsValidPosition(Vector2f(north.x, north.y))) {
      if (unordered_solids_[north.y * 1024 + north.x] == region_index) {

        stack.push_back(north);
        unordered_solids_[north.y * 1024 + north.x] = 9999;

        if (outside_edges_[north.y * 1024 + north.x] == kUndefinedRegion &&
            !map.CanOccupy(Vector2f(north.x, north.y), 0.8f)) {
          outside_edges_[north.y * 1024 + north.x] = region_index;
        }
      }
    }

    if (IsValidPosition(Vector2f(south.x, south.y))) {
      if (unordered_solids_[south.y * 1024 + south.x] == region_index) {

        stack.push_back(south);
        unordered_solids_[south.y * 1024 + south.x] = 9999;

        if (outside_edges_[south.y * 1024 + south.x] == kUndefinedRegion &&
            !map.CanOccupy(Vector2f(south.x, south.y), 0.8f)) {
          outside_edges_[south.y * 1024 + south.x] = region_index;
        }
      }
    }


    #if 0
    if (IsValidPosition(Vector2f(east.x, east.y)) && !map.CanOccupyRadius(Vector2f(east.x, east.y), 0.8f)) {
      if (outside_edges_[east.y * 1024 + east.x] == kUndefinedRegion &&
        unordered_solids_[east.y * 1024 + east.x] == region_index) {
        outside_edges_[east.y * 1024 + east.x] = region_index;
        stack.push_back(east);
      }
    }

    if (IsValidPosition(Vector2f(south.x, south.y)) && !map.CanOccupyRadius(Vector2f(south.x, south.y), 0.8f)) {
      if (outside_edges_[south.y * 1024 + south.x] == kUndefinedRegion &&
        unordered_solids_[south.y * 1024 + south.x] == region_index) {
        outside_edges_[south.y * 1024 + south.x] = region_index;
        stack.push_back(south);
      }
    }

    if (IsValidPosition(Vector2f(southwest.x, southwest.y)) && !map.CanOccupyRadius(Vector2f(southwest.x, southwest.y), 0.8f)) {
      if (outside_edges_[southwest.y * 1024 + southwest.x] == kUndefinedRegion &&
        unordered_solids_[southwest.y * 1024 + southwest.x] == region_index) {
        outside_edges_[southwest.y * 1024 + southwest.x] = region_index;
        stack.push_back(southwest);
      }
    }

    if (IsValidPosition(Vector2f(southeast.x, southeast.y)) && !map.CanOccupyRadius(Vector2f(southeast.x, southeast.y), 0.8f)) {
      if (outside_edges_[southeast.y * 1024 + southeast.x] == kUndefinedRegion &&
        unordered_solids_[southeast.y * 1024 + southeast.x] == region_index) {
        outside_edges_[southeast.y * 1024 + southeast.x] = region_index;
        stack.push_back(southeast);
      }
    }

    if (IsValidPosition(Vector2f(north.x, north.y)) && !map.CanOccupyRadius(Vector2f(north.x, north.y), 0.8f)) {
      if (outside_edges_[north.y * 1024 + north.x] == kUndefinedRegion &&
        unordered_solids_[north.y * 1024 + north.x] == region_index) {
        outside_edges_[north.y * 1024 + north.x] = region_index;
        stack.push_back(north);
      }
    }


    if (IsValidPosition(Vector2f(northwest.x, northwest.y)) && !map.CanOccupyRadius(Vector2f(northwest.x, northwest.y), 0.8f)) {
      if (outside_edges_[northwest.y * 1024 + northwest.x] == kUndefinedRegion &&
        unordered_solids_[northwest.y * 1024 + northwest.x] == region_index) {
        outside_edges_[northwest.y * 1024 + northwest.x] = region_index;
        stack.push_back(northwest);
      }
    }
    
    if (IsValidPosition(Vector2f(northeast.x, northeast.y)) && !map.CanOccupyRadius(Vector2f(northeast.x, northeast.y), 0.8f)) {
      if (outside_edges_[northeast.y * 1024 + northeast.x] == kUndefinedRegion &&
        unordered_solids_[northeast.y * 1024 + northeast.x] == region_index) {
        outside_edges_[northeast.y * 1024 + northeast.x] = region_index;
        stack.push_back(northeast);
      }
    }
    #endif
  }
}

void RegionRegistry::CreateRegions(const Map& map, std::vector<Vector2f> seed_points, float radius) {

  //unordered_solids_.clear();

  // use a seed point to find walls on only the desired regions
 // store tiles the ship cant fit on (needs work) to help determine the regions boundries

  for (Vector2f seed : seed_points) {
    RegionIndex index = CreateRegion();

    FloodFillRegion(map, seed, index, radius);

    MapCoord top_tile = MapCoord(9999, 9999);

    // find a tile in the set with the lowest Y coordinate, this must be part of the outside edge
    for (uint16_t y = 0; y < 1024; ++y) {
      for (uint16_t x = 0; x < 1024; ++x) {
        if (unordered_solids_[y * 1024 + x] == index) {
          if (y < top_tile.y) {
            top_tile = Vector2f(x, y);
          }
        }
      }
    }
    // use the found tile to flood fill this regions outside edge
    FloodFillSolidRegion(map, top_tile, index);
  }
}

// this method is not working at least for Extreme Games
void RegionRegistry::CreateAll(const Map& map, float radius) {
  for (uint16_t y = 0; y < 1024; ++y) {
    for (uint16_t x = 0; x < 1024; ++x) {
      MapCoord coord(x, y);

      if (map.CanOccupyRadius(Vector2f(x, y), radius)) {
        // if (!map.IsSolid(x, y)) {
        // If the current coord is empty and hasn't been inserted into region
        // map then create a new region and flood fill it
        if (!IsRegistered(coord)) 
        {
          auto region_index = CreateRegion();
          FloodFillRegion(map, coord, region_index, radius);

          
        }
      }
    }
  }
}

void RegionRegistry::DebugUpdate(Vector2f position) {

    RegionIndex index = GetRegionIndex(position);

    for (uint16_t y = 0; y < 1024; ++y) {
      for (uint16_t x = 0; x < 1024; ++x) {
        if (index != -1 && unordered_solids_[y * 1024 + x] == index) {
          Vector2f check = Vector2f(x, y);
          //RenderWorldLine(position, check, check + Vector2f(1, 1), RGB(255, 255, 255));
          //RenderWorldLine(position, check + Vector2f(0, 1), check + Vector2f(1, 0), RGB(255, 255, 255));
        }
      }
    }

  for (uint16_t y = 0; y < 1024; ++y) {
    for (uint16_t x = 0; x < 1024; ++x) {
      if (index != -1 && outside_edges_[y * 1024 + x] == index) {
        Vector2f check = Vector2f(x, y);
         RenderWorldLine(position, check, check + Vector2f(1, 1), RGB(255, 255, 255));
         RenderWorldLine(position, check + Vector2f(0, 1), check + Vector2f(1, 0), RGB(255, 255, 255));
      }
    }
  }
  for (uint16_t y = 0; y < 1024; ++y) {
    for (uint16_t x = 0; x < 1024; ++x) {
      if (index != -1 && coord_regions_ [y * 1024 + x] == index) {
        Vector2f check = Vector2f(x,y);
        //RenderWorldLine(position, check, check + Vector2f(1, 1), RGB(255, 255, 255));
        //RenderWorldLine(position, check + Vector2f(0, 1), check + Vector2f(1, 0), RGB(255, 255, 255));
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
  //coord_regions_[coord] = index;
  if (!IsValidPosition(Vector2f(coord.x, coord.y))) return;
  coord_regions_[coord.y * 1024 + coord.x] = index;
}

RegionIndex RegionRegistry::CreateRegion() {
  return region_count_++;
}

RegionIndex RegionRegistry::GetRegionIndex(MapCoord coord) {
  //auto itr = coord_regions_.find(coord);
  //return itr->second;
  if (!IsValidPosition(Vector2f(coord.x, coord.y))) return -1;
  return coord_regions_[coord.y * 1024 + coord.x];
}

bool RegionRegistry::IsConnected(MapCoord a, MapCoord b) const {
  // Only one needs to be checked for invalid because the second line will
  // fail if one only one is invalid
  //auto first = coord_regions_.find(a);
  //if (first == coord_regions_.end()) return false;
  if (!IsValidPosition(Vector2f(a.x, a.y))) return false;
  if (!IsValidPosition(Vector2f(b.x, b.y))) return false;

RegionIndex first = coord_regions_[a.y * 1024 + a.x];
  if (first == -1) return false;

  //auto second = coord_regions_.find(b);
  RegionIndex second = coord_regions_[b.y * 1024 + b.x];

  //return first->second == second->second;
  return first == second;
}

bool RegionRegistry::IsEdge(MapCoord coord) const {
  //return outside_edges_.find(coord) != outside_edges_.end();
  if (!IsValidPosition(Vector2f(coord.x, coord.y))) return true;
  return outside_edges_[coord.y * 1024 + coord.x] != -1;
}

bool IsValidPosition(MapCoord coord) {
  return coord.x >= 0 && coord.x < 1024 && coord.y >= 0 && coord.y < 1024;
}

}  // namespace marvin
