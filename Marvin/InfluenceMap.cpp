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

void InfluenceMap::DebugUpdate(GameProxy& game) {
  const float kInfluenceValue = 1.0f;
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
}


void InfluenceMap::Update(GameProxy& game) {
  


  for (Weapon* weapon : game.GetWeapons()) {
    const Player* player = game.GetPlayerById(weapon->GetPlayerId());

    if (player == nullptr || player->frequency == game.GetPlayer().frequency) {
      continue;
    }

    if (weapon->GetData().type == WeaponType::Decoy) continue;
    if (weapon->GetData().type == WeaponType::Repel) continue;
    if (weapon->GetData().type == WeaponType::None) continue;
    
    Vector2f velocity = weapon->GetVelocity();
    Vector2f direction = Normalize(velocity);
    float influence_length = (velocity * (weapon->GetRemainingTicks() / 100.0f)).Length();

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
    float value = damage / (game.GetEnergy() + 2.0f);
    //float value = damage / (max_energy);
    if (value > 2.0f) {
      value = 2.0f;
    }

    if (game.GetMap().IsSolid(weapon->GetPosition())) continue;

    CastWeapon(game.GetMap(), weapon->GetPosition(), direction, influence_length, value, weapon);
  }

  for (std::size_t i = 0; i < game.GetEnemys().size(); i++) {
    const Player& player = game.GetEnemys()[i];
  //const Player& player = game.GetPlayer();

    if (!player.active || !IsValidPosition(player.position)) {
      continue;
    }
    float max_energy = game.GetMaxEnergy();
    float value = player.energy / (game.GetEnergy() + 2.0f);
    //float value = player.energy / (max_energy);
    if (value > 2.0f) {
      value = 2.0f;
    }

    Vector2f velocity = player.velocity;
    Vector2f direction = Normalize(velocity);
   // g_RenderState.RenderDebugText("  PlayerPositionX: %f", player.position.x);
   // g_RenderState.RenderDebugText("  PlayerPositionY: %f", player.position.y);
  //  g_RenderState.RenderDebugText("  PlayerNormVelocityX: %f", direction.x);
  //  g_RenderState.RenderDebugText("  PlayerNormVelocityY: %f", direction.y);
    float rotation_offset = Vector2f(Normalize(velocity) + player.GetHeading()).Length();
    float speed = velocity.Length();
    float influence_length = speed * rotation_offset + 10.0f;

    if (game.GetMap().IsSolid(player.position)) continue;
    

    CastPlayer(game.GetMap(), player.position, direction, speed * 3.0f, value);
    direction = player.GetHeading();
    //direction = Normalize(player.velocity);
   Vector2f direction_left = Rotate(direction, 0.125f);
    Vector2f direction_left_quarter = Rotate(direction, 0.25f);
    Vector2f direction_right = Rotate(direction, -0.125f);
    Vector2f direction_right_quarter = Rotate(direction, -0.25f);

    CastPlayer(game.GetMap(), player.position, direction, influence_length, value);
    CastPlayer(game.GetMap(), player.position, direction_right, influence_length, value);
    CastPlayer(game.GetMap(), player.position, direction_left, influence_length, value); 
    CastPlayer(game.GetMap(), player.position, direction_right_quarter, influence_length, value);
    CastPlayer(game.GetMap(), player.position, direction_left_quarter, influence_length, value); 
  }

#if DEBUG_RENDER_INFLUENCE
  DebugUpdate(game);
#endif
}


void InfluenceMap::CastWeapon(const Map& map, Vector2f from, Vector2f direction, float max_length, float value,
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

void InfluenceMap::CastPlayer(const Map& map, Vector2f from, Vector2f direction, float max_length, float value) {

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
