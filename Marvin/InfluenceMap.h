#pragma once

#include "GameProxy.h"
#include "RayCaster.h"

namespace marvin {

class Vector2f;
class Bot;

enum class RayWidth : short { One, Three, Five , Seven };

namespace triangle {



struct Triangle {
  Vector2f vertices[3];

  inline bool Contains(const Vector2f& pt) {
    const Vector2f& v1 = vertices[0];
    const Vector2f& v2 = vertices[1];
    const Vector2f& v3 = vertices[2];

    float d1, d2, d3;
    bool has_neg, has_pos;

    d1 = sign(pt, v1, v2);
    d2 = sign(pt, v2, v3);
    d3 = sign(pt, v3, v1);

    has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(has_neg && has_pos);
  }

  float sign(const Vector2f& p1, const Vector2f& p2, const Vector2f& p3) {
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
  }
};
}  // namespace triangle

class InfluenceMap {
 public:
  InfluenceMap() : map_extent_(1023) { tiles = new float[1024 * 1024]; }
  void DebugUpdate(const Vector2f& position);

  float GetValue(uint16_t x, uint16_t y);
  float GetValue(Vector2f v);

  void AddValue(uint16_t x, uint16_t y, float value);
  void SetValue(uint16_t x, uint16_t y, float value);

  void Clear();
  void Decay(float dt);

  void CastPlayer(const Map& map, const Player& player, Bot& bot);

  void CastWeapons(Bot& bot);
  void CastWeapon(const Map& map, Weapon* weapon, Bot& bot);

  CastResult CastInfluence(const Map& map, Vector2f from, Vector2f direction, float max_length, RayWidth width,
                           float value, bool perform_collision);
  CastResult SpreadInfluence(const Map& map, Vector2f from, Vector2f direction, float max_length, float value);
  void FloodFillInfluence(const Map& map, GameProxy& game, const Player& player, float radius);

  void CastWeaponOld(const Map& map, Vector2f from, Vector2f direction, float max_length, float value, Weapon* weapon);
  void CastPlayerOld(const Map& map, Vector2f from, Vector2f direction, float max_length, float value);

 private:
  int map_extent_;
  float* tiles;
};

}  // namespace marvin
