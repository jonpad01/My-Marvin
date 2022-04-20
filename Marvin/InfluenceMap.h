#pragma once

#include "GameProxy.h"
#include "RayCaster.h"

namespace marvin {

class Vector2f;
class Bot;

class InfluenceMap {
 public:
  InfluenceMap() { tiles = new float[1024 * 1024]; }

  float GetValue(uint16_t x, uint16_t y);
  float GetValue(Vector2f v);

  void AddValue(uint16_t x, uint16_t y, float value);
  void SetValue(uint16_t x, uint16_t y, float value);

  void Clear();
  void Decay(float dt);

  void Update(GameProxy& game);

  void CastWeapon(const Map& map, Vector2f from, Vector2f direction, float max_length, float value, Weapon* weapon);
  void CastPlayer(const Map& map, Vector2f from, Vector2f direction, float max_length,float value);

 private:
  void DebugUpdate(GameProxy& game);
  float* tiles;
};

}  // namespace marvin
