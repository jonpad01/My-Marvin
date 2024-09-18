#include <fuse/Map.h>

namespace fuse {

bool Map::IsSolid(const Vector2f& position) const {
  return IsSolid((u16)position.x, (u16)position.y);
}

TileId Map::GetTileId(const Vector2f& position) const {
  return GetTileId((u16)position.x, (u16)position.y);
}

bool Map::IsSolid(u16 x, u16 y) const {
  TileId id = GetTileId(x, y);

  if (id == 0) return false;
  if (id >= 162 && id <= 169) return true;
  if (id < 170) return true;
  if (id >= 192 && id <= 240) return true;
  if (id >= 242 && id <= 252) return true;

  return false;
}

TileId Map::GetTileId(u16 x, u16 y) const {
  if (x >= 1024 || y >= 1024) return 252;
  if (map_data == nullptr) return 0;

  return map_data[y * kMapExtent + x];
}

}  // namespace fuse
