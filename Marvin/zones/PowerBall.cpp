#include "PowerBall.h"

#include <chrono>
#include <cstring>
#include <limits>

#include "../Debug.h"
#include "../GameProxy.h"
#include "../Map.h"
#include "../RayCaster.h"
#include "../RegionRegistry.h"
#include "../Shooter.h"
#include "../platform/ContinuumGameProxy.h"
#include "../platform/Platform.h"

namespace marvin {
namespace pb {

void PowerBallBehaviorBuilder::CreateBehavior(Bot& bot) {

  bot.FindPowerBallGoal();
  bot.GetBlackboard().Set<std::string>("PowerBallMap", bot.GetGame().GetMapFile());

  auto find_enemy = std::make_unique<pb::FindEnemyNode>();
  auto aim_with_gun = std::make_unique<pb::AimWithGunNode>();
  auto aim_with_bomb = std::make_unique<pb::AimWithBombNode>();
  auto target_in_los = std::make_unique<pb::InLineOfSightNode>();
  auto shoot_gun = std::make_unique<pb::ShootGunNode>();
  auto shoot_bomb = std::make_unique<pb::ShootBombNode>();
  auto path_to_enemy = std::make_unique<pb::PathToEnemyNode>();
  auto move_to_enemy = std::make_unique<pb::MoveToEnemyNode>();
  auto follow_path = std::make_unique<pb::FollowPathNode>();
  auto patrol = std::make_unique<pb::PatrolNode>();

  auto move_method_selector = std::make_unique<behavior::SelectorNode>(move_to_enemy.get());
  auto gun_sequence = std::make_unique<behavior::SequenceNode>(aim_with_gun.get(), shoot_gun.get());
  auto bomb_sequence = std::make_unique<behavior::SequenceNode>(aim_with_bomb.get(), shoot_bomb.get());
  auto bomb_gun_sequence = std::make_unique<behavior::SelectorNode>(gun_sequence.get(), bomb_sequence.get());
  auto parallel_shoot_enemy =
      std::make_unique<behavior::ParallelNode>(bomb_gun_sequence.get(), move_method_selector.get());
  auto los_shoot_conditional =
      std::make_unique<behavior::SequenceNode>(target_in_los.get(), parallel_shoot_enemy.get());
  auto enemy_path_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy.get(), follow_path.get());
  auto patrol_path_sequence = std::make_unique<behavior::SequenceNode>(patrol.get(), follow_path.get());
  auto path_or_shoot_selector =
      std::make_unique<behavior::SelectorNode>(los_shoot_conditional.get(), enemy_path_sequence.get());
  auto handle_enemy = std::make_unique<behavior::SequenceNode>(find_enemy.get(), path_or_shoot_selector.get());

  auto root_selector = std::make_unique<behavior::SelectorNode>(handle_enemy.get(), patrol_path_sequence.get());

  auto freq_warp_attach = std::make_unique<pb::FreqWarpAttachNode>();
  auto ball_selector = std::make_unique<pb::BallSelectorNode>();

  auto root_sequence = std::make_unique<behavior::SequenceNode>(ball_selector.get(), root_selector.get());

  engine_->PushRoot(std::move(root_sequence));

  engine_->PushNode(std::move(find_enemy));
  engine_->PushNode(std::move(aim_with_gun));
  engine_->PushNode(std::move(aim_with_bomb));
  engine_->PushNode(std::move(target_in_los));
  engine_->PushNode(std::move(shoot_gun));
  engine_->PushNode(std::move(shoot_bomb));
  engine_->PushNode(std::move(path_to_enemy));
  engine_->PushNode(std::move(move_to_enemy));
  engine_->PushNode(std::move(follow_path));
  engine_->PushNode(std::move(patrol));

  engine_->PushNode(std::move(move_method_selector));
  engine_->PushNode(std::move(bomb_gun_sequence));
  engine_->PushNode(std::move(bomb_sequence));
  engine_->PushNode(std::move(gun_sequence));
  engine_->PushNode(std::move(parallel_shoot_enemy));
  engine_->PushNode(std::move(los_shoot_conditional));
  engine_->PushNode(std::move(enemy_path_sequence));
  engine_->PushNode(std::move(patrol_path_sequence));
  engine_->PushNode(std::move(path_or_shoot_selector));
  engine_->PushNode(std::move(handle_enemy));

  engine_->PushNode(std::move(root_selector));

  engine_->PushNode(std::move(freq_warp_attach));
  engine_->PushNode(std::move(ball_selector));

  engine_->PushNode(std::move(root_sequence));
}


behavior::ExecuteResult BallSelectorNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();

  Vector2f position = game.GetPosition();
  float closest_distance = std::numeric_limits<float>::max();
  const BallData* target_ball = nullptr;

  for (std::size_t i = 0; i < game.GetBalls().size(); i++) {
    const BallData& ball = game.GetBalls()[i];

    if (!ctx.bot->GetRegions().IsConnected(position, ball.position)) {
      continue;
    }

    float distance = position.Distance(ball.position);

    if (distance < closest_distance) {
      closest_distance = distance;
      target_ball = &game.GetBalls()[i];
    }
  }

  ctx.blackboard.Set<const BallData*>("TargetBall", target_ball);

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult FreqWarpAttachNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  uint64_t time = ctx.bot->GetTime().GetTime();

  // probably 775 (7.75 seconds)
  uint16_t hold_time = game.GetSettings().ShipSettings[game.GetPlayer().ship].SoccerBallThrowTimer * 10;

  uint64_t ball_cooldown = ctx.blackboard.ValueOr<uint64_t>("BallCoolDown", 0);
  bool set_timer = ctx.blackboard.ValueOr<bool>("SetTimer", true);

  if (time >= ball_cooldown) {
    set_timer = true;
    ctx.blackboard.Set("SetTimer", true);
  }

  int64_t timer = hold_time;

  const BallData* ball = ctx.blackboard.ValueOr<const BallData*>("TargetBall", nullptr);

  // make this node pass even if the target is a nullptr
  if (ball && ball->held && ball->last_holder_id == game.GetPlayer().id) {
    if (set_timer) {
      ctx.blackboard.Set("BallCoolDown", time + hold_time);
      ctx.blackboard.Set("SetTimer", false);
    } else {
      timer = ball_cooldown - time;
    }
  }

  ctx.blackboard.Set("BallTimer", timer);
  // RenderText(std::to_string(timer), GetWindowCenter() - Vector2f(100, 100), RGB(100, 100, 100), RenderText_Centered);

  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult FindEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  behavior::ExecuteResult result = behavior::ExecuteResult::Failure;
  float closest_cost = std::numeric_limits<float>::max();
  float cost = 0;
  auto& game = ctx.bot->GetGame();
  const Player* target = nullptr;
  const Player& bot = ctx.bot->GetGame().GetPlayer();

  const BallData* ball = ctx.blackboard.ValueOr<const BallData*>("TargetBall", nullptr);

  if (ball) {
    // make this node pass even if the target is a nullptr
    if (ball->held && ball->last_holder_id == game.GetPlayer().id) {
      result = behavior::ExecuteResult::Success;
    }
    // if able to grab the ball patrol to ball
    if (!ball->held) {
      if (ball->inactive_timer > 1200 || ball->last_holder_id != game.GetPlayer().id) {
        return behavior::ExecuteResult::Failure;
      }
    }
  }

  Vector2f position = game.GetPosition();

  // this loop checks every player and finds the closest one based on a cost formula
  // if this does not succeed then there is no target, it will go into patrol mode
  for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
    const Player& player = game.GetPlayers()[i];

    if (!IsValidTarget(ctx, player)) continue;

    if (ball && ball->held && ball->last_holder_id == player.id) {
      target = &game.GetPlayers()[i];
      ctx.blackboard.Set("target_player", target);
      return behavior::ExecuteResult::Success;
    } else {
      auto to_target = player.position - position;
      CastResult in_sight = RayCast(game.GetMap(), game.GetPosition(), Normalize(to_target), to_target.Length());

      // make players in line of sight high priority

      if (!in_sight.hit) {
        cost = CalculateCost(game, bot, player) / 100;
      } else {
        cost = CalculateCost(game, bot, player);
      }
      if (cost < closest_cost) {
        closest_cost = cost;
        target = &game.GetPlayers()[i];
        result = behavior::ExecuteResult::Success;
      }
    }
  }
  // on the first loop, there is nothing in current_target
  const Player* current_target = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
  // const marvin::Player& check_current_target = *current_target;

  // on the following loops, this compares what it found to the current_target
  // and checks if the current_target is still valid
  if (current_target && IsValidTarget(ctx, *current_target)) {
    // Calculate the cost to the current target so there's some stickiness
    // between close targets.
    const float kStickiness = 2.5f;
    float cost = CalculateCost(game, bot, *current_target);

    if (cost * kStickiness < closest_cost) {
      target = current_target;
    }
  }

  // and this sets the current_target for the next loop
  ctx.blackboard.Set("target_player", target);

  return result;
}

float FindEnemyNode::CalculateCost(GameProxy& game, const Player& bot_player, const Player& target) {
  float dist = bot_player.position.Distance(target.position);
  // How many seconds it takes to rotate 180 degrees
  float rotate_speed = game.GetShipSettings().InitialRotation / 200.0f;            // MaximumRotation
  float move_cost = dist / (game.GetShipSettings().InitialSpeed / 10.0f / 16.0f);  // MaximumSpeed
  Vector2f direction = Normalize(target.position - bot_player.position);
  float dot = std::abs(bot_player.GetHeading().Dot(direction) - 1.0f) / 2.0f;
  float rotate_cost = std::abs(dot) * rotate_speed;

  return move_cost + rotate_cost;
}

bool FindEnemyNode::IsValidTarget(behavior::ExecuteContext& ctx, const Player& target) {
  const auto& game = ctx.bot->GetGame();
  const Player& bot_player = game.GetPlayer();
  const BallData* ball = ctx.blackboard.ValueOr<const BallData*>("TargetBall", nullptr);

  if (!target.active) return false;
  if (target.id == game.GetPlayer().id) return false;
  if (target.ship > 7) return false;

  if (ball && ball->held && ball->last_holder_id == game.GetPlayer().id) {
    if (target.frequency != game.GetPlayer().frequency) return false;
  } else {
    if (target.frequency == game.GetPlayer().frequency) return false;
  }

  if (target.name[0] == '<') return false;

  if (game.GetMap().GetTileId(target.position) == marvin::kSafeTileId) {
    return false;
  }

  if (!IsValidPosition(target.position)) {
    return false;
  }

  MapCoord bot_coord(bot_player.position);
  MapCoord target_coord(target.position);

  if (!ctx.bot->GetRegions().IsConnected(bot_coord, target_coord)) {
    return false;
  }
  return true;
}

behavior::ExecuteResult PathToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  std::vector<Vector2f> path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());
  Vector2f bot = game.GetPosition();

  const BallData* ball = ctx.blackboard.ValueOr<const BallData*>("TargetBall", nullptr);

  if (ball && ball->held && ball->last_holder_id == game.GetPlayer().id) {
      // FIX THIS
    //path = ctx.bot->GetPathfinder().CreatePath(path, bot, ctx.bot->GetPowerBallGoalPath(),
    //                                          game.GetShipSettings().GetRadius());

    ctx.blackboard.Set("path", path);
    return behavior::ExecuteResult::Success;
  }

  const Player* enemy = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

  if (enemy == nullptr) {
    return behavior::ExecuteResult::Failure;
  }

  path = ctx.bot->GetPathfinder().CreatePath(path, bot, enemy->position, game.GetShipSettings().GetRadius());

  ctx.blackboard.Set("path", path);
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult PatrolNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  Vector2f from = game.GetPosition();
  std::vector<Vector2f> path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());

  const BallData* ball = ctx.blackboard.ValueOr<const BallData*>("TargetBall", nullptr);

  if (!ball) {
    return behavior::ExecuteResult::Failure;
  }

  path = ctx.bot->GetPathfinder().CreatePath(path, from, ball->position, game.GetShipSettings().GetRadius());

  ctx.blackboard.Set("path", path);

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult FollowPathNode::Execute(behavior::ExecuteContext& ctx) {
  auto path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());

  size_t path_size = path.size();
  auto& game = ctx.bot->GetGame();

  if (path.empty()) return behavior::ExecuteResult::Failure;

  Vector2f current = path.front();
  Vector2f from = game.GetPosition();

  // Fix an issue where the pathfinder generates nodes in the center of the tile
  // but the ship can't occupy the exact center. This is only an issue without path smoothing.
  if (!path.empty() && (u16)from.x == (u16)path.at(0).x && (u16)from.y == (u16)path.at(0).y) {
    path.erase(path.begin());

    if (!path.empty()) {
      current = path.front();
    }
  }

  while (path.size() > 1 && CanMoveBetween(game, game.GetPosition(), path.at(1))) {
    path.erase(path.begin());
    current = path.front();
  }

  if (path.size() == 1 && path.front().DistanceSq(game.GetPosition()) < 2.0f * 2.0f) {
    path.clear();
  }

  if (path.size() != path_size) {
    ctx.blackboard.Set("path", path);
  }

  ctx.bot->Move(current, 0.0f);

  return behavior::ExecuteResult::Success;
}

bool FollowPathNode::CanMoveBetween(GameProxy& game, Vector2f from, Vector2f to) {
  Vector2f trajectory = to - from;
  Vector2f direction = Normalize(trajectory);
  Vector2f side = Perpendicular(direction);

  float distance = from.Distance(to);
  float radius = game.GetShipSettings().GetRadius();

  CastResult center = RayCast(game.GetMap(), from, direction, distance);
  CastResult side1 = RayCast(game.GetMap(), from + side * radius, direction, distance);
  CastResult side2 = RayCast(game.GetMap(), from - side * radius, direction, distance);

  return !center.hit && !side1.hit && !side2.hit;
}

behavior::ExecuteResult InLineOfSightNode::Execute(behavior::ExecuteContext& ctx) {
  behavior::ExecuteResult result = behavior::ExecuteResult::Failure;
  const auto target = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

  auto& game = ctx.bot->GetGame();

  float ball_speed = game.GetSettings().ShipSettings[game.GetPlayer().ship].SoccerBallSpeed / 10.0f / 16.0f;

  //  const BallData* ball = ctx.blackboard.ValueOr<const BallData*>("TargetBall", nullptr);

  int64_t ball_timer = ctx.blackboard.ValueOr<int64_t>("BallTimer", 7750);

  //  if (ctx.com->ShootWall(ctx.pb->GetPowerBallGoal(), Vector2f(0, 0), ball_speed, 400, 10) || ball_time < 200) {
  //     return behavior::ExecuteResult::Success;
  //  }

  if (!target) return behavior::ExecuteResult::Failure;

  auto to_target = target->position - game.GetPosition();
  CastResult ray_cast = RayCast(game.GetMap(), game.GetPosition(), Normalize(to_target), to_target.Length());

  if (!ray_cast.hit) {
    result = behavior::ExecuteResult::Success;
  }
  return result;
}

behavior::ExecuteResult AimWithGunNode::Execute(behavior::ExecuteContext& ctx) {
  int64_t ball_timer = ctx.blackboard.ValueOr<int64_t>("BallTimer", 7750);

  // if (ctx.pb->HasWallShot() || ball_timer < 200) {
  //     return behavior::ExecuteResult::Success;
  //  }

  const auto target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

  if (target_player == nullptr) return behavior::ExecuteResult::Failure;

  const Player& target = *target_player;
  auto& game = ctx.bot->GetGame();
  const Player& bot_player = game.GetPlayer();

  if (!CanShootGun(game, game.GetMap(), game.GetPosition(), target.position)) {
    return behavior::ExecuteResult::Failure;
  }

  float proj_speed = game.GetSettings().ShipSettings[bot_player.ship].BulletSpeed / 10.0f / 16.0f;

  Vector2f target_pos = target.position;

  //  Vector2f seek_position = CalculateShot(game.GetPosition(), target_pos, bot_player.velocity, target.velocity,
  //  proj_speed);

  Vector2f projectile_trajectory = (bot_player.GetHeading() * proj_speed) + bot_player.velocity;

  Vector2f projectile_direction = Normalize(projectile_trajectory);

  float target_radius = game.GetSettings().ShipSettings[target.ship].GetRadius();

  float aggression = ctx.blackboard.ValueOr("aggression", 0.0f);
  float radius_multiplier = 1.0f;

  // if the target is cloaking and bot doesnt have xradar make the bot shoot wide
  if (!(game.GetPlayer().status & 4)) {
    if (target.status & 2) {
      radius_multiplier = 3.0f;
    }
  }

  float nearby_radius = target_radius * radius_multiplier;

  Vector2f box_min = target.position - Vector2f(nearby_radius, nearby_radius);
  Vector2f box_extent(nearby_radius * 1.2f, nearby_radius * 1.2f);
  float dist;
  Vector2f norm;

  bool hit = RayBoxIntersect(bot_player.position, projectile_direction, box_min, box_extent, &dist, &norm);

  if (!hit) {
    //       box_min = seek_position - Vector2f(nearby_radius, nearby_radius);
    hit = RayBoxIntersect(bot_player.position, bot_player.GetHeading(), box_min, box_extent, &dist, &norm);
  }

  //    if (seek_position.DistanceSq(target_player->position) < 15 * 15) {
  //         ctx.blackboard.Set("target_position", seek_position);
  //     }
  //      else {
  //         ctx.blackboard.Set("target_position", target.position);
  //      }

  if (hit) {
    return behavior::ExecuteResult::Success;
  }

  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult AimWithBombNode::Execute(behavior::ExecuteContext& ctx) {
  const auto target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

  if (target_player == nullptr) return behavior::ExecuteResult::Failure;

  const Player& target = *target_player;
  auto& game = ctx.bot->GetGame();
  const Player& bot_player = game.GetPlayer();

  if (!CanShootBomb(game, game.GetMap(), game.GetPosition(), target.position)) {
    return behavior::ExecuteResult::Failure;
  }

  float proj_speed = game.GetSettings().ShipSettings[bot_player.ship].BombSpeed / 10.0f / 16.0f;

  Vector2f target_pos = target.position;

  //   Vector2f seek_position = CalculateShot(game.GetPosition(), target_pos, bot_player.velocity, target.velocity,
  //   proj_speed);

  Vector2f projectile_trajectory = (bot_player.GetHeading() * proj_speed) + bot_player.velocity;

  Vector2f projectile_direction = Normalize(projectile_trajectory);

  float target_radius = game.GetSettings().ShipSettings[target.ship].GetRadius();

  float radius_multiplier = 0.80f;

  // if the target is cloaking and bot doesnt have xradar make the bot shoot wide
  if (!(game.GetPlayer().status & 4)) {
    if (target.status & 2) {
      radius_multiplier = 3.0f;
    }
  }

  float nearby_radius = target_radius * radius_multiplier;

  Vector2f box_min = target.position - Vector2f(nearby_radius, nearby_radius);
  Vector2f box_extent(nearby_radius, nearby_radius);
  float dist;
  Vector2f norm;

  bool hit = RayBoxIntersect(bot_player.position, projectile_direction, box_min, box_extent, &dist, &norm);

  if (!hit) {
    //       box_min = seek_position - Vector2f(nearby_radius, nearby_radius);
    hit = RayBoxIntersect(bot_player.position, bot_player.GetHeading(), box_min, box_extent, &dist, &norm);
  }

  //     if (seek_position.DistanceSq(target_player->position) < 15 * 15) {
  //         ctx.blackboard.Set("target_position", seek_position);
  //     }
  //     else {
  //          ctx.blackboard.Set("target_position", target.position);
  //     }

  if (hit) {
    return behavior::ExecuteResult::Success;
  }

  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult ShootGunNode::Execute(behavior::ExecuteContext& ctx) {
  ctx.bot->GetKeys().Press(VK_CONTROL);
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult ShootBombNode::Execute(behavior::ExecuteContext& ctx) {
  ctx.bot->GetKeys().Press(VK_TAB);
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult MoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();

  //   if (ctx.pb->HasWallShot()) {

  //      ctx.bot->Move(ctx.bot->GetWallShot(), 5.0f);

  //        ctx.bot->GetSteering().Face(ctx.bot->GetWallShot());
  //        return behavior::ExecuteResult::Success;
  //     }

  float energy_pct = game.GetEnergy() / (float)game.GetShipSettings().InitialEnergy;

  bool in_center = ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));
  bool in_safe = game.GetMap().GetTileId(game.GetPlayer().position) == marvin::kSafeTileId;
  Vector2f target_position = ctx.blackboard.ValueOr("target_position", Vector2f());

  int ship = game.GetPlayer().ship;
  //#if 0
  Vector2f mine_position(0, 0);
  for (Weapon* weapon : game.GetWeapons()) {
    const Player* weapon_player = game.GetPlayerById(weapon->GetPlayerId());
    if (weapon_player == nullptr) continue;
    if (weapon_player->frequency == game.GetPlayer().frequency) continue;
    if (weapon->IsMine()) mine_position = (weapon->GetPosition());
    if (mine_position.Distance(game.GetPosition()) < 5.0f) {
      game.Repel(ctx.bot->GetKeys());
    }
  }
  //#endif
  float hover_distance = 0.0f;

  if (in_safe)
    hover_distance = 0.0f;
  else if (in_center) {
    hover_distance = 12.0f / energy_pct;
  } else {
    hover_distance = 3.0f;
  }
  if (hover_distance < 0.0f) hover_distance = 0.0f;
  const Player& shooter = *ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

  ctx.bot->Move(target_position, hover_distance);

  ctx.bot->GetSteering().Face(target_position);

  return behavior::ExecuteResult::Success;
}

bool MoveToEnemyNode::IsAimingAt(GameProxy& game, const Player& shooter, const Player& target, Vector2f* dodge) {
  float proj_speed = game.GetShipSettings(shooter.ship).BulletSpeed / 10.0f / 16.0f;
  float radius = game.GetShipSettings(target.ship).GetRadius() * 1.5f;
  Vector2f box_pos = target.position - Vector2f(radius, radius);

  Vector2f shoot_direction = Normalize(shooter.velocity + (shooter.GetHeading() * proj_speed));

  if (shoot_direction.Dot(shooter.GetHeading()) < 0) {
    shoot_direction = shooter.GetHeading();
  }

  Vector2f extent(radius * 2, radius * 2);

  float shooter_radius = game.GetShipSettings(shooter.ship).GetRadius();
  Vector2f side = Perpendicular(shooter.GetHeading()) * shooter_radius * 1.5f;

  float distance;

  Vector2f directions[2] = {shoot_direction, shooter.GetHeading()};

  for (Vector2f direction : directions) {
    if (RayBoxIntersect(shooter.position, direction, box_pos, extent, &distance, nullptr) ||
        RayBoxIntersect(shooter.position + side, direction, box_pos, extent, &distance, nullptr) ||
        RayBoxIntersect(shooter.position - side, direction, box_pos, extent, &distance, nullptr)) {
#if 0
                    Vector2f hit = shooter.position + shoot_direction * distance;

                    *dodge = Normalize(side * side.Dot(Normalize(hit - target.position)));

#endif

      if (distance < 30) {
        *dodge = Perpendicular(direction);

        return true;
      }
    }
  }

  return false;
}

}  // namespace pb
}  // namespace marvin
