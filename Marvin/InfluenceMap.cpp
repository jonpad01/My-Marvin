#include "InfluenceMap.h"

#include  <queue>
#include "Bot.h"
#include "RayCaster.h"
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
  if (value > maximum_value_) {
    value = maximum_value_;
  }
  tiles[y * 1024 + x] = value;
}

void InfluenceMap::Clear() {
  for (size_t i = 0; i < 1024 * 1024; ++i) {
    tiles[i] = 0.0f;
  }
}

void InfluenceMap::Decay(const Vector2f& position, float max_speed, float dt, float decay_multiplier) {
/* 
* The maximum_value is how many seconds a tile will take to decay (default 1 second),
* max speed is in tiles per second. 
*
* Concept: If the ship is moving at a max speed of 17 tiles per second and the maximum decay time
* is 1 second then the decay function only needs to decay at a 17 tile distance.
* 
* This is pretty restrictive but it could still be used to watch heat areas around other players
* (such as protecting a team anchor) by feeding it that players position and the max speed for that
* players ship.
*/
  float padding = 5.0f;
  float decay_distance = (maximum_value_ * max_speed) / decay_multiplier + padding;

  for (float y = position.y - decay_distance; y <= position.y + decay_distance; ++y) {
    for (float x = position.x - decay_distance; x <= position.x + decay_distance; ++x) {
        // check if position isnt pushed off the edge of the map
      if (IsValidPosition(Vector2f(x, y))) {
        std::size_t index = (std::size_t)y * 1024 + (std::size_t)x;
        tiles[index] = tiles[index] - dt * decay_multiplier;
        if (tiles[index] < 0.0f) {
          tiles[index] = 0.0f;
        }
      }
    }
  }
}

void InfluenceMap::DebugUpdate(const Vector2f& position) {

  const float kInfluenceValue = 1.0f;

  for (i32 y = -50; y < 50; ++y) {
    for (i32 x = -50; x < 50; ++x) {
      Vector2f check(std::floor(position.x) + x, std::floor(position.y) + y);

      if (IsValidPosition(check)) {
        float value = GetValue(check);

        if (value >= 0.1f) {
          int r = (int)(std::min(value * (255 / kInfluenceValue), 255.0f));
          RenderWorldLine(position, check, check + Vector2f(1, 1), RGB(r, 100, 100));
          RenderWorldLine(position, check + Vector2f(0, 1), check + Vector2f(1, 0), RGB(r, 100, 100));
        }
      }
    }
  }
}


void InfluenceMap::CastWeapons(Bot& bot) {
  GameProxy& game = bot.GetGame();
  Vector2f position = game.GetPosition();

  Vector2f resolution(1920, 1080);
  Vector2f view_min_ = position - resolution / 2.0f / 16.0f;
  Vector2f view_max_ = position + resolution / 2.0f / 16.0f;

  for (Weapon* weapon : game.GetWeapons()) {
    if (InRect(weapon->GetPosition(), view_min_, view_max_)) {
      CastWeapon(game.GetMap(), weapon, bot);
    }
  }
}

void InfluenceMap::CastWeapon(const Map& map, Weapon* weapon, Bot& bot) {
  GameProxy& game = bot.GetGame();

  if (game.GetMap().IsSolid(weapon->GetPosition())) return;

  const Player* player = game.GetPlayerById(weapon->GetPlayerId());

  if (player == nullptr || player->frequency == game.GetPlayer().frequency) {
    return;
  }

  if (weapon->GetData().type == WeaponType::Decoy) return;
  if (weapon->GetData().type == WeaponType::Repel) return;
  if (weapon->GetData().type == WeaponType::None) return;

  int bounces_remaining = 100000;
  Vector2f velocity = weapon->GetVelocity();
  Vector2f direction = Normalize(velocity);
  Vector2f position = weapon->GetPosition();
  RayWidth width = RayWidth::One;
  bool perform_collision = true;
  float influence_length = (velocity * (weapon->GetRemainingTicks() / 100.0f)).Length();

  u32 damage = 0;

  u32 bullet_damage =
      (game.GetSettings().BulletDamageLevel + (game.GetSettings().BulletDamageUpgrade * weapon->GetData().level)) /
      1000;
  u32 bomb_damage = game.GetSettings().BombDamageLevel / 1000;
  u32 burst_damage = game.GetSettings().BurstDamageLevel / 1000;

  switch (weapon->GetData().type) {
    case WeaponType::Bomb: {
      bounces_remaining = weapon->GetRemainingBounces();
      damage = bomb_damage;
    } break;
    case WeaponType::ProximityBomb: {
      damage = bomb_damage;
      bounces_remaining = weapon->GetRemainingBounces();
      u32 prox_tiles = game.GetSettings().ProximityDistance + weapon->GetData().level;
      switch (prox_tiles) {
        case 2:
        case 3: {
          width = RayWidth::Three;
        }
        case 4:
        case 5: {
          width = RayWidth::Five;
        } break;
        case 6:
        case 7: {
          width = RayWidth::Seven;
        } break;
      }
    } break;
    case WeaponType::Thor: {
      perform_collision = false;
      damage = bomb_damage;
      u32 prox_tiles = game.GetSettings().ProximityDistance + weapon->GetData().level;
      switch (prox_tiles) {
        case 2:
        case 3: {
          width = RayWidth::Three;
        }
        case 4:
        case 5: {
          width = RayWidth::Five;
        } break;
        case 6:
        case 7: {
          width = RayWidth::Seven;
        } break;
      }
    } break;
    case WeaponType::Burst: {
      damage = burst_damage;
    } break;
    case WeaponType::BouncingBullet: {
      damage = bullet_damage;
    } break;
    case WeaponType::Bullet: {
      damage = bullet_damage;
      bounces_remaining = 0;
    } break;
  }

  // g_RenderState.RenderDebugText("  DAMAGE: %f", (float)damage);
  
  // Calculate a value for the weapon being casted into the influence map.
  // Value ranges from 0 to 1.0 depending on how much damage it does.
  // compare damage against highest damge weapon
  float value = (float)damage / (float)bomb_damage;
  CastResult result;

  do {
    result = CastInfluence(map, position, direction, influence_length, width, value, perform_collision);
    bounces_remaining--;

    position = result.position;
    direction = Vector2f(direction.x * result.normal.x, direction.y * result.normal.y);
    value = value * ((influence_length - result.distance) / influence_length);
    influence_length -= result.distance;
  } while (result.hit && bounces_remaining >= 0);
}


void InfluenceMap::CastPlayer(const Map& map, const Player& player, Bot& bot) {
  GameProxy& game = bot.GetGame();

    if (!player.active || !IsValidPosition(player.position)) {
    return;
    }
    if (game.GetMap().IsSolid(player.position)) return;

    const float influence_multiplier = 0.75f;

    Vector2f velocity = player.velocity;
    Vector2f direction = Normalize(velocity);
    Vector2f start_pos = player.position;
    float travelled_length = 0.0f;

    float rotation_offset = Vector2f(Normalize(velocity) + player.GetHeading()).Length();
    float speed = velocity.Length();
    float influence_length = (speed * rotation_offset * influence_multiplier) + 10.0f;
    float value = player.energy / (game.GetMaxEnergy());

   

    // step into player direction
    for (float i = 0.0f; i < influence_length; i++) {
      Vector2f side = Perpendicular(direction);
      Vector2f position = start_pos + direction * (i - travelled_length);
      if (map.IsSolid((unsigned short)position.x, (unsigned short)position.y)) {
       // direction = Vector2f(direction.x * result.normal.x, direction.y * result.normal.y);
      //  start_pos = result.position;
        travelled_length = i;
       // result = CastInfluence(map, start_pos, direction, i, RayWidth::Five, value);
      } else {
      // cast influence to each side
      //CastInfluence(game.GetMap(), position, side, influence_length - (influence_length - i), 1, value);
      //CastInfluence(game.GetMap(), position, -side, influence_length - (influence_length - i), 1, value);
     }
    }
}

void InfluenceMap::FloodFillInfluence(const Map& map, GameProxy& game, const Player& player, float radius) {
  if (!player.active || !IsValidPosition(player.position)) {
    return;
  }

  const float influence_multiplier = 1.0f;
  // a range of 0 - 2 (0 = flying backwards, 2 = flying forwards, 1 = sideways or stopped)
  float rotation_multiplier = Vector2f(Normalize(player.velocity) + player.GetHeading()).Length();

  if (rotation_multiplier < 1.0f) {
    rotation_multiplier = 1.0f;
  }

  struct VisitState {
    MapCoord coord;
    float distance;

    VisitState(MapCoord coord, float distance) : coord(coord), distance(distance) {}
  };

  std::queue<VisitState> queue;
  std::unordered_set<MapCoord> visited;

  // Start at the player's position and flood fill forward
  const Vector2f& start = player.position;

#if 1
  Vector2f direction = Normalize(player.velocity);
#else
  Vector2f direction = player.GetHeading();
#endif

  queue.emplace(start, 0.0f);
  visited.insert(start);

  constexpr float max_distance = 60.0f;

  // Create a barrier along the ship's side to stop backwards travel
  // These nodes will be considered visited so the queue nodes will not attempt to travel backwards
  {
    Vector2f side = Perpendicular(direction);

    for (float i = 0; i < 1024; ++i) {
      for (float j = 0; j < 3; ++j) {
        Vector2f current = start - direction * j + side * i;

        if (map.IsSolid(current)) break;

        visited.insert(current);
      }
    }

    for (float i = 0; i < 1024; ++i) {
      for (float j = 0; j < 3; ++j) {
        Vector2f current = start - direction * j - side * i;

        if (map.IsSolid(current)) break;

        visited.insert(current);
      }
    }
  }

  while (!queue.empty()) {
    // Grab the next tile from the queue
    VisitState state = queue.front();
    MapCoord coord = state.coord;
    Vector2f current(coord.x, coord.y);

    queue.pop();

    if (state.distance > max_distance) continue;

    // Set the value to be high near the player and fall off as the distance increases
    float value = 1.0f - state.distance / max_distance;
    SetValue((u16)current.x, (u16)current.y, value);

    // Visit the neighbors of this tile and increment the flood distance by 1
    const MapCoord west(coord.x - 1, coord.y);
    const MapCoord east(coord.x + 1, coord.y);
    const MapCoord north(coord.x, coord.y - 1);
    const MapCoord south(coord.x, coord.y + 1);

    if (visited.find(west) == visited.end() && map.CanOccupy(Vector2f(west.x, west.y), radius)) {
      queue.emplace(west, state.distance + 1.0f);
      visited.insert(west);
    }

    if (visited.find(east) == visited.end() && map.CanOccupy(Vector2f(east.x, east.y), radius)) {
      queue.emplace(east, state.distance + 1.0f);
      visited.insert(east);
    }

    if (visited.find(north) == visited.end() && map.CanOccupy(Vector2f(north.x, north.y), radius)) {
      queue.emplace(north, state.distance + 1.0f);
      visited.insert(north);
    }

    if (visited.find(south) == visited.end() && map.CanOccupy(Vector2f(south.x, south.y), radius)) {
      queue.emplace(south, state.distance + 1.0f);
      visited.insert(south);
    }
  }
}


CastResult InfluenceMap::CastInfluence(const Map& map, Vector2f from, Vector2f direction, float max_length, RayWidth width, float value, bool perform_collision) {
  CastResult result;

  Vector2f vRayUnitStepSize = Vector2f(abs(1.0f / direction.x), abs(1.0f / direction.y));
  
  Vector2f vMapCheck = Vector2f(std::floor(from.x), std::floor(from.y));
  Vector2f vRayLength1D, vStep, reflection;

  float fDistance = 0.0f;

  bool cornered = false, bTileFound = false;

  // fix for when stepping out of a bottom right corner both the first Y and the X steps are solid tiles
  // this is a logic error if the raycaster were to start inside a solid position, but this should be
  // safe since the raycaster is not intended to start inside of a wall.

  if ((int)from.x == from.x && (int)from.y == from.y) {
    if (vRayUnitStepSize.x == vRayUnitStepSize.y) {
      if (direction.x < 0.0f && direction.y < 0.0f) {
        if (map.IsSolid((unsigned short)vMapCheck.x - 1, (unsigned short)vMapCheck.y) &&
            map.IsSolid((unsigned short)vMapCheck.x, (unsigned short)vMapCheck.y - 1)) {
          cornered = true;
        }
      }
    }
  }

  if (direction.x < 0.0f) {
    vStep.x = -1.0f;
    vRayLength1D.x = (from.x - float(vMapCheck.x)) * vRayUnitStepSize.x;
  } else {
    vStep.x = 1.0f;
    vRayLength1D.x = (float(vMapCheck.x + 1) - from.x) * vRayUnitStepSize.x;
  }

  if (direction.y < 0.0f) {
    vStep.y = -1.0f;
    vRayLength1D.y = (from.y - float(vMapCheck.y)) * vRayUnitStepSize.y;
  } else {
    vStep.y = 1.0f;
    vRayLength1D.y = (float(vMapCheck.y + 1) - from.y) * vRayUnitStepSize.y;
  }

  // Perform "Walk" until collision or range check
  while (!bTileFound && fDistance < max_length) {
    // Walk along shortest path
    if (vRayLength1D.x < vRayLength1D.y) {
      vMapCheck.x += vStep.x;
      fDistance = vRayLength1D.x;
      vRayLength1D.x += vRayUnitStepSize.x;
      reflection = Vector2f(-1, 1);
    } else {
      vMapCheck.y += vStep.y;
      fDistance = vRayLength1D.y;
      vRayLength1D.y += vRayUnitStepSize.y;
      reflection = Vector2f(1, -1);
    }

    // Test tile at new test point
    if (IsValidPosition(vMapCheck)) {
      if (perform_collision && map.IsSolid((unsigned short)vMapCheck.x, (unsigned short)vMapCheck.y)) {
        bool skipFirstCheck = cornered && fDistance == 0.0f;

        if (!skipFirstCheck) {
          bTileFound = true;
        }

      } else {
        Vector2f side = Perpendicular(direction);

           SetValue((uint16_t)vMapCheck.x, (uint16_t)vMapCheck.y, value * (max_length - fDistance) / max_length);

           u16 influence_width = 0;

            switch (width) {
             case RayWidth::Three: {
               influence_width = 2;
             } break;
             case RayWidth::Five: {
               influence_width = 3;
             } break;
             case RayWidth::Seven: {
               influence_width = 4;
             } break;
             default: {
               influence_width = 0;
             } break;
            }

        for (u16 i = 1; i < influence_width; i++) {

          Vector2f side1 = vMapCheck + side * i;
          Vector2f side2 = vMapCheck - side * i;

          if (!map.IsSolid(side1)) {
            SetValue((uint16_t)side1.x, (uint16_t)side1.y, value * (max_length - fDistance) / max_length);
          }

          if (!map.IsSolid(side2)) {
            SetValue((uint16_t)side2.x, (uint16_t)side2.y, value * (max_length - fDistance) / max_length);
          }
        
        }        
      }
    } else {
      return result;
    }
  }
  if (bTileFound) {

    result.hit = true;
    result.distance = fDistance;
    result.position = from + direction * fDistance;

    if ((int)result.position.x == result.position.x && (int)result.position.y == result.position.y) {
      if (direction.x > 0 && direction.y > 0) {
        if (map.IsSolid((unsigned short)result.position.x - 1, (unsigned short)result.position.y) &&
            map.IsSolid((unsigned short)result.position.x, (unsigned short)result.position.y - 1)) {
          reflection = Vector2f(-1, -1);
        }
      }

      if (direction.x < 0 && direction.y > 0) {
        if (map.IsSolid((unsigned short)result.position.x - 1, (unsigned short)result.position.y - 1) &&
            map.IsSolid((unsigned short)result.position.x, (unsigned short)result.position.y)) {
          reflection = Vector2f(-1, -1);
        }
      }

      if (direction.x > 0 && direction.y < 0) {
        if (map.IsSolid((unsigned short)result.position.x - 1, (unsigned short)result.position.y - 1) &&
            map.IsSolid((unsigned short)result.position.x, (unsigned short)result.position.y)) {
          reflection = Vector2f(-1, -1);
        }
      }

      if (direction.x < 0 && direction.y < 0) {
        if (map.IsSolid((unsigned short)result.position.x - 1, (unsigned short)result.position.y) &&
            map.IsSolid((unsigned short)result.position.x, (unsigned short)result.position.y - 1)) {
          reflection = Vector2f(-1, -1);
        }
      }
    }
    result.normal = reflection;
  }
  #if DEBUG_RENDER_INFLUENCE_TEXT
  g_RenderState.RenderDebugText("  LOOP: %f", bounces_);
  g_RenderState.RenderDebugText("  directionX: %f", direction.x);
  g_RenderState.RenderDebugText("  directionY: %f", direction.y);
  g_RenderState.RenderDebugText("  fromX: %f", from.x);
  g_RenderState.RenderDebugText("  fromY: %f", from.y);
  g_RenderState.RenderDebugText("  vMapCheckX: %f", vMapCheck.x);
  g_RenderState.RenderDebugText("  vMapCheckY: %f", vMapCheck.y);
  g_RenderState.RenderDebugText("  xStepSize: %f", vRayUnitStepSize.x);
  g_RenderState.RenderDebugText("  yStepSize: %f", vRayUnitStepSize.y);
  g_RenderState.RenderDebugText("  vRayLength1D_X: %f", vRayLength1D.x);
  g_RenderState.RenderDebugText("  vRayLength1D_Y: %f", vRayLength1D.y);
  g_RenderState.RenderDebugText("  vStepX: %f", vStep.x);
  g_RenderState.RenderDebugText("  vStepY: %f", vStep.y);
  g_RenderState.RenderDebugText("  reflectionX: %f", reflection.x);
  g_RenderState.RenderDebugText("  reflectionY: %f", reflection.y);
  g_RenderState.RenderDebugText("  fDistance: %f", fDistance);
  g_RenderState.RenderDebugText("  vIntersectionX: %f", result.position.x);
  g_RenderState.RenderDebugText("  vIntersectionY: %f", result.position.y);
  #endif

  return result;
}

CastResult InfluenceMap::SpreadInfluence(const Map& map, Vector2f from, Vector2f direction, float max_length, float value) {
  CastResult result;

  Vector2f vRayUnitStepSize = Vector2f(abs(1.0f / direction.x), abs(1.0f / direction.y));

  Vector2f vMapCheck = Vector2f(std::floor(from.x), std::floor(from.y));
  Vector2f vRayLength1D, vStep, reflection;

  float fDistance = 0.0f;

  bool cornered = false, bTileFound = false;

  if ((int)from.x == from.x && (int)from.y == from.y) {
    if (vRayUnitStepSize.x == vRayUnitStepSize.y) {
      if (direction.x < 0.0f && direction.y < 0.0f) {
        if (map.IsSolid((unsigned short)vMapCheck.x - 1, (unsigned short)vMapCheck.y) &&
            map.IsSolid((unsigned short)vMapCheck.x, (unsigned short)vMapCheck.y - 1)) {
          cornered = true;
        }
      }
    }
  }

  if (direction.x < 0.0f) {
    vStep.x = -1.0f;
    vRayLength1D.x = (from.x - float(vMapCheck.x)) * vRayUnitStepSize.x;
  } else {
    vStep.x = 1.0f;
    vRayLength1D.x = (float(vMapCheck.x + 1) - from.x) * vRayUnitStepSize.x;
  }

  if (direction.y < 0.0f) {
    vStep.y = -1.0f;
    vRayLength1D.y = (from.y - float(vMapCheck.y)) * vRayUnitStepSize.y;
  } else {
    vStep.y = 1.0f;
    vRayLength1D.y = (float(vMapCheck.y + 1) - from.y) * vRayUnitStepSize.y;
  }

  while (!bTileFound && fDistance < max_length) {
    if (vRayLength1D.x < vRayLength1D.y) {
      vMapCheck.x += vStep.x;
      fDistance = vRayLength1D.x;
      vRayLength1D.x += vRayUnitStepSize.x;
      reflection = Vector2f(-1, 1);
    } else {
      vMapCheck.y += vStep.y;
      fDistance = vRayLength1D.y;
      vRayLength1D.y += vRayUnitStepSize.y;
      reflection = Vector2f(1, -1);
    }

    if (IsValidPosition(vMapCheck)) {
      if (map.IsSolid((unsigned short)vMapCheck.x, (unsigned short)vMapCheck.y)) {
        bool skipFirstCheck = cornered && fDistance == 0.0f;

        if (!skipFirstCheck) {
          bTileFound = true;
        }

      } else {
        SetValue((uint16_t)vMapCheck.x, (uint16_t)vMapCheck.y, value * (max_length - fDistance) / max_length);

        Vector2f side = Perpendicular(direction);
        CastInfluence(map, vMapCheck, side, fDistance, RayWidth::One, value * (max_length - fDistance) / max_length, true);
        CastInfluence(map, vMapCheck, -side, fDistance, RayWidth::One, value * (max_length - fDistance) / max_length, true);       
      }
    }
  }

  if (bTileFound) {
    result.hit = true;
    result.distance = fDistance;
    result.position = from + direction * fDistance;

    if ((int)result.position.x == result.position.x && (int)result.position.y == result.position.y) {
      if (direction.x > 0 && direction.y > 0) {
        if (map.IsSolid((unsigned short)result.position.x - 1, (unsigned short)result.position.y) &&
            map.IsSolid((unsigned short)result.position.x, (unsigned short)result.position.y - 1)) {
          reflection = Vector2f(-1, -1);
        }
      }

      if (direction.x < 0 && direction.y > 0) {
        if (map.IsSolid((unsigned short)result.position.x - 1, (unsigned short)result.position.y - 1) &&
            map.IsSolid((unsigned short)result.position.x, (unsigned short)result.position.y)) {
          reflection = Vector2f(-1, -1);
        }
      }

      if (direction.x > 0 && direction.y < 0) {
        if (map.IsSolid((unsigned short)result.position.x - 1, (unsigned short)result.position.y - 1) &&
            map.IsSolid((unsigned short)result.position.x, (unsigned short)result.position.y)) {
          reflection = Vector2f(-1, -1);
        }
      }

      if (direction.x < 0 && direction.y < 0) {
        if (map.IsSolid((unsigned short)result.position.x - 1, (unsigned short)result.position.y) &&
            map.IsSolid((unsigned short)result.position.x, (unsigned short)result.position.y - 1)) {
          reflection = Vector2f(-1, -1);
        }
      }
    }
    result.normal = reflection;
  }

  return result;
}


void InfluenceMap::CastWeaponOld(const Map& map, Vector2f from, Vector2f direction, float max_length, float value,
                              Weapon* weapon) {
  WeaponType type = weapon->GetData().type;
  int bounces_remaining = 100000;
  bool perform_collision = type != WeaponType::Thor;

  if (type == WeaponType::Bomb || type == WeaponType::ProximityBomb) {
    bounces_remaining = weapon->GetRemainingBounces();
  }

  if (type == WeaponType::Bullet) {
    bounces_remaining = 0;
  }

  for (float i = 0; i < max_length && bounces_remaining >= 0; ++i) {
    bool bounce = false;

    from.x += direction.x;

    if (perform_collision && map.IsSolid(from)) {
      bounce = true;
      from.x -= direction.x;
      direction.x *= -1.0f;
    }

    from.y += direction.y;

    if (perform_collision && map.IsSolid(from)) {
      bounce = true;
      from.y -= direction.y;
      direction.y *= -1.0f;
    }

    SetValue((uint16_t)from.x, (uint16_t)from.y, value * (1.0f - (i / max_length)));

    Vector2f side = Perpendicular(direction);
    Vector2f side1 = from + side;
    Vector2f side2 = from - side;

    if (!map.IsSolid(side1)) {
      SetValue((uint16_t)side1.x, (uint16_t)side1.y, value * (1.0f - (i / max_length)));
    }

    if (!map.IsSolid(side2)) {
      SetValue((uint16_t)side2.x, (uint16_t)side2.y, value * (1.0f - (i / max_length)));
    }

    if (bounce) --bounces_remaining;
  }
}

void InfluenceMap::CastPlayerOld(const Map& map, Vector2f from, Vector2f direction, float max_length, float value) {

  for (float i = 0; i < max_length; ++i) {

    from.x += direction.x;

    if (map.IsSolid(from)) {
      from.x -= direction.x;
      direction.x *= -1.0f;
    }

    from.y += direction.y;

    if (map.IsSolid(from)) {
      from.y -= direction.y;
      direction.y *= -1.0f;
    }

    SetValue((uint16_t)from.x, (uint16_t)from.y, value * (1.0f - (i / max_length)));

    Vector2f side = Perpendicular(direction);
    Vector2f side1 = from + side;
    Vector2f side2 = from - side;

    if (!map.IsSolid(side1)) {
      SetValue((uint16_t)side1.x, (uint16_t)side1.y, value * (1.0f - (i / max_length)));
    }

    if (!map.IsSolid(side2)) {
      SetValue((uint16_t)side2.x, (uint16_t)side2.y, value * (1.0f - (i / max_length)));
    }
  }
}
}  // namespace marvin
