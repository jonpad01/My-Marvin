#include "Map.h"
#include "RegionRegistry.h"

#include <fstream>

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

bool Map::CanOccupy(const Vector2f& position, float radius) const {

    //MapCoord map_position((uint16_t)std::floor(position.x), (uint16_t)std::floor(position.y));
    //int radius_check = 0;
    int radius_check = (int)(radius + 0.5f);
    //float  radius_check = 16.0f * radius;
#if 0
    uint16_t radius_check = (uint16_t)std::ceil(radius);
    
    MapCoord check_directions[]{ MapCoord(0, radius_check), MapCoord(0, -radius_check), MapCoord(radius_check, 0), MapCoord(-radius_check, 0) };
                               //MapCoord(radius_check, radius_check), MapCoord(radius_check, -radius_check), MapCoord(-radius_check, radius_check), MapCoord(-radius_check, -radius_check) };
    //check vertical
    if (!IsSolid(map_position.x, map_position.y + radius_check) && !IsSolid(map_position.x, map_position.y - radius_check)) {
        if (!IsSolid(map_position.x + radius_check, map_position.y) && !IsSolid(map_position.x - radius_check, map_position.y)) {
            return true;
        }
    }

//#if 0 
    for (MapCoord check_tile : check_directions) {

        MapCoord current(check_tile.x + map_position.x, check_tile.y + map_position.y);


        if (IsSolid((uint16_t)current.x, (uint16_t)current.y)) {
            continue;
        }
        
        if (!IsSolid((uint16_t)current.x, (uint16_t)current.y)) {
            return true;
        }
        
            if (!IsSolid((uint16_t)current.x, (uint16_t)current.y)) {
                if (current.x == map_position.x) {
                    if (!IsSolid(current.x, current.y + radius_check) && !IsSolid(current.x, current.y - radius_check)) {
                        return true;
                    }
                }
                else if (current.y == map_position.y) {
                    if (!IsSolid(current.x + radius_check, current.y) && !IsSolid(current.x - radius_check, current.y)) {
                        return true;
                    }
                }
            }
        
    }

#endif
//#if 0  
    for (int y = -radius_check; y <= radius_check; ++y) {
        for (int x = -radius_check; x <= radius_check; ++x) {
            uint16_t world_x = (uint16_t)(position.x + x);
            uint16_t world_y = (uint16_t)(position.y + y);

            if (IsSolid(world_x, world_y)) {
                return false;
            }
        }
    }
//#endif
   // return false;
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
