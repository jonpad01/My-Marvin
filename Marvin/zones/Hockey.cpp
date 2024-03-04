#include "Hockey.h"

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
namespace hz {

void HockeyBehaviorBuilder::CreateBehavior(Bot& bot) {

  auto find_enemy = std::make_unique<hz::FindEnemyNode>();
  auto looking_at_enemy = std::make_unique<hz::LookingAtEnemyNode>();
  auto target_in_los = std::make_unique<hz::InLineOfSightNode>();
  auto shoot_enemy = std::make_unique<hz::ShootEnemyNode>();
  auto path_to_enemy = std::make_unique<hz::PathToEnemyNode>();
  auto move_to_enemy = std::make_unique<hz::MoveToEnemyNode>();
  auto follow_path = std::make_unique<hz::FollowPathNode>();
  auto patrol = std::make_unique<hz::PatrolNode>();

  auto move_method_selector = std::make_unique<behavior::SelectorNode>(move_to_enemy.get());
  auto shoot_sequence = std::make_unique<behavior::SequenceNode>(looking_at_enemy.get(), shoot_enemy.get());
  auto parallel_shoot_enemy =
      std::make_unique<behavior::ParallelNode>(shoot_sequence.get(), move_method_selector.get());
  auto los_shoot_conditional =
      std::make_unique<behavior::SequenceNode>(target_in_los.get(), parallel_shoot_enemy.get());
  auto enemy_path_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy.get(), follow_path.get());
  auto patrol_path_sequence = std::make_unique<behavior::SequenceNode>(patrol.get(), follow_path.get());
  auto path_or_shoot_selector =
      std::make_unique<behavior::SelectorNode>(los_shoot_conditional.get(), enemy_path_sequence.get());
  auto handle_enemy = std::make_unique<behavior::SequenceNode>(find_enemy.get(), path_or_shoot_selector.get());

  auto root_selector = std::make_unique<behavior::SelectorNode>(handle_enemy.get(), patrol_path_sequence.get());

  auto ball_selector = std::make_unique<hz::BallSelectorNode>();

  auto root_sequence = std::make_unique<behavior::SequenceNode>(ball_selector.get(), root_selector.get());

  engine_->PushRoot(std::move(root_sequence));

  engine_->PushNode(std::move(find_enemy));
  engine_->PushNode(std::move(looking_at_enemy));
  engine_->PushNode(std::move(target_in_los));
  engine_->PushNode(std::move(shoot_enemy));
  engine_->PushNode(std::move(path_to_enemy));
  engine_->PushNode(std::move(move_to_enemy));
  engine_->PushNode(std::move(follow_path));
  engine_->PushNode(std::move(patrol));

  engine_->PushNode(std::move(move_method_selector));
  engine_->PushNode(std::move(shoot_sequence));
  engine_->PushNode(std::move(parallel_shoot_enemy));
  engine_->PushNode(std::move(los_shoot_conditional));
  engine_->PushNode(std::move(enemy_path_sequence));
  engine_->PushNode(std::move(patrol_path_sequence));
  engine_->PushNode(std::move(path_or_shoot_selector));
  engine_->PushNode(std::move(handle_enemy));
  engine_->PushNode(std::move(root_selector));
  engine_->PushNode(std::move(ball_selector));
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

behavior::ExecuteResult FindEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  behavior::ExecuteResult result = behavior::ExecuteResult::Failure;

  auto& game = ctx.bot->GetGame();
  float closest_cost = std::numeric_limits<float>::max();
  float cost = 0;

  const Player& bot = ctx.bot->GetGame().GetPlayer();

  const Player* target = nullptr;

  const BallData* ball = ctx.blackboard.ValueOr<const BallData*>("TargetBall", nullptr);

  for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
    const Player& player = game.GetPlayers()[i];

    if (!IsValidTarget(ctx, player)) continue;

    if (ball && ball->held && player.id == ball->last_holder_id) {
      target = &game.GetPlayers()[i];
      result = behavior::ExecuteResult::Success;
    } else {
      cost = CalculateCost(game, bot, player);

      if (cost < closest_cost) {
        closest_cost = cost;
        target = &game.GetPlayers()[i];
        result = behavior::ExecuteResult::Success;
      }
    }
  }

  ctx.blackboard.Set<const Player*>("target_player", target);

  return result;
}

float FindEnemyNode::CalculateCost(GameProxy& game, const Player& bot_player, const Player& target) {
  float dist = bot_player.position.Distance(target.position);
  // How many seconds it takes to rotate 180 degrees
  float rotate_speed = game.GetShipSettings().GetInitialRotation();       // MaximumRotation
  float move_cost = dist / game.GetShipSettings().GetInitialSpeed();  // MaximumSpeed
  Vector2f direction = Normalize(target.position - bot_player.position);
  float dot = std::abs(bot_player.GetHeading().Dot(direction) - 1.0f) / 2.0f;
  float rotate_cost = std::abs(dot) * rotate_speed;

  return move_cost + rotate_cost;
}

bool FindEnemyNode::IsValidTarget(behavior::ExecuteContext& ctx, const Player& target) {
  const auto& game = ctx.bot->GetGame();
  const Player& bot_player = game.GetPlayer();

  if (target.dead) return false;
  if (target.id == game.GetPlayer().id) return false;
  if (target.ship > 7) return false;
  if (target.frequency == game.GetPlayer().frequency) return false;
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
  Vector2f enemy = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr)->position;

  path = ctx.bot->GetPathfinder().CreatePath(*ctx.bot, bot, enemy, game.GetShipSettings().GetRadius());

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

  const Vector2f goal_0 = Vector2f(404, 512);
  const Vector2f goal_1 = Vector2f(619, 512);

  if (ball->held && ball->last_holder_id == game.GetPlayer().id) {
#if 0
                if (game.GetPlayer().frequency == 01) {
                    if (ctx.bot->GetRegions().IsConnected(game.GetPosition(), goal_0)) {
                        path = CreatePath(ctx, "path", from, goal_0, game.GetShipSettings().GetRadius());
                    }
                    else path = CreatePath(ctx, "path", from, goal_1, game.GetShipSettings().GetRadius());
                }
                else if (game.GetPlayer().frequency == 00) {
                 
                    if (ctx.bot->GetRegions().IsConnected(game.GetPosition(), goal_1)) {
                        path = CreatePath(ctx, "path", from, goal_1, game.GetShipSettings().GetRadius());
                    }
                    else path = CreatePath(ctx, "path", from, goal_0, game.GetShipSettings().GetRadius());
                }
#endif
    path = ctx.bot->GetPathfinder().CreatePath(*ctx.bot, from, goal_0, game.GetShipSettings().GetRadius());

    if (game.GetPosition().DistanceSq(goal_1) < 25.0f * 25.0f ||
        game.GetPosition().DistanceSq(goal_0) < 25.0f * 25.0f) {
      ctx.bot->GetKeys().Press(VK_CONTROL);
    }

  } else
    path = ctx.bot->GetPathfinder().CreatePath(*ctx.bot, from, ball->position, game.GetShipSettings().GetRadius());

#if 0
            std::vector<Vector2f> nodes{ Vector2f(570, 540), Vector2f(455, 540), Vector2f(455, 500), Vector2f(570, 500) };
            int index = ctx.blackboard.ValueOr<int>("patrol_index", 0);
            Vector2f to = nodes.at(index);

            if (game.GetPosition().DistanceSq(to) < 5.0f * 5.0f) {
                index = (index + 1) % nodes.size();
                ctx.blackboard.Set("patrol_index", index);
                to = nodes.at(index);
            }
#endif

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

  while (path.size() > 1 && CanMoveBetween(*ctx.bot, game.GetPosition(), path.at(1))) {
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

bool FollowPathNode::CanMoveBetween(Bot& bot, Vector2f from, Vector2f to) {
  Vector2f trajectory = to - from;
  Vector2f direction = Normalize(trajectory);
  Vector2f side = Perpendicular(direction);

  float distance = from.Distance(to);
  float radius = bot.GetGame().GetShipSettings().GetRadius();

  return !RadiusRayCastHit(bot, from, to, radius);
}

behavior::ExecuteResult InLineOfSightNode::Execute(behavior::ExecuteContext& ctx) {
  behavior::ExecuteResult result = behavior::ExecuteResult::Failure;

  const auto target = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
  if (!target) return behavior::ExecuteResult::Failure;

  auto& game = ctx.bot->GetGame();

  auto to_target = target->position - game.GetPosition();
  CastResult ray_cast = SolidRayCast(*ctx.bot, game.GetPosition(), target->position);

  if (!ray_cast.hit) {
    result = behavior::ExecuteResult::Success;
  }

  return result;
}

behavior::ExecuteResult LookingAtEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  const auto target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

  if (!target_player) {
    return behavior::ExecuteResult::Failure;
  }

  const Player& target = *target_player;
  auto& game = ctx.bot->GetGame();

  const Player& bot_player = game.GetPlayer();

  float target_radius = game.GetSettings().ShipSettings[target.ship].GetRadius();
  float radius = game.GetShipSettings().GetRadius();
  float proj_speed = game.GetSettings().ShipSettings[bot_player.ship].GetBulletSpeed();

  ShotResult result = ctx.bot->GetShooter().CalculateShot(game.GetPosition(), target.position,
                                                          bot_player.velocity,
                                                          target.velocity, proj_speed);

  if (!result.hit) {
    ctx.blackboard.Set<Vector2f>("shot_position", result.solution);
    ctx.blackboard.Set<bool>("has_shot", false);
    return behavior::ExecuteResult::Failure;
  }

  Vector2f totarget = result.solution - game.GetPosition();
  RenderLine(GetWindowCenter(), GetWindowCenter() + (Normalize(totarget) * totarget.Length() * 16), RGB(100, 0, 0));

  // Vector2f projectile_trajectory = (bot_player.GetHeading() * proj_speed) + bot_player.velocity;
  // Vector2f projectile_direction = Normalize(projectile_trajectory);

  float radius_multiplier = 1.2f;

  // if the target is cloaking and bot doesnt have xradar make the bot shoot wide
  if (!(game.GetPlayer().status & 4)) {
    if (target.status & 2) {
      radius_multiplier = 3.0f;
    }
  }

  float nearby_radius = target_radius * radius_multiplier;

  // Vector2f box_min = target.position - Vector2f(nearby_radius, nearby_radius);
  Vector2f box_min = result.solution - Vector2f(nearby_radius, nearby_radius);
  Vector2f box_extent(nearby_radius * 1.2f, nearby_radius * 1.2f);
  float dist;
  Vector2f norm;
  bool rHit = false;

  if ((game.GetShipSettings().DoubleBarrel & 1) != 0) {
    Vector2f side = Perpendicular(bot_player.GetHeading());

    bool rHit1 = FloatingRayBoxIntersect(bot_player.position + side * radius, bot_player.GetHeading(), result.solution,
                                 nearby_radius,
                                 &dist, &norm);
    bool rHit2 = FloatingRayBoxIntersect(bot_player.position - side * radius, bot_player.GetHeading(), result.solution,
                                 nearby_radius,
                                 &dist, &norm);
    if (rHit1 || rHit2) {
      rHit = true;
    }

  } else {
    rHit = FloatingRayBoxIntersect(bot_player.position, bot_player.GetHeading(), result.solution, nearby_radius, &dist,
                                   &norm);
  }

  ctx.blackboard.Set<bool>("has_shot", rHit);
  ctx.blackboard.Set<Vector2f>("shot_position", result.solution);

  // bool hit = RayBoxIntersect(bot_player.position, projectile_direction, box_min, box_extent, &dist, &norm);

  if (rHit) {
    if (CanShootGun(game, game.GetMap(), game.GetPosition(), result.solution)) {
      return behavior::ExecuteResult::Success;
    }
  }
  return behavior::ExecuteResult::Failure;
}

bool LookingAtEnemyNode::CanShoot(const marvin::Map& map, const marvin::Player& bot_player,
                                  const marvin::Player& target) {
  if (bot_player.position.DistanceSq(target.position) > 4 * 4) return false;
  if (map.GetTileId(bot_player.position) == marvin::kSafeTileId) return false;

  return true;
}

behavior::ExecuteResult ShootEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  ctx.bot->GetKeys().Press(VK_CONTROL);

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult MoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();

  Vector2f target_position = ctx.blackboard.ValueOr("shot_position", Vector2f());

  float hover_distance = 1.0f;

  ctx.bot->Move(target_position, hover_distance);

  ctx.bot->GetSteering().Face(*ctx.bot, target_position);

  return behavior::ExecuteResult::Success;
}

bool MoveToEnemyNode::IsAimingAt(GameProxy& game, const Player& shooter, const Player& target, Vector2f* dodge) {
  float proj_speed = game.GetShipSettings(shooter.ship).GetBulletSpeed();
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
    if (FloatingRayBoxIntersect(shooter.position, direction, target.position, radius, &distance, nullptr) ||
        FloatingRayBoxIntersect(shooter.position + side, direction, target.position, radius, &distance, nullptr) ||
        FloatingRayBoxIntersect(shooter.position - side, direction, target.position, radius, &distance, nullptr)) {
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
}  // namespace hz
}  // namespace marvin