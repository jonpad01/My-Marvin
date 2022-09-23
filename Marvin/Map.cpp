#include "Map.h"

#include <fstream>

#include "RegionRegistry.h"

namespace marvin {

Map::Map(const TileData& tile_data) : tile_data_(tile_data) {}

TileId Map::GetTileId(u16 x, u16 y) const {
  if (x >= 1024 || y >= 1024) return 0;
  return tile_data_[y * kMapExtent + x];
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
  if (id >= 162 && id <= 169) return false;  // treat doors as non-solid
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

bool Map::CanPathOn(const Vector2f& position, float radius) const {
  /* Peg the check position to the same tile on the ship and expand the search for every tile it
  *  will occupy at that point.
  * 
  *  if the ship is a 2 x 2 square, the upper left square is the position and the radius expands down
     and to the right. This will always push the path to the upper left in tight spaces.

     note: This method allows diagonal steps through diagonal gaps, but those nodes need to still
     be considered pathathble for the bot to correctly path through tubes with non solid corners
  */

  int tile_diameter = (int)((radius + 0.5f) * 2);
  //Direction direction = GetDirection(Normalize(to - from));
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

/* 
look at the starting tile and see if the ship can step into the ending tile
orientation is the ships current orientation on the start tile
if the ship is a 2x2 tile square, then an orientation of 0,0 would mean the ship is oriented
so its top left corner is occupying the start tile

this works with a flood fill method where a north/south/east/west lookup would increment
the orientation of the ship tile before calling the function, and the function does bounds checking
to make sure the orientation stays with in the ships diameter
*/
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
    for (int x = -(tile_diameter -1); x <= 0; ++x) {
      if (CornerPointCheck((int)position.x + x, (int)position.y + y, tile_diameter)) {
        return true;
      }
    }
  }
  return false;
}

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

}  // namespace marvin
