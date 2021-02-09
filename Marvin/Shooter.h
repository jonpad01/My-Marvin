#pragma once

#include "GameProxy.h"
#include "behavior/BehaviorEngine.h"
#include "path/Pathfinder.h"


namespace marvin {

	bool CalculateShot(Vector2f pShooter, Vector2f pTarget, Vector2f vShooter, Vector2f vTarget, float sProjectile, Vector2f* Solution);

	bool BounceShot(GameProxy& game, Vector2f target_pos, Vector2f target_vel, float target_radius, Vector2f direction, bool* bomb_hit, Vector2f* wall_pos);

	bool CanShootGun(GameProxy& game, const Map& map, Vector2f player, Vector2f target);
	bool CanShootBomb(GameProxy& game, const Map& map, Vector2f player, Vector2f target);



}	//namespace marvin