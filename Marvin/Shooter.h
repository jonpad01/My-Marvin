#pragma once

#include "GameProxy.h"
#include "behavior/BehaviorEngine.h"
#include "path/Pathfinder.h"

namespace marvin {

class Bot;

struct ShotResult {
  ShotResult() : hit(false) {}
  bool hit;
  Vector2f solution;
  Vector2f final_position;
};

class Shooter {
public:
  void DebugUpdate(Bot& bot);

  ShotResult CalculateShot(Vector2f pShooter, Vector2f pTarget, Vector2f vShooter, Vector2f vTarget, float sProjectile);

  ShotResult BouncingBulletShot(Bot& bot, Vector2f target_pos, Vector2f target_vel, float target_radius);

  ShotResult BouncingBombShot(Bot& bot, Vector2f target_pos, Vector2f target_vel, float target_radius);

  ShotResult BounceShot(Bot& bot, Vector2f pTarget, Vector2f vTarget, float rTarget, Vector2f pShooter,
                        Vector2f vShooter, Vector2f dShooter, float proj_speed, float alive_time, float bounces);

  void LookForWallShot(GameProxy& game, Vector2f target_pos, Vector2f target_vel, float proj_speed, int alive_time,
                       uint8_t bounces);
};



bool CanShoot(GameProxy& game, Vector2f player_pos, Vector2f solution, Vector2f weapon_velocity, float alive_time);

bool CanShootGun(GameProxy& game, const Map& map, Vector2f player, Vector2f target);
bool CanShootBomb(GameProxy& game, const Map& map, Vector2f player, Vector2f target);

bool IsValidTarget(Bot& bot, const Player& target, bool anchoring);

}  // namespace marvin
