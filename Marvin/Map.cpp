#include "Map.h"

#include <fstream>


#include "RegionRegistry.h"
#include "MapCoord.h"
#include "Vector2i.h"
//#include "path/NodeProcessor.h"

namespace marvin {

  //  enum class Corner : short {TopLeft, TopRight, BottomLeft, BottomRight};
//enum class Direction : short { North, South, East, West, NorthWest, NorthEast, SouthWest, SouthEast, None };

Direction GetDirecton(Vector2i direction) {
  if (direction.x == -1) {
    if (direction.y == -1) {
      return Direction::NorthWest;
    } else if (direction.y == 0) {
      return Direction::West;
    } else if (direction.y == 1) {
      return Direction::SouthWest;
    }
  } else if (direction.x == 0) {
     if (direction.y == -1) {
      return Direction::North; 
    } else if (direction.y == 1) {
      return Direction::South;
     }
  } else if (direction.x == 1) {
     if (direction.y == -1) {
      return Direction::NorthEast;
     } else if (direction.y == 0) {
      return Direction::East;
     } else if (direction.y == 1) {
      return Direction::SouthEast;
     }
  }
  return Direction::None;
}

Map::Map(const TileData& tile_data) : tile_data_(tile_data) {
  memset(mine_map, 0, sizeof(mine_map));
}

TileId Map::GetTileId(u16 x, u16 y) const {
  if (x >= 1024 || y >= 1024) return 0;

  u8* map_memory = (u8*)*(u32*)(*(u32*)(0x4C1AFC) + 0x127ec + 0x1d6d0);
  if (map_memory) return map_memory[y * kMapExtent + x];
  
  return 170;
  //return tile_data_[y * kMapExtent + x];
}

bool Map::IsSolid(u16 x, u16 y) const {
  if (x >= 1024 || y >= 1024) return true;
  return IsSolid(GetTileId(x, y));
}

void Map::SetTileId(u16 x, u16 y, TileId id) {
  if (x >= 1024 || y >= 1024) return;
  tile_data_[y * kMapExtent + x] = id;
}

void Map::SetTileId(const Vector2f& position, TileId id) {
  u16 x = static_cast<u16>(position.x);
  u16 y = static_cast<u16>(position.y);

  SetTileId(x, y, id);
}

bool Map::IsSolid(TileId id) const {
  if (id == 0) return false;
  //if (id >= 162 && id <= 169) return false;  // treat doors as non-solid
  if (id >= 162 && id <= 169) return true;  // doors
  if (id < 170) return true;
  if (id >= 192 && id <= 240) return true;
  if (id >= 242 && id <= 252) return true;

  return false;
}

bool Map::IsSolid(const Vector2f& position) const {
  u16 x = static_cast<u16>(position.x);
  u16 y = static_cast<u16>(position.y);

  return IsSolid(x, y);
}

TileId Map::GetTileId(const Vector2f& position) const {
  u16 x = static_cast<u16>(position.x);
  u16 y = static_cast<u16>(position.y);

  return GetTileId(x, y);
}

bool Map::IsMined(MapCoord coord) const {
  return mine_map[coord.y * kMapExtent + coord.x] == 1;
}

void Map::SetMinedTiles(std::vector<Weapon*> mines) {
  ClearMinedTiles();
  for (Weapon* mine : mines) {
    SetMinedTile(mine->GetPosition());
  }
}

void Map::SetMinedTile(MapCoord coord) {
  mine_map[coord.y * kMapExtent + coord.x] = 1;
  mined_tile_index_list.emplace_back(coord.y * kMapExtent + coord.x);
}

void Map::ClearMinedTiles() {
  for (std::size_t i = 0; i < mined_tile_index_list.size(); i++) {
    mine_map[mined_tile_index_list[i]] = 0;
  }
  mined_tile_index_list.clear();
}

// if any tile in the square is solid return true
bool Map::IsSolidSquare(MapCoord top_left, uint16_t length) const {
  uint16_t abs_x = top_left.x;
  uint16_t abs_y = top_left.y;

  for (uint16_t x = 0; x < length; x++) {
    for (uint16_t y = 0; y < length; y++) {
      MapCoord pos(abs_x + x, abs_y + y);
      if (IsSolid(pos)) {
        return true;
      }
    }
  }
  return false;
}

// get a list of positions/orientation points that the ship can fit on the selected tile
OccupyMap Map::CalculateOccupyMap(MapCoord start, float radius) const {
  OccupyMap result;

  uint16_t diameter = uint16_t(radius + 0.5f) * 2;
  MapCoord offset(start.x - diameter + 1, start.y - diameter + 1);

  for (uint16_t x = 0; x < diameter; x++) {
    for (uint16_t y = 0; y < diameter; y++) {
      MapCoord pos(offset.x + x, offset.y + y);

      result.set[result.count++] = IsSolidSquare(pos, diameter);
    }
  }

  return result;
}

#if 0
// for 1 tile steps
bool Map::CanMoveTo(MapCoord from, MapCoord to, float radius) const {
  
 path::CoordOffset coord_direction = path::CoordOffset(from.x - to.x, from.y - to.y);
 Vector2i map_direction = Vector2i(from.x - to.x, from.y - to.y);
  Direction direction = GetDirecton(map_direction);
 
  OccupyMap fromMap = CalculateOccupyMap(from, radius);
  OccupyMap toMap = CalculateOccupyMap(to, radius);


  // compare the lists, if the ship was able to complete a 1 tile step in the same direction return true
  for (std::size_t i = 0; i < fromMap.count; i++) {
    if (fromMap[i] || toMap[i]) {
      // if (fromMap[i] == MapCoord(0, 0) || toMap[i] == MapCoord(0, 0)) {
      continue;
    } else {
      return true;
    }

    //  if (fromMap[i] - toMap[i] == cardinal) {
    //    return true;
    // }
  }
  return false;
}
#endif

//#if 0
bool Map::CanPathOn(const Vector2f& position, float radius) const {
  /* Peg the check position to the same tile on the ship and expand the search for every tile it
  *  will occupy at that point.
  *
  *  if the ship is a 2 x 2 square, the upper left square is the position and the radius expands down
     and to the right. This will always push the path to the upper left in tight spaces.

     note: This method allows diagonal steps through diagonal gaps, but those nodes need to still
     be considered pathathble for the bot to correctly path through tubes with non solid corners
  */

  int tile_diameter = int((radius + 0.5f) * 2);
  // Direction direction = GetDirection(Normalize(to - from));
  Vector2f offset;

  switch (tile_diameter) {
    case 3:
    case 4: {
      offset = Vector2f(1, 1);
    } break;
    case 5: {
      offset = Vector2f(2, 2);
    } break;
  }

  for (float y = -offset.y; y < tile_diameter - offset.y; ++y) {
    for (float x = -offset.x; x < tile_diameter - offset.x; ++x) {
      uint16_t world_x = (uint16_t)(position.x + x);
      uint16_t world_y = (uint16_t)(position.y + y);
      if (IsSolid(world_x, world_y)) {
        return false;
      }
    }
  }

// if direction is diagonal use recursive method to skip it if both the sides steps are false
#if 0
  switch (direction) {
    case Direction::NorthWest: {
      if (!CanPathOn(from, East(to), radius) && !CanPathOn(from, South(to), radius)) {
        return false;
      }
    } break;
    case Direction::SouthWest: {
      if (!CanPathOn(from, East(to), radius) && !CanPathOn(from, North(to), radius)) {
        return false;
      }
    } break;
    case Direction::NorthEast: {
      if (!CanPathOn(from, West(to), radius) && !CanPathOn(from, South(to), radius)) {
        return false;
      }
    } break;
    case Direction::SouthEast: {
      if (!CanPathOn(from, West(to), radius) && !CanPathOn(from, North(to), radius)) {
        return false;
      }
    } break;
  }
#endif

  return true;
}

/*
the start_corner is the ships orientation on the position its checking
0,0 = top left, 0,1 = bottom left, 1,0 = top right, 1,1 = bottom right
this method only allows checks at the ships corners
*/
bool Map::CornerPointCheck(const Vector2f& start, bool right_corner_check, bool bottom_corner_check,
                           float radius) const {
  u16 diameter = (u16)((radius + 0.5f) * 2.0f);

  u16 start_x = (u16)start.x;
  u16 start_y = (u16)start.y;

  if (right_corner_check == true) {
    start_x -= (diameter - 1);
  }

  if (bottom_corner_check == true) {
    start_y -= (diameter - 1);
  }

  u16 end_x = start_x + (diameter - 1);
  u16 end_y = start_y + (diameter - 1);

  for (u16 y = start_y; y <= end_y; ++y) {
    for (u16 x = start_x; x <= end_x; ++x) {
      if (IsSolid(x, y)) {
        return false;
      }
    }
  }
  return true;
}

bool Map::CornerPointCheck(int sX, int sY, int diameter) const {
  for (int y = 0; y < diameter; ++y) {
    for (int x = 0; x < diameter; ++x) {
      uint16_t world_x = (uint16_t)(sX + x);
      uint16_t world_y = (uint16_t)(sY + y);
      if (IsSolid(world_x, world_y)) {
        return false;
      }
    }
  }
  return true;
}
//#endif
/*
look at the starting tile and see if the ship can step into the ending tile
orientation is the ships current orientation on the start tile
if the ship is a 2x2 tile square, then an orientation of 0,0 would mean the ship is oriented
so its top left corner is occupying the start tile

this works with a flood fill method where a north/south/east/west lookup would increment
the orientation of the ship tile before calling the function, and the function does bounds checking
to make sure the orientation stays with in the ships diameter
*/
#if 0
bool Map::CanStepInto(const Vector2f& start, const Vector2f& end, Vector2f* orientation, float radius) const {
  int diameter = (int)((radius + 0.5f) * 2.0f);

  Vector2f offset = *orientation;

  if (offset.x > diameter) {
    offset.x = diameter;
  }
  if (offset.x < 0) {
    offset.x = 0;
  }
  if (offset.y > diameter) {
    offset.y = diameter;
  }
  if (offset.y < 0) {
    offset.y = 0;
  }

  *orientation = offset;

  Vector2f direction = end - start;
  int offset_x = start.x - offset.x;
  int offset_y = start.y - offset.y;

  for (int x = offset_x; x < offset_x + diameter; x++) {
    for (int y = offset_y; y < offset_y + diameter; y++) {
      if (IsSolid(u16(x + direction.x), u16(y + direction.y))) {
        return false;
      }
    }
  }
  return true;
}
#endif


// doesnt seem to work in the pathfinder
bool Map::CanTraverse(const Vector2f& start, const Vector2f& end, float radius) const {
  if (!CanOverlapTile(start, radius)) return false;
  if (!CanOverlapTile(end, radius)) return false;

  Vector2f cross = Perpendicular(Normalize(start - end));

  bool left_solid = IsSolid(start + cross);
  bool right_solid = IsSolid(start - cross);

  if (left_solid) {
    for (float i = 0; i < radius * 2.0f; ++i) {
      if (!CanOverlapTile(start - cross * i, radius)) {
        return false;
      }

      if (!CanOverlapTile(end - cross * i, radius)) {
        return false;
      }
    }

    return true;
  }

  if (right_solid) {
    for (float i = 0; i < radius * 2.0f; ++i) {
      if (!CanOverlapTile(start + cross * i, radius)) {
        return false;
      }

      if (!CanOverlapTile(end + cross * i, radius)) {
        return false;
      }
    }

    return true;
  }

  return true;
}

OccupyRect Map::GetPossibleOccupyRect(const Vector2f& position, float radius) const {
  OccupyRect result = {};

  u16 d = (u16)(radius * 2.0f);
  u16 start_x = (u16)position.x;
  u16 start_y = (u16)position.y;

  u16 far_left = start_x - d;
  u16 far_right = start_x + d;
  u16 far_top = start_y - d;
  u16 far_bottom = start_y + d;

  result.occupy = false;

  // Handle wrapping that can occur from using unsigned short
  if (far_left > 1023) far_left = 0;
  if (far_right > 1023) far_right = 1023;
  if (far_top > 1023) far_top = 0;
  if (far_bottom > 1023) far_bottom = 1023;

  bool solid = IsSolid(start_x, start_y);
  if (d < 1 || solid) {
    result.occupy = !solid;
    result.start_x = (u16)position.x;
    result.start_y = (u16)position.y;
    result.end_x = (u16)position.x;
    result.end_y = (u16)position.y;

    return result;
  }

  // Loop over the entire check region and move in the direction of the check tile.
  // This makes sure that the check tile is always contained within the found region.
  for (u16 check_y = far_top; check_y <= far_bottom; ++check_y) {
    s16 dir_y = (start_y - check_y) > 0 ? 1 : (start_y == check_y ? 0 : -1);

    // Skip cardinal directions because the radius is >1 and must be found from a corner region.
    if (dir_y == 0) continue;

    for (u16 check_x = far_left; check_x <= far_right; ++check_x) {
      s16 dir_x = (start_x - check_x) > 0 ? 1 : (start_x == check_x ? 0 : -1);

      if (dir_x == 0) continue;

      bool can_fit = true;

      for (s16 y = check_y; std::abs(y - check_y) <= d && can_fit; y += dir_y) {
        for (s16 x = check_x; std::abs(x - check_x) <= d; x += dir_x) {
          if (IsSolid(x, y)) {
            can_fit = false;
            break;
          }
        }
      }

      if (can_fit) {
        // Calculate the final region. Not necessary for simple overlap check, but might be useful
        u16 found_start_x = 0;
        u16 found_start_y = 0;
        u16 found_end_x = 0;
        u16 found_end_y = 0;

        if (check_x > start_x) {
          found_start_x = check_x - d;
          found_end_x = check_x;
        } else {
          found_start_x = check_x;
          found_end_x = check_x + d;
        }

        if (check_y > start_y) {
          found_start_y = check_y - d;
          found_end_y = check_y;
        } else {
          found_start_y = check_y;
          found_end_y = check_y + d;
        }

        result.start_x = found_start_x;
        result.start_y = found_start_y;
        result.end_x = found_end_x;
        result.end_y = found_end_y;

        result.occupy = true;
        return result;
      }
    }
  }

  return result;
}

// Rects must be initialized memory that can contain all possible occupy rects.
size_t Map::GetAllOccupiedRects(Vector2f position, float radius, OccupiedRect* rects) const {
  size_t count = 0;

  u16 d = (u16)(radius * 2.0f);
  u16 start_x = (u16)position.x;
  u16 start_y = (u16)position.y;

  u16 far_left = start_x - d;
  u16 far_right = start_x + d;
  u16 far_top = start_y - d;
  u16 far_bottom = start_y + d;

  // Handle wrapping that can occur from using unsigned short
  if (far_left > 1023) far_left = 0;
  if (far_right > 1023) far_right = 1023;
  if (far_top > 1023) far_top = 0;
  if (far_bottom > 1023) far_bottom = 1023;

  bool solid = IsSolid(start_x, start_y);
  if (d < 1 || solid) {
    rects->start_x = (u16)position.x;
    rects->start_y = (u16)position.y;
    rects->end_x = (u16)position.x;
    rects->end_y = (u16)position.y;

    return !solid;
  }

  // Loop over the entire check region and move in the direction of the check tile.
  // This makes sure that the check tile is always contained within the found region.
  for (u16 check_y = far_top; check_y <= far_bottom; ++check_y) {
    s16 dir_y = (start_y - check_y) > 0 ? 1 : (start_y == check_y ? 0 : -1);

    // Skip cardinal directions because the radius is >1 and must be found from a corner region.
    if (dir_y == 0) continue;

    for (u16 check_x = far_left; check_x <= far_right; ++check_x) {
      s16 dir_x = (start_x - check_x) > 0 ? 1 : (start_x == check_x ? 0 : -1);

      if (dir_x == 0) continue;

      bool can_fit = true;

      for (s16 y = check_y; std::abs(y - check_y) <= d && can_fit; y += dir_y) {
        for (s16 x = check_x; std::abs(x - check_x) <= d; x += dir_x) {
          if (IsSolid(x, y)) {
            can_fit = false;
            break;
          }
        }
      }

      if (can_fit) {
        // Calculate the final region. Not necessary for simple overlap check, but might be useful
        u16 found_start_x = 0;
        u16 found_start_y = 0;
        u16 found_end_x = 0;
        u16 found_end_y = 0;

        if (check_x > start_x) {
          found_start_x = check_x - d;
          found_end_x = check_x;
        } else {
          found_start_x = check_x;
          found_end_x = check_x + d;
        }

        if (check_y > start_y) {
          found_start_y = check_y - d;
          found_end_y = check_y;
        } else {
          found_start_y = check_y;
          found_end_y = check_y + d;
        }

        OccupiedRect* rect = rects + count++;

        rect->start_x = found_start_x;
        rect->start_y = found_start_y;
        rect->end_x = found_end_x;
        rect->end_y = found_end_y;
      }
    }
  }

  return count;
}

Vector2f Map::GetOccupyCenter(const Vector2f& position, float radius) const {
  u16 d = (u16)(radius * 2.0f);
  u16 start_x = (u16)position.x;
  u16 start_y = (u16)position.y;

  u16 far_left = start_x - d;
  u16 far_right = start_x + d;
  u16 far_top = start_y - d;
  u16 far_bottom = start_y + d;

  // Handle wrapping that can occur from using unsigned short
  if (far_left > 1023) far_left = 0;
  if (far_right > 1023) far_right = 1023;
  if (far_top > 1023) far_top = 0;
  if (far_bottom > 1023) far_bottom = 1023;

  bool solid = IsSolid(start_x, start_y);
  if (d < 1 || solid) {
    return position;
  }

  Vector2f accum;
  size_t count = 0;

  // Loop over the entire check region and move in the direction of the check tile.
  // This makes sure that the check tile is always contained within the found region.
  for (u16 check_y = far_top; check_y <= far_bottom; ++check_y) {
    s16 dir_y = (start_y - check_y) > 0 ? 1 : (start_y == check_y ? 0 : -1);

    // Skip cardinal directions because the radius is >1 and must be found from a corner region.
    if (dir_y == 0) continue;

    for (u16 check_x = far_left; check_x <= far_right; ++check_x) {
      s16 dir_x = (start_x - check_x) > 0 ? 1 : (start_x == check_x ? 0 : -1);

      if (dir_x == 0) continue;

      bool can_fit = true;

      for (s16 y = check_y; std::abs(y - check_y) <= d && can_fit; y += dir_y) {
        for (s16 x = check_x; std::abs(x - check_x) <= d; x += dir_x) {
          if (IsSolid(x, y)) {
            can_fit = false;
            break;
          }
        }
      }

      if (can_fit) {
        // Calculate the final region. Not necessary for simple overlap check, but might be useful
        u16 found_start_x = 0;
        u16 found_start_y = 0;
        u16 found_end_x = 0;
        u16 found_end_y = 0;

        if (check_x > start_x) {
          found_start_x = check_x - d;
          found_end_x = check_x;
        } else {
          found_start_x = check_x;
          found_end_x = check_x + d;
        }

        if (check_y > start_y) {
          found_start_y = check_y - d;
          found_end_y = check_y;
        } else {
          found_start_y = check_y;
          found_end_y = check_y + d;
        }

        Vector2f min((float)found_start_x, (float)found_start_y);
        Vector2f max((float)found_end_x + 1, (float)found_end_y + 1);

        accum += (min + max) * 0.5f;
        ++count;
      }
    }
  }

  if (count <= 0) return position;

  return accum * (1.0f / count);
}

// verified working with debugger
bool Map::CanOverlapTile(const Vector2f& position, float radius) const {
  u16 d = (u16)(radius * 2.0f);
  u16 start_x = (u16)position.x;
  u16 start_y = (u16)position.y;

  u16 far_left = start_x - d;
  u16 far_right = start_x + d;
  u16 far_top = start_y - d;
  u16 far_bottom = start_y + d;

  // Handle wrapping that can occur from using unsigned short
  if (far_left > 1023) far_left = 0;
  if (far_right > 1023) far_right = 1023;
  if (far_top > 1023) far_top = 0;
  if (far_bottom > 1023) far_bottom = 1023;

  bool solid = IsSolid(start_x, start_y);
  if (d < 1 || solid) return !solid;

  // Loop over the entire check region and move in the direction of the check tile.
  // This makes sure that the check tile is always contained within the found region.
  for (u16 check_y = far_top; check_y <= far_bottom; ++check_y) {
    s16 dir_y = (start_y - check_y) > 0 ? 1 : (start_y == check_y ? 0 : -1);

    // Skip cardinal directions because the radius is >1 and must be found from a corner region.
    if (dir_y == 0) continue;

    for (u16 check_x = far_left; check_x <= far_right; ++check_x) {
      s16 dir_x = (start_x - check_x) > 0 ? 1 : (start_x == check_x ? 0 : -1);

      if (dir_x == 0) continue;

      bool can_fit = true;

      for (s16 y = check_y; std::abs(y - check_y) <= d && can_fit; y += dir_y) {
        for (s16 x = check_x; std::abs(x - check_x) <= d; x += dir_x) {
          if (IsSolid(x, y)) {
            can_fit = false;
            break;
          }
        }
      }

      if (can_fit) {
        return true;
      }
    }
  }

  return false;
}
#if 0
bool Map::CanOccupy(const Vector2f& position, float radius) const {
  /* Convert the ship into a tiled grid and put each tile of the ship on the test
     position.

     If the ship can get any part of itself on the tile return true.
  */
  if (IsSolid(position)) {
    return false;
  }

  // casting the result to int always rounds towards 0
  int tile_diameter = (int)((radius + 0.5f) * 2);

  if (tile_diameter == 0) {
    return true;
  }

  for (int y = -(tile_diameter - 1); y <= 0; ++y) {
    for (int x = -(tile_diameter - 1); x <= 0; ++x) {
      if (CornerPointCheck((int)position.x + x, (int)position.y + y, tile_diameter)) {
        return true;
      }
    }
  }
  return false;
}
#endif
bool Map::CanOccupyRadius(const Vector2f& position, float radius) const {
  // rounds 2 tile ships to a 3 tile search

  if (IsSolid(position)) {
    return false;
  }

  radius = std::floor(radius + 0.5f);

  for (float y = -radius; y <= radius; ++y) {
    for (float x = -radius; x <= radius; ++x) {
      uint16_t world_x = (uint16_t)(position.x + x);
      uint16_t world_y = (uint16_t)(position.y + y);
      if (IsSolid(world_x, world_y)) {
        return false;
      }
    }
  }
  return true;
}



struct Tile {
  u32 x : 12;
  u32 y : 12;
  u32 tile : 8;
};

std::unique_ptr<Map> Map::Load(const char* filename) {
  std::ifstream input(filename, std::ios::in | std::ios::binary);

  if (!input.is_open()) return nullptr;

  input.seekg(0, std::ios::end);
  std::size_t size = static_cast<std::size_t>(input.tellg());
  input.seekg(0, std::ios::beg);

  std::string data;
  data.resize(size);

  input.read(&data[0], size);

  std::size_t pos = 0;
  if (data[0] == 'B' && data[1] == 'M') {
    pos = *(u32*)(&data[2]);
  }

  TileData tiles(kMapExtent * kMapExtent);

  while (pos < size) {
    Tile tile = *reinterpret_cast<Tile*>(&data[pos]);

    tiles[tile.y * kMapExtent + tile.x] = tile.tile;

    if (tile.tile == 217) {
      // Mark large asteroid
      for (std::size_t y = 0; y < 2; ++y) {
        for (std::size_t x = 0; x < 2; ++x) {
          tiles[(tile.y + y) * kMapExtent + (tile.x + x)] = tile.tile;
        }
      }
    } else if (tile.tile == 219) {
      // Mark space station tiles
      for (std::size_t y = 0; y < 6; ++y) {
        for (std::size_t x = 0; x < 6; ++x) {
          tiles[(tile.y + y) * kMapExtent + (tile.x + x)] = tile.tile;
        }
      }
    } else if (tile.tile == 220) {
      // Mark wormhole
      for (std::size_t y = 0; y < 5; ++y) {
        for (std::size_t x = 0; x < 5; ++x) {
          tiles[(tile.y + y) * kMapExtent + (tile.x + x)] = tile.tile;
        }
      }
    } 

    pos += sizeof(Tile);
  }

  return std::make_unique<Map>(tiles);
}

std::unique_ptr<Map> Map::Load(const std::string& filename) {
  return Load(filename.c_str());
}

bool IsValidPosition(MapCoord coord) {
  return coord.x >= 0 && coord.x < 1024 && coord.y >= 0 && coord.y < 1024;
}

}  // namespace marvin