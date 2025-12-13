#pragma once

#include <memory>
#include <string>
#include <vector>
#include <bitset>
#include <span>
#include <unordered_map>

#include "Types.h"
#include "Vector2f.h"
#include "GameProxy.h"

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

// Reduced version of OccupyRect to remove bool so it fits more in a cache line
struct OccupiedRect {
  u16 start_x;
  u16 start_y;
  u16 end_x;
  u16 end_y;

  bool operator==(const OccupiedRect& other) const {
    return start_x == other.start_x && start_y == other.start_y && end_x == other.end_x && end_y == other.end_y;
  }

  inline bool Contains(Vector2f position) const {
    u16 x = (u16)position.x;
    u16 y = (u16)position.y;

    return x >= start_x && x <= end_x && y >= start_y && y <= end_y;
  }
};

constexpr size_t kMaxOccupySet = 96;
struct OccupyMap {
  u32 count = 0;
  std::bitset<kMaxOccupySet> set;

  bool operator[](size_t index) const { return set[index]; }
};

class Map {
 public:
  Map(const TileData& tile_data);

  bool IsSolid(TileId id) const;
  bool IsSolid(u16 x, u16 y) const;
  bool IsSolidSquare(MapCoord top_left, uint16_t length) const;
  bool IsSolid(const Vector2f& position) const;
  TileId GetTileId(u16 x, u16 y) const;
  TileId GetTileId(const Vector2f& position) const;

  // Checks if the ship can go from one tile directly to another.
  bool CanTraverse(const Vector2f& start, const Vector2f& end, float radius) const;
  bool CanOverlapTile(const Vector2f& position, float radius) const;
  // Returns a possible rect that creates an occupiable area that contains the tested position.
  OccupyRect GetPossibleOccupyRect(const Vector2f& position, float radius) const;
  Vector2f GetOccupyCenter(const Vector2f& position, float radius) const;
  size_t GetAllOccupiedRects(Vector2f position, float radius, OccupiedRect* rects) const;

  bool CanMoveTo(MapCoord from, MapCoord to, float radius) const ;
  OccupyMap CalculateOccupyMap(MapCoord start, float radius) const;
  bool CanOccupy(const Vector2f& position, float radius) const;
  bool CanOccupyRadius(const Vector2f& position, float radius) const;
  bool CanStepInto(const Vector2f& start, const Vector2f& end, Vector2f* orientation, float radius) const;
  bool CornerPointCheck(int sX, int sY, int diameter) const;
  bool CornerPointCheck(const Vector2f& start, bool right_corner_check, bool bottom_corner_check, float radius) const;
  bool CanPathOn(const Vector2f& position, float radius) const;
  void SetTileId(u16 x, u16 y, TileId id);
  void SetTileId(const Vector2f& position, TileId id);

  bool IsMined(MapCoord coord) const;
  void SetMinedTile(MapCoord coord);
  void SetMinedTiles(std::vector<Weapon*> tiles);
  void ClearMinedTiles();

  const std::vector<std::string>* GetRegions(MapCoord coord) const;
  bool HasRegion(const std::string& name) const;
  const std::bitset<1024 * 1024>* GetTileSet(std::string name) const;
  bool HasRegions() const { return !regions.empty(); }

  static std::unique_ptr<Map> Load(const char* filename);
  static std::unique_ptr<Map> Load(const std::string& filename);

 private:

  bool LoadMetaData(unsigned char* start, u32 len);
  bool ProcessRegionChunk(unsigned char* start, u32 len);
  bool DecodeRegionTiles(std::span<const std::byte> data, std::bitset<1024 * 1024>* tiles);

  std::unordered_map<std::string, std::bitset<1024 * 1024>> regions;



  TileData tile_data_;
  bool mine_map[1024 * 1024];
  std::vector<std::size_t> mined_tile_index_list;
};

bool IsValidPosition(MapCoord coord);

}  // namespace marvin