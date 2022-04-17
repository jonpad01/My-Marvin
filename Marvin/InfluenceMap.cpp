#include "InfluenceMap.h"

#include "Bot.h"
#include "Debug.h"
#include "Map.h"
#include "Vector2f.h"
#include "platform/Platform.h"

namespace marvin {

float InfluenceMap::GetValue(uint16_t x, uint16_t y) {
  return tiles[y * 1024 + x];
}

float InfluenceMap::GetValue(Vector2f v) {
  return tiles[(uint16_t)v.y * 1024 + (uint16_t)v.x];
}

void InfluenceMap::AddValue(uint16_t x, uint16_t y, float value) {
  tiles[y * 1024 + x] += value;
}

void InfluenceMap::SetValue(uint16_t x, uint16_t y, float value) {
  tiles[y * 1024 + x] = value;
}

void InfluenceMap::Clear() {
  for (size_t i = 0; i < 1024 * 1024; ++i) {
    tiles[i] = 0.0f;
  }
}

void InfluenceMap::Decay(float dt) {
  for (size_t i = 0; i < 1024 * 1024; ++i) {
    tiles[i] = tiles[i] - dt;
    if (tiles[i] < 0) {
      tiles[i] = 0;
    }
  }
}

void InfluenceMap::Update(GameProxy& game) {
  const float kInfluenceValue = 1.0f;


  for (Weapon* weapon : game.GetWeapons()) {
    const Player* player = game.GetPlayerById(weapon->GetPlayerId());

    if (player == nullptr || player->frequency == game.GetPlayer().frequency) {
      continue;
    }

    if (weapon->GetData().type == WeaponType::Decoy) continue;
    if (weapon->GetData().type == WeaponType::Repel) continue;
    if (weapon->GetData().type == WeaponType::None) continue;

    

    Vector2f position = weapon->GetPosition();
    Vector2f velocity = weapon->GetVelocity();
    Vector2f direction = Normalize(velocity);
    float influence_length = (velocity * (weapon->GetRemainingTicks() / 100.0f)).Length();
    Vector2f side = Normalize(Perpendicular(velocity));

    u32 damage = 0;
    
    switch (weapon->GetData().type) {
      case WeaponType::Thor:
      case WeaponType::ProximityBomb:
      case WeaponType::Bomb: {
        damage = game.GetSettings().BombDamageLevel;
      } break;
      case WeaponType::Burst: {
        damage = game.GetSettings().BurstDamageLevel;

      } break;
      default: {
        damage =
            game.GetSettings().BulletDamageLevel + game.GetSettings().BulletDamageUpgrade * weapon->GetData().level;
      } break;
    }

    // Calculate a value for the weapon being casted into the influence map.
    // Value ranges from 0 to 1.0 depending on how much damage it does.
    float max_energy = game.GetMaxEnergy();
    float value = damage / max_energy;

    if (value > 1.0f) {
      value = 1.0f;
    }

    if (game.GetMap().IsSolid(weapon->GetPosition())) continue;

    CastWeapon(game.GetMap(), weapon->GetPosition(), Normalize(weapon->GetVelocity()), influence_length, value, weapon);

    //CastReflectedInfluence(game.GetMap(), position, direction, side, influence_length, kInfluenceValue);
    //CastReflectedInfluence(game.GetMap(), position + side, direction, influence_length, kInfluenceValue);
    //CastReflectedInfluence(game.GetMap(), position - side, direction, influence_length, kInfluenceValue);
  }

  for (std::size_t i = 0; i < game.GetEnemys().size(); i++) {
    const Player& player = game.GetEnemys()[i];

    Vector2f position = player.position;
    Vector2f velocity = player.velocity;
    Vector2f direction = Normalize(velocity);
    float speed = velocity.Length();
    float influence_length = 2.0f * speed;
    Vector2f side = Normalize(Perpendicular(velocity));

  //g_RenderState.RenderDebugText("  Enemy Speed: %f", speed);


    //CastInfluence(game.GetMap(), position, direction, side, influence_length, kInfluenceValue);
    //CastInfluence(game.GetMap(), position + side, direction, influence_length, kInfluenceValue);
    //CastInfluence(game.GetMap(), position - side, direction, influence_length, kInfluenceValue);
  }
#if DEBUG_RENDER
        for (i32 y = -50; y < 50; ++y) {
            for (i32 x = -50; x < 50; ++x) {
                Vector2f pos = game.GetPosition();
                Vector2f check(std::floor(pos.x) + x, std::floor(pos.y) + y);

                if (check.x > 1023.0f) {
                  check.x = 1023.0f;
                }
                if (check.x < 1.0f) {
                  check.x = 1.0f;
                }
                if (check.y > 1023.0f) {
                  check.y = 1023.0f;
                }
                if (check.y < 1.0f) {
                  check.y = 1.0f;
                }
                float value = GetValue(check);

                if (value >= 0.1f) {
                    int r = (int)(std::min(value * (255 / kInfluenceValue), 255.0f));
                    RenderWorldLine(game.GetPosition(), check, check + Vector2f(1, 1), RGB(r, 100, 100));
                    RenderWorldLine(game.GetPosition(), check + Vector2f(0, 1), check + Vector2f(1, 0), RGB(r, 100, 100));
                }
            }
        }
#endif
}
void InfluenceMap::CastInfluence(const Map& map, Vector2f from, Vector2f direction, float max_length, float value) {
  //CastResult InfluenceMap::CastInfluence(const Map& map, const Vector2f& from, const Vector2f& direction,
                                      // Vector2f side, float max_length, float value) {
  CastResult result;
  //Vector2f side = Perpendicular(direction);

  if (map.IsSolid((unsigned short)from.x, (unsigned short)from.y)) {
    //return result;
    return;
  }

  Vector2f vMapSize = {1024.0f, 1024.0f};

  float xStepSize = std::sqrt(1 + (direction.y / direction.x) * (direction.y / direction.x));
  float yStepSize = std::sqrt(1 + (direction.x / direction.y) * (direction.x / direction.y));

  Vector2f vMapCheck = Vector2f(std::floor(from.x), std::floor(from.y));
  Vector2f vRayLength1D;

  Vector2f vStep;

  if (direction.x < 0) {
    vStep.x = -1.0f;
    vRayLength1D.x = (from.x - float(vMapCheck.x)) * xStepSize;
  } else {
    vStep.x = 1.0f;
    vRayLength1D.x = (float(vMapCheck.x + 1) - from.x) * xStepSize;
  }

  if (direction.y < 0) {
    vStep.y = -1.0f;
    vRayLength1D.y = (from.y - float(vMapCheck.y)) * yStepSize;
  } else {
    vStep.y = 1.0f;
    vRayLength1D.y = (float(vMapCheck.y + 1) - from.y) * yStepSize;
  }

  // Perform "Walk" until collision or range check
  bool bTileFound = false;
  float fDistance = 0.0f;

  while (!bTileFound && fDistance < max_length) {
    // Walk along shortest path
    if (vRayLength1D.x < vRayLength1D.y) {
      vMapCheck.x += vStep.x;
      fDistance = vRayLength1D.x;
      vRayLength1D.x += xStepSize;
    } else {
      vMapCheck.y += vStep.y;
      fDistance = vRayLength1D.y;
      vRayLength1D.y += yStepSize;
    }

    // Test tile at new test point
    if (vMapCheck.x >= 0 && vMapCheck.x < vMapSize.x && vMapCheck.y >= 0 && vMapCheck.y < vMapSize.y) {
      if (map.IsSolid((unsigned short)vMapCheck.x, (unsigned short)vMapCheck.y)) {
        bTileFound = true;
      } else {
        SetValue((uint16_t)vMapCheck.x, (uint16_t)vMapCheck.y, value);

        //SetValue((uint16_t)std::ceil(vMapCheck.x + side.x), (uint16_t)std::ceil(vMapCheck.y + side.y), value);
        //SetValue((uint16_t)std::floor(vMapCheck.x - side.x), (uint16_t)std::floor(vMapCheck.y - side.y), value);
      }
    }
  }
  /*
  Vector2f vIntersection;
  if (bTileFound) {
    //step the distance, and intersection position back 1 to the last non solid tile.
    //fDistance -= 1.0f;
    vIntersection = from + direction * fDistance;
    float dist;

    result.hit = true;
    result.distance = fDistance;
    result.position = vIntersection;

    // wall tiles are checked and returns distance + norm
    RayBoxIntersect(from, direction, vMapCheck, Vector2f(1.0f, 1.0f), &dist, &result.normal);
  }

  return result;
  */
}

void InfluenceMap::CastWeapon(const Map& map, Vector2f from, Vector2f direction, float max_length, float value,
                              Weapon* weapon) {
  if (map.IsSolid((unsigned short)from.x, (unsigned short)from.y)) {
    return;
  }

  Vector2f vMapSize = {1024.0f, 1024.0f};

  float total_distance = 0.0f;
  float total_max = max_length;

  int bounces_remaining = 100000;

  if (weapon->GetData().type == WeaponType::Bomb || weapon->GetData().type == WeaponType::ProximityBomb) {
    bounces_remaining = weapon->GetRemainingBounces();
  }

  while (total_distance < total_max) {
    float xStepSize = std::sqrt(1 + (direction.y / direction.x) * (direction.y / direction.x));
    float yStepSize = std::sqrt(1 + (direction.x / direction.y) * (direction.x / direction.y));

    Vector2f side = Perpendicular(direction);

    Vector2f vMapCheck = Vector2f(std::floor(from.x), std::floor(from.y));
    Vector2f vRayLength1D;

    Vector2f vStep;

    if (direction.x < 0) {
      vStep.x = -1.0f;
      vRayLength1D.x = (from.x - float(vMapCheck.x)) * xStepSize;
    } else {
      vStep.x = 1.0f;
      vRayLength1D.x = (float(vMapCheck.x + 1) - from.x) * xStepSize;
    }

    if (direction.y < 0) {
      vStep.y = -1.0f;
      vRayLength1D.y = (from.y - float(vMapCheck.y)) * yStepSize;
    } else {
      vStep.y = 1.0f;
      vRayLength1D.y = (float(vMapCheck.y + 1) - from.y) * yStepSize;
    }

    // Perform "Walk" until collision or range check
    bool bTileFound = false;
    float fDistance = 0.0f;

    while (!bTileFound && fDistance < max_length) {
      // Walk along shortest path
      if (vRayLength1D.x < vRayLength1D.y) {
        vMapCheck.x += vStep.x;
        fDistance = vRayLength1D.x;
        vRayLength1D.x += xStepSize;
      } else {
        vMapCheck.y += vStep.y;
        fDistance = vRayLength1D.y;
        vRayLength1D.y += yStepSize;
      }

      // Test tile at new test point
      if (vMapCheck.x >= 0 && vMapCheck.x < vMapSize.x && vMapCheck.y >= 0 && vMapCheck.y < vMapSize.y) {
        if (weapon->GetData().type != WeaponType::Thor &&
            map.IsSolid((unsigned short)vMapCheck.x, (unsigned short)vMapCheck.y)) {
          bTileFound = true;

          if (bounces_remaining-- <= 0) return;

          float dist;
          Vector2f normal;

          if (RayBoxIntersect(from, direction, vMapCheck, Vector2f(1.0f, 1.0f), &dist, &normal)) {
            from = (from + direction * dist);
            direction = Normalize(direction - (normal * (2.0f * (direction.Dot(normal)))));
            max_length -= fDistance;
          } else {
            return;
          }
        } else {
          SetValue((uint16_t)vMapCheck.x, (uint16_t)vMapCheck.y, value * (1.0f - (total_distance / total_max)));

          Vector2f side1 = vMapCheck + side;
          Vector2f side2 = vMapCheck - side;

          if (!map.IsSolid(side1)) {
            SetValue((uint16_t)side1.x, (uint16_t)side1.y, value * (1.0f - (total_distance / total_max)));
          }

          if (!map.IsSolid(side2)) {
            SetValue((uint16_t)side2.x, (uint16_t)side2.y, value * (1.0f - (total_distance / total_max)));
          }
        }
      }
    }

    total_distance += fDistance;
  }
}

void InfluenceMap::CastReflectedInfluence(const Map& map, const Vector2f& from, const Vector2f& direction,
                                          Vector2f side, float max_length, float value) {

    Vector2f current_direction = direction;

  //CastResult influence_cast = CastInfluence(map, from, direction, side, max_length, value);
 // float remaining_distance = max_length - influence_cast.distance;
  
  //  while (influence_cast.hit) {
   //   current_direction = current_direction - (influence_cast.normal * (2.0f * current_direction.Dot(influence_cast.normal)));
      //influence_cast = CastInfluence(map, influence_cast.position, current_direction, side, remaining_distance, value);
     // remaining_distance -= influence_cast.distance;
   // }
}
}  // namespace marvin
