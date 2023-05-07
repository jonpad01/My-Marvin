#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Types.h"
#include "Vector2f.h"

namespace marvin {

	struct MapCoord;

constexpr std::size_t kMapExtent = 1024;

using TileId = u8;
using TileData = std::vector<TileId>;

constexpr TileId kSafeTileId = 171;

struct OccupyRect {
  bool occupy;

  u16 start_x;
  u16 start_y;
  u16 end_x;
  u16 end_y;
};

class Map {
 public:
  Map(const TileData& tile_data);

  bool IsSolid(TileId id) const;
  bool IsSolid(u16 x, u16 y) const;
  bool IsSolid(const Vector2f& position) const;
  TileId GetTileId(u16 x, u16 y) const;
  TileId GetTileId(const Vector2f& position) const;

  // Checks if the ship can go from one tile directly to another.
  bool CanTraverse(const Vector2f& start, const Vector2f& end, float radius) const;
  bool CanOverlapTile(const Vector2f& position, float radius) const;
  // Returns a possible rect that creates an occupiable area that contains the tested position.
  OccupyRect GetPossibleOccupyRect(const Vector2f& position, float radius) const;

  bool CanOccupy(const Vector2f& position, float radius) const;
  bool CanOccupyRadius(const Vector2f& position, float radius) const;
  bool CanStepInto(const Vector2f& start, const Vector2f& end, Vector2f* orientation, float radius) const;
  bool CornerPointCheck(int sX, int sY, int diameter) const;
  bool CornerPointCheck(const Vector2f& start, bool right_corner_check, bool bottom_corner_check, float radius) const;
  bool CanPathOn(const Vector2f& position, float radius) const;
  void SetTileId(u16 x, u16 y, TileId id);
  void SetTileId(const Vector2f& position, TileId id);

  static std::unique_ptr<Map> Load(const char* filename);
  static std::unique_ptr<Map> Load(const std::string& filename);

 private:
  TileData tile_data_;
};

bool IsValidPosition(MapCoord coord);

}  // namespace marvin