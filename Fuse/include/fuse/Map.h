#pragma once

#include <fuse/Math.h>
#include <fuse/Types.h>

namespace fuse {

using TileId = u8;

constexpr u16 kMapExtent = 1024;
constexpr TileId kSafeTileId = 171;

class FUSE_EXPORT Map {
 public:
  Map() : map_data(nullptr) {}
  Map(u8* ptr) : map_data(ptr) {}

  inline bool IsLoaded() const { return map_data != nullptr; }

  bool IsSolid(const Vector2f& position) const;
  TileId GetTileId(const Vector2f& position) const;

  bool IsSolid(u16 x, u16 y) const;
  TileId GetTileId(u16 x, u16 y) const;

 private:
  u8* map_data = nullptr;
};

}  // namespace fuse
