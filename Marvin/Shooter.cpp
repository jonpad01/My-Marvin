#include "Shooter.h"

#include "Bot.h"
#include "Debug.h"
#include "RayCaster.h"

namespace marvin {

    void Shooter::DebugUpdate(Bot& bot) {
      BouncingBombShot(bot, Vector2f(0, 0), Vector2f(0, 0), 0.0f);
      BouncingBulletShot(bot, Vector2f(0, 0), Vector2f(0, 0), 0.0f);
    }

ShotResult Shooter::CalculateShot(Vector2f pShooter, Vector2f pTarget, Vector2f vShooter, Vector2f vTarget, float sProjectile) {

   ShotResult result;
   result.hit = true;
  // directional vector poiting to target from shooter
  Vector2f totarget = pTarget - pShooter;
  Vector2f v = vTarget - vShooter;

  // dot product
  float a = v.Dot(v) - sProjectile * sProjectile;
  float b = 2 * v.Dot(totarget);
  float c = totarget.Dot(totarget);

  // quadratic formula
  float disc = (b * b) - 4.0f * a * c;
  float t = 0.0f;

  // if disc or t are below zero it means the target is moving away at a speed faster than the bullet
  // the shot is impossible, t will equal 0 and the solution will be the targets position, but the hit will return false
  // this is called the square root discriminant
  if (disc >= 0.0f && a != 0.0f && c != 0.0f) {
    float t1 = (-b - std::sqrt(disc)) / (2.0f * a);
    float t2 = (-b + std::sqrt(disc)) / (2.0f * a);

    if (t1 >= 0.0f && t2 >= 0.0f) {
      t = std::min(t1, t2);
    } else {
      t = std::max(t1, t2);
    }
    // this shouldnt happen, but im pretty sure it does sometimes
    if (t < 0.0f) {
      t = 0.0f;
      result.hit = false;
    }

  } else {
    result.hit = false;
  }

  // the solution is basically the directional vector from the shooter to this position
  Vector2f dSolution = pTarget + (v * t);
  // this is the intersected position
  Vector2f pSolution = pTarget + (vTarget * t);
  // return a position with the direcrtional solution and the magnitude solution
  Vector2f solution = pShooter + Normalize(dSolution - pShooter) * pShooter.Distance(pSolution);
  // check if shot is within map boundries, i think this happens with bouncing shots
  if (!IsValidPosition(solution)) {
    solution = pTarget;
    result.hit = false;
  }

  // this solution is used for bouncing shots, determining hover distance, and bullet travel time to target
  
  result.solution = solution;
  return result;
}

ShotResult Shooter::BouncingBombShot(Bot& bot, Vector2f target_pos, Vector2f target_vel, float target_radius) {
  auto& game = bot.GetGame();

  float proj_speed = game.GetShipSettings().GetBombSpeed();
  float alive_time = game.GetSettings().GetBombAliveTime();
  float bounces = (float)game.GetShipSettings().BombBounceCount;

  ShotResult result = BounceShot(bot, target_pos, target_vel, 2.0f, game.GetPosition(), game.GetPlayer().velocity,
                 game.GetPlayer().GetHeading(), proj_speed, alive_time, bounces);

  return result;
}

ShotResult Shooter::BouncingBulletShot(Bot& bot, Vector2f target_pos, Vector2f target_vel, float target_radius) {
  auto& game = bot.GetGame();
  ShotResult result;

  Vector2f direction = game.GetPlayer().GetHeading();
  Vector2f position = game.GetPosition();
  float radius = game.GetShipSettings().GetRadius();
  Vector2f side = Perpendicular(game.GetPlayer().GetHeading());

  float proj_speed = game.GetShipSettings().GetBulletSpeed();
  float alive_time = game.GetSettings().GetBulletAliveTime();

  bool double_barrel = (game.GetShipSettings().DoubleBarrel & 1) != 0;
  bool multi_enabled = game.GetPlayer().multifire_status;

  std::vector<Vector2f> pos_adjust;

  // if ship has double barrel make 2 calculations with offset positions
  if (double_barrel) {
    pos_adjust.push_back(side * radius * 0.8f);
    pos_adjust.push_back(side * -radius * 0.8f);
  } else {
    // if not make one calculation from center
    pos_adjust.push_back(Vector2f(0, 0));
  }
  // if multifire is enabled push back 2 more
  if (multi_enabled) {
    if (double_barrel) {
      pos_adjust.push_back(side * radius * 0.8f);
      pos_adjust.push_back(side * -radius * 0.8f);
    } else {
      pos_adjust.push_back(Vector2f(0, 0));
      pos_adjust.push_back(Vector2f(0, 0));
    }
  }

  for (std::size_t i = 0; i < pos_adjust.size(); i++) {
    position = game.GetPosition() + pos_adjust[i];

    if (game.GetPlayer().multifire_status) {
      if (i == (pos_adjust.size() - 2)) {
        direction = game.GetPlayer().GetHeading(game.GetShipSettings().GetMultiFireAngle());
      } else if (i == (pos_adjust.size() - 3)) {
        direction = game.GetPlayer().GetHeading(-game.GetShipSettings().GetMultiFireAngle());
      }
    }
   ShotResult current_result = BounceShot(bot, target_pos, target_vel, target_radius, position, game.GetPlayer().velocity, direction, proj_speed,
               alive_time, 100.0f);

   if (i == 0 || current_result.hit) {
     result = current_result;
   }
  }
  return result;
}

/*
Bounching shot concept:  Use the calculate shot function in combination with the raycaster to look for a solution that is in line with
the guns projected path. The calculate shot function starts with the player position and calculates a solution to the target, if the 
calculated bullet trajectory is in line with the solution return true.  If not use the raycraster to find and reflect off the wall and
calculate a new solution and check again, until it runs out of bounces or reaches the projectiles maximum travel distance.
*/

ShotResult Shooter::BounceShot(Bot& bot, Vector2f pTarget, Vector2f vTarget, float rTarget, Vector2f pShooter,
                               Vector2f vShooter, Vector2f dShooter, float proj_speed, float alive_time,
                               float bounces) {
  auto& game = bot.GetGame();
  ShotResult result;
  float traveled_dist = 0.0f;
  float total_travel_time = 0.0f;

  Vector2f proj_velocity = dShooter * proj_speed + vShooter;
  float proj_travel = proj_velocity.Length() * alive_time;

  Vector2f sDirection = Normalize(proj_velocity);


  for (float j = 0.0f; j <= bounces; ++j) {
    /*
     Concept: When the shot is bounced off the wall, the calculate shot function will become inaccurate because
      it is not able to calculate the time it took for the bullet to travel to the next starting point.  This
     method attempts to calculate a new speed that compensates for the time it took to reach the starting point,
     this means the calculated speed will always get slower as it bounces off of more walls.
      */

    // get the total distance the projectile could travel and calculate the total time to reach the target
    float total_travel_time = (traveled_dist + pTarget.Distance(pShooter)) / proj_speed;
    // now chop out the distance it has already traveled and calcutate a speed as if it takes this long
    // for the bullet to reach the target
    float reduced_proj_speed = pTarget.Distance(pShooter) / total_travel_time;

    // the shot solution, calculated at the wall, travel time is included by reducing the bullet speed
    ShotResult cResult = CalculateShot(pShooter, pTarget, vShooter, vTarget, reduced_proj_speed);
    result.solution = cResult.solution;

    if (cResult.hit) {
      // check if the solution is in line of sight of the current shooting position
      //Vector2f tosPosition = cResult.solution - pShooter;
      if (!SolidRayCast(bot, pShooter, cResult.solution).hit) {
        // check if the solution is in line of sight of the targets position
        //Vector2f t2shot = cResult.solution - pTarget;
        if (!SolidRayCast(bot, pTarget, cResult.solution).hit) {
          if (FloatingRayBoxIntersect(pShooter, sDirection, cResult.solution, rTarget, nullptr, nullptr)) {
            if (pShooter.Distance(cResult.solution) <= proj_travel) {
#if DEBUG_RENDER_SHOOTER
              RenderWorldLine(game.GetPosition(), pShooter, cResult.solution, RGB(100, 0, 0));              
              RenderWorldBox(game.GetPosition(), cResult.solution - Vector2f(1, 1), cResult.solution + Vector2f(1, 1),
                             RGB(0, 0, 255));
#endif
              result.hit = true;
              result.final_position = cResult.solution;
              return result;
            }
          }
        }
      }
    }
    // if the calculate shot didnt return true, cast a line to the wall and calculate a new direction and position
    CastResult wall_line = RayCast(bot, RayBarrier::Solid, pShooter, sDirection, proj_travel);

    if (wall_line.hit) {
#if DEBUG_RENDER_SHOOTER
      RenderWorldLine(game.GetPosition(), pShooter, wall_line.position, RGB(0, 100, 100));
      if (j == bounces) {
        RenderWorldBox(game.GetPosition(), wall_line.position - Vector2f(1, 1), wall_line.position + Vector2f(1, 1),
                       RGB(0, 0, 255));
      }
#endif

      sDirection = Vector2f(sDirection.x * wall_line.normal.x, sDirection.y * wall_line.normal.y);
      vShooter = Vector2f(vShooter.x * wall_line.normal.x, vShooter.y * wall_line.normal.y);

      pShooter = wall_line.position;

      proj_travel -= wall_line.distance;
      traveled_dist += wall_line.distance;
      result.final_position = wall_line.position;

    } else {
      result.final_position = pShooter + sDirection * proj_travel;
#if DEBUG_RENDER_SHOOTER
      RenderWorldLine(game.GetPosition(), pShooter, pShooter + sDirection * proj_travel, RGB(0, 100, 100));
      RenderWorldBox(game.GetPosition(), result.final_position - Vector2f(1.0f, 1.0f),
                     result.final_position + Vector2f(1.0f, 1.0f), RGB(0, 0, 255));
#endif
      break;
    }
  }

  return result;
}

void Shooter::LookForWallShot(GameProxy& game, Vector2f target_pos, Vector2f target_vel, float proj_speed, int alive_time,
                     uint8_t bounces) {
  for (std::size_t i = 0; i < 1; i++) {
    int left_rotation = game.GetPlayer().discrete_rotation - i;
    int right_rotation = game.GetPlayer().discrete_rotation + i;

    if (left_rotation < 0) {
      left_rotation += 40;
    }
    if (right_rotation > 39) {
      right_rotation -= 40;
    }

    Vector2f left_trajectory =
        (DiscreteToHeading(left_rotation) * proj_speed) + game.GetPlayer().velocity;
    Vector2f right_trajectory =
        (DiscreteToHeading(right_rotation) * proj_speed) + game.GetPlayer().velocity;

    Vector2f left_direction = Normalize(left_trajectory);
    Vector2f right_direction = Normalize(right_trajectory);
  }
}

bool CanShoot(GameProxy& game, Vector2f player_pos, Vector2f target, Vector2f weapon_velocity, float alive_time) {
  float projectile_travel_sq = (weapon_velocity * alive_time).LengthSq();

  if (player_pos.DistanceSq(target) > projectile_travel_sq) return false;
  if (game.GetMap().GetTileId(player_pos) == marvin::kSafeTileId) return false;

  return true;
}

bool CanShootGun(GameProxy& game, const Map& map, Vector2f player, Vector2f target) {

  float bullet_alive_time = game.GetSettings().GetBulletAliveTime();
  float bullet_speed = game.GetShipSettings().GetBulletSpeed();

  Vector2f adjusted_bullet_velocity = game.GetPlayer().GetHeading() * bullet_speed + game.GetPlayer().velocity;
  float bullet_travel = adjusted_bullet_velocity.Length() * bullet_alive_time;

  //float bullet_travel = (bullet_speed + (game.GetPlayer().velocity * game.GetPlayer().GetHeading())) *
                   //     ((float)game.GetSettings().BulletAliveTime / 100.0f);

  if (player.Distance(target) > bullet_travel) return false;
  if (map.GetTileId(player) == marvin::kSafeTileId) return false;

  return true;
}

bool CanShootBomb(GameProxy& game, const Map& map, Vector2f player, Vector2f target) {

  float bomb_alive_time = game.GetSettings().GetBombAliveTime();
  float bomb_speed = game.GetShipSettings().GetBombSpeed();

  Vector2f adjusted_bomb_velocity = game.GetPlayer().GetHeading() * bomb_speed + game.GetPlayer().velocity;
  float bomb_travel = adjusted_bomb_velocity.Length() * bomb_alive_time;

 // float bomb_travel = (bomb_speed + (game.GetPlayer().velocity * game.GetPlayer().GetHeading())) *
                   //   ((float)game.GetSettings().BombAliveTime / 100.0f);

  if (player.Distance(target) > bomb_travel) return false;
  if (map.GetTileId(player) == marvin::kSafeTileId) return false;

  return true;
}

bool IsValidTarget(Bot& bot, const Player& target, CombatRole combat_role) {
  auto& game = bot.GetGame();

  const Player& bot_player = game.GetPlayer();

  // anchors shoud wait until the target is no longer lag attachable
  if (combat_role != CombatRole::Anchor && (target.dead || !IsValidPosition(target.position))) {
    return false;
  }
  else if (!IsValidPosition(target.position)) {
    return false;
  }

  if (target.id == game.GetPlayer().id) return false;
  if (target.ship > 7) return false;
  if (target.frequency == game.GetPlayer().frequency) return false;
  if (target.name[0] == '<') return false;

  if (game.GetMap().GetTileId(target.position) == marvin::kSafeTileId) {
    return false;
  }

  MapCoord bot_coord(bot_player.position);
  MapCoord target_coord(target.position);

  if (!bot.GetRegions().IsConnected(bot_coord, target_coord)) {
    return false;
  }
  // TODO: check if player is cloaking and outside radar range
  // 1 = stealth, 2= cloak, 3 = both, 4 = xradar
  bool stealthing = (target.status & 1) != 0;
  bool cloaking = (target.status & 2) != 0;

  Vector2f resolution(1920, 1080);
  Vector2f view_min_ = game.GetPosition() - resolution / 2.0f / 16.0f;
  Vector2f view_max_ = game.GetPosition() + resolution / 2.0f / 16.0f;

  // if the bot doesnt have xradar
  if (!(bot.GetGame().GetPlayer().status & 4)) {
    if (stealthing && cloaking) return false;

    bool visible = InRect(target.position, view_min_, view_max_);

    if (stealthing && !visible) return false;
  }

  return true;
}

}  // namespace marvin
