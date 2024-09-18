#pragma once

#include <fuse/Math.h>

namespace fuse {

enum class WeaponType : short { None, Bullet, BouncingBullet, Bomb, ProximityBomb, Repel, Decoy, Burst, Thor };

struct WeaponData {
  WeaponType type : 5;
  u16 level : 2;
  u16 shrapbouncing : 1;
  u16 shraplevel : 2;
  u16 shrap : 5;
  u16 alternate : 1;
};
static_assert(sizeof(WeaponData) == 2, "WeaponData must be two bytes exactly.");

struct FUSE_EXPORT Weapon {
  u32 _vtable;

  u32 x;  // 0x04
  u32 y;  // 0x08

  u32 _unuseda;
  i32 velocity_x;
  i32 velocity_y;
  u32 _unused2[32];

  u32 pid;  // 0x98

  char _unused3[11];

  u16 type;

  u16 GetPlayerId() const {
    return pid;
  }

  WeaponData GetType() const {
    WeaponData result;
    memcpy(&result, &type, sizeof(type));
    return result;
  }

  Vector2f GetPosition() const {
    return Vector2f(x / 1000.0f / 16.0f, y / 1000.0f / 16.0f);
  }

  Vector2f GetVelocity() const {
    return Vector2f(velocity_x / 1000.0f / 16.0f, velocity_y / 1000.0f / 16.0f);
  }
};

}  // namespace fuse
