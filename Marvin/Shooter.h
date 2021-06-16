#pragma once

#include "GameProxy.h"
#include "behavior/BehaviorEngine.h"
#include "path/Pathfinder.h"

namespace marvin {

class Bot;

bool CalculateShot(Vector2f pShooter, Vector2f pTarget, Vector2f vShooter, Vector2f vTarget, float sProjectile,
                   Vector2f* Solution);

bool BounceShot(GameProxy& game, Vector2f target_pos, Vector2f target_vel, float target_radius, Vector2f direction,
                bool* bomb_hit, Vector2f* wall_pos);

void LookForWallShot(GameProxy& game, Vector2f target_pos, Vector2f target_vel, float proj_speed, int alive_time,
                     uint8_t bounces);

bool CanShoot(GameProxy& game, Vector2f player_pos, Vector2f solution, float proj_speed, float alive_time);

bool CanShootGun(GameProxy& game, const Map& map, Vector2f player, Vector2f target);
bool CanShootBomb(GameProxy& game, const Map& map, Vector2f player, Vector2f target);

bool IsValidTarget(Bot& bot, const Player& target);

}  // namespace marvin
