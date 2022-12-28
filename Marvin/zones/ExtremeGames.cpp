#include "ExtremeGames.h"

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
namespace eg {

void ExtremeGamesBehaviorBuilder::CreateBehavior(Bot& bot) {

    Path seed_point{Vector2f(512, 512)};

    // only create a region for center because the other method wasnt working
    //bot.GetRegions().CreateRegions(bot.GetGame().GetMap(), seed_point, 0.875f);
    bot.GetRegions().CreateAll(bot.GetGame().GetMap(), 0.875f);
    marvin::debug_log << "regions created" << std::endl;

  uint16_t ship = bot.GetGame().GetPlayer().ship;

    if (ship == 8) {
      ship = 2;
    }

    bot.GetBlackboard().Set<uint16_t>("Freq", 999);
    bot.GetBlackboard().Set<uint16_t>("PubTeam0", 00);
    bot.GetBlackboard().Set<uint16_t>("PubTeam1", 01);
    bot.GetBlackboard().Set<uint16_t>("Ship", ship);
    bot.GetBlackboard().Set<Vector2f>("Spawn", Vector2f(512, 512));
  

  auto freq_warp_attach = std::make_unique<eg::FreqWarpAttachNode>();
  auto find_enemy = std::make_unique<eg::FindEnemyNode>();
  auto aim_with_gun = std::make_unique<eg::AimWithGunNode>();
  auto aim_with_bomb = std::make_unique<eg::AimWithBombNode>();
  auto target_in_los = std::make_unique<eg::InLineOfSightNode>();
  auto shoot_gun = std::make_unique<eg::ShootGunNode>();
  auto shoot_bomb = std::make_unique<eg::ShootBombNode>();
  auto path_to_enemy = std::make_unique<eg::PathToEnemyNode>();
  auto move_to_enemy = std::make_unique<eg::MoveToEnemyNode>();
  //auto follow_path = std::make_unique<eg::FollowPathNode>();
  auto patrol = std::make_unique<eg::PatrolNode>();

  auto move_method_selector = std::make_unique<behavior::SelectorNode>(move_to_enemy.get());
  auto gun_sequence = std::make_unique<behavior::SequenceNode>(aim_with_gun.get(), shoot_gun.get());
  auto bomb_sequence = std::make_unique<behavior::SequenceNode>(aim_with_bomb.get(), shoot_bomb.get());
  auto bomb_gun_sequence = std::make_unique<behavior::SelectorNode>(gun_sequence.get(), bomb_sequence.get());
  auto parallel_shoot_enemy =
      std::make_unique<behavior::ParallelNode>(bomb_gun_sequence.get(), move_method_selector.get());
  auto los_shoot_conditional =
      std::make_unique<behavior::SequenceNode>(target_in_los.get(), parallel_shoot_enemy.get());
  auto enemy_path_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy.get(), follow_path_.get());
  auto patrol_path_sequence = std::make_unique<behavior::SequenceNode>(patrol.get(), follow_path_.get());
  auto path_or_shoot_selector =
      std::make_unique<behavior::SelectorNode>(los_shoot_conditional.get(), enemy_path_sequence.get());
  auto handle_enemy = std::make_unique<behavior::SequenceNode>(find_enemy.get(), path_or_shoot_selector.get());
  auto root_selector =
      std::make_unique<behavior::SelectorNode>(freq_warp_attach.get(), handle_enemy.get(), patrol_path_sequence.get());

  engine_->PushRoot(std::move(root_selector));

   engine_->PushNode(std::move(freq_warp_attach));
  engine_->PushNode(std::move(find_enemy));
   engine_->PushNode(std::move(aim_with_gun));
  engine_->PushNode(std::move(aim_with_bomb));
   engine_->PushNode(std::move(target_in_los));
  engine_->PushNode(std::move(shoot_gun));
   engine_->PushNode(std::move(shoot_bomb));
  engine_->PushNode(std::move(path_to_enemy));
   engine_->PushNode(std::move(move_to_enemy));
 // engine_->PushNode(std::move(follow_path));
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
}




behavior::ExecuteResult FreqWarpAttachNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();

  bool in_center = ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));
  uint64_t time = ctx.bot->GetTime().GetTime();
  uint64_t spam_check = ctx.blackboard.ValueOr<uint64_t>("SpamCheck", 0);
  uint64_t f7_check = ctx.blackboard.ValueOr<uint64_t>("F7SpamCheck", 0);
  uint64_t chat_check = ctx.blackboard.ValueOr<uint64_t>("ChatWait", 0);
  uint64_t p_check = ctx.blackboard.ValueOr<uint64_t>("!PCheck", 0);
  bool no_spam = time > spam_check;
  bool chat_wait = time > chat_check;
  bool no_p = time > p_check;
  bool no_f7_spam = time > f7_check;

  std::vector<Player> duelers;
  bool team_in_base = false;

  // find and store attachable team players
  for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
    const Player& player = game.GetPlayers()[i];
    bool in_center = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));
    bool same_team = player.frequency == game.GetPlayer().frequency;
    bool not_bot = player.id != game.GetPlayer().id;
    if (!in_center && same_team && not_bot && player.ship < 8) {
      duelers.push_back(game.GetPlayers()[i]);
      team_in_base = true;
    }
  }

  // read private messages for !p and send the same message to join a duel team
  // std::vector<std::string> chat = game.GetChat(5);
  for (ChatMessage chat : game.GetChat()) {
    if (chat.message == "!p" && no_p) {
      game.P();

      ctx.blackboard.Set("!PCheck", time + 3000);
      ctx.blackboard.Set("ChatWait", time + 200);

      g_RenderState.RenderDebugText("FreqWarpAttachNode(success): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Success;
    } else if (chat.message == "!l" && no_p) {
      game.L();

      ctx.blackboard.Set("!PCheck", time + 3000);
      ctx.blackboard.Set("ChatWait", time + 200);

      g_RenderState.RenderDebugText("FreqWarpAttachNode(success): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Success;
    } else if (chat.message == "!r" && no_p) {
      game.R();

      ctx.blackboard.Set("!PCheck", time + 3000);
      ctx.blackboard.Set("ChatWait", time + 200);

      g_RenderState.RenderDebugText("FreqWarpAttachNode(success): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Success;
    }
  }

  // bot needs to halt so eg can process chat input, i guess
  if (!chat_wait) {
    g_RenderState.RenderDebugText("FreqWarpAttachNode(success): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  bool dueling = game.GetPlayer().frequency != 00 && game.GetPlayer().frequency != 01;

  // attach code
  // the region registry doesnt thing the eg center bases are connected to center
  if (in_center && duelers.size() != 0 && team_in_base && dueling) {
    // a saved value to keep the ticker moving up or down
    bool up_down = ctx.blackboard.ValueOr<bool>("UpDown", true);

    // what player the ticker is currently on
    const Player& selected_player = game.GetSelectedPlayer();

    // if ticker is on a player in base attach
    for (std::size_t i = 0; i < duelers.size(); i++) {
      const Player& player = duelers[i];

      if (player.id == selected_player.id && IsValidPosition(player.position)) {
        if (CheckStatus(ctx)) game.F7();
        g_RenderState.RenderDebugText("FreqWarpAttachNode(success): %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Success;
      }
    }

    // if the ticker is not on a player in base then move the ticker for the next check
    if (up_down)
      game.PageUp();
    else
      game.PageDown();

    // find the ticker position
    int64_t ticker = game.TickerPosition();
    // if the ticker has reached the end of the list switch direction
    if (ticker <= 0)
      ctx.blackboard.Set("UpDown", false);
    else if (ticker >= game.GetPlayers().size() - 1)
      ctx.blackboard.Set("UpDown", true);

    g_RenderState.RenderDebugText("FreqWarpAttachNode(success): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  Vector2f bot_position = game.GetPlayer().position;
  bool in_safe = game.GetMap().GetTileId(bot_position) == kSafeTileId;
  // try to detach
  for (std::size_t i = 0; i < duelers.size(); i++) {
    const Player& player = duelers[i];
    if (player.position == bot_position && no_f7_spam && !in_center) {
      game.F7();
      ctx.blackboard.Set("F7SpamCheck", time + 150);

      g_RenderState.RenderDebugText("FreqWarpAttachNode(success): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Success;
    }
  }

  //#if 0

  bool x_active = (game.GetPlayer().status & 4) != 0;
  bool stealthing = (game.GetPlayer().status & 1) != 0;
  bool cloaking = (game.GetPlayer().status & 2) != 0;

  bool has_xradar = (game.GetShipSettings().XRadarStatus & 1) != 0;
  bool has_stealth = (game.GetShipSettings().StealthStatus & 1) != 0;
  bool has_cloak = (game.GetShipSettings().CloakStatus & 3) != 0;
#if 0
            //if stealth isnt on but availible, presses home key in continuumgameproxy.cpp
            if (!stealthing && has_stealth && no_spam) {
                game.Stealth();
                ctx.blackboard.Set("SpamCheck", time + 300);
                return behavior::ExecuteResult::Success;
            }
#endif
#if 0
            //same as stealth but presses shift first
            if (!cloaking && has_cloak && no_spam) {
                game.Cloak(ctx.bot->GetKeys());
                ctx.blackboard.Set("SpamCheck", time + 300);
                return behavior::ExecuteResult::Success;
            }
//#endif
//#if 0
            //in deva xradar is free so just turn it on
            if (!x_active && has_xradar && no_spam) {
                game.XRadar();
                ctx.blackboard.Set("SpamCheck", time + 300);
                return behavior::ExecuteResult::Success;
            }
//#endif
#endif
  g_RenderState.RenderDebugText("FreqWarpAttachNode(fail): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

bool FreqWarpAttachNode::CheckStatus(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  float energy_pct = (game.GetEnergy() / (float)game.GetShipSettings().InitialEnergy) * 100.0f;
  bool result = false;
  if ((game.GetPlayer().status & 2) != 0) {
    game.Cloak(ctx.bot->GetKeys());
    return false;
  }
  if ((game.GetPlayer().status & 1) != 0) {
    game.Stealth();
    return false;
  }
  if (energy_pct >= 100.0f) result = true;
  return result;
}

behavior::ExecuteResult FindEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  behavior::ExecuteResult result = behavior::ExecuteResult::Failure;
  float closest_cost = std::numeric_limits<float>::max();
  float cost = 0;
  auto& game = ctx.bot->GetGame();
  const Player* target = nullptr;
  const Player& bot = ctx.bot->GetGame().GetPlayer();

  Vector2f position = game.GetPosition();
  Vector2f resolution(1920, 1080);
  view_min_ = bot.position - resolution / 2.0f / 16.0f;
  view_max_ = bot.position + resolution / 2.0f / 16.0f;

  // this loop checks every player and finds the closest one based on a cost formula
  // if this does not succeed then there is no target, it will go into patrol mode
  for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
    const Player& player = game.GetPlayers()[i];

    if (!IsValidTarget(ctx, player)) continue;
    auto to_target = player.position - position;
    bool in_sight = !RadiusRayCastHit(*ctx.bot, game.GetPosition(), Normalize(to_target), to_target.Length());

    // make players in line of sight high priority

    if (in_sight) {
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
  
  if (result == behavior::ExecuteResult::Success) {
    g_RenderState.RenderDebugText("FindEnemyNode(success): %llu", timer.GetElapsedTime());
  } else {
    g_RenderState.RenderDebugText("FindEnemyNode(fail): %llu", timer.GetElapsedTime());
  }
  
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

  if (!target.active) return false;
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
  // TODO: check if player is cloaking and outside radar range
  // 1 = stealth, 2= cloak, 3 = both, 4 = xradar
  bool stealthing = (target.status & 1) != 0;
  bool cloaking = (target.status & 2) != 0;

  // if the bot doesnt have xradar
  if (!(game.GetPlayer().status & 4)) {
    if (stealthing && cloaking) return false;

    bool visible = InRect(target.position, view_min_, view_max_);

    if (stealthing && !visible) return false;
  }

  return true;
}

behavior::ExecuteResult PathToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();

  std::vector<Vector2f> path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());

  Vector2f bot = game.GetPosition();
  Vector2f enemy = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr)->position;

  path = ctx.bot->GetPathfinder().CreatePath(*ctx.bot, bot, enemy, game.GetShipSettings().GetRadius());

  ctx.blackboard.Set("path", path);

  g_RenderState.RenderDebugText("PathToEnemyNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult PatrolNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  Vector2f from = game.GetPosition();

  std::vector<Vector2f> path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());

  std::vector<Vector2f> nodes{Vector2f(570, 540), Vector2f(455, 540), Vector2f(455, 500), Vector2f(570, 500)};
  int index = ctx.blackboard.ValueOr<int>("patrol_index", 0);

  Vector2f to = nodes.at(index);

  if (game.GetPosition().DistanceSq(to) < 5.0f * 5.0f) {
    index = (index + 1) % nodes.size();
    ctx.blackboard.Set("patrol_index", index);
    to = nodes.at(index);
  }

  path = ctx.bot->GetPathfinder().CreatePath(*ctx.bot, from, to, game.GetShipSettings().GetRadius());
  ctx.blackboard.Set("path", path);

  g_RenderState.RenderDebugText("PatrolNode(Success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult FollowPathNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());

  size_t path_size = path.size();
  auto& game = ctx.bot->GetGame();

  if (path.empty()) {
    g_RenderState.RenderDebugText("FollowPathNode(fail): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

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

  while (path.size() > 1 && CanMoveBetween(*ctx.bot, game, game.GetPosition(), path.at(1))) {
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

  g_RenderState.RenderDebugText("FollowPathNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

bool FollowPathNode::CanMoveBetween(Bot& bot, GameProxy& game, Vector2f from, Vector2f to) {
  Vector2f trajectory = to - from;
  Vector2f direction = Normalize(trajectory);
  Vector2f side = Perpendicular(direction);

  float distance = from.Distance(to);
  float radius = game.GetShipSettings().GetRadius();

  bool hit = DiameterRayCastHit(bot, from, to, 0.875f);

  return !hit;
}

behavior::ExecuteResult InLineOfSightNode::Execute(behavior::ExecuteContext& ctx) {
  behavior::ExecuteResult result = behavior::ExecuteResult::Failure;

  const Player* target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
  if (!target_player) {
    return behavior::ExecuteResult::Failure;
  }

  auto& game = ctx.bot->GetGame();

  auto to_target = target_player->position - game.GetPosition();
  Vector2f direction = Normalize(to_target);
  Vector2f side = Perpendicular(direction);
  float radius = game.GetShipSettings().GetRadius();

  CastResult center = RayCast(*ctx.bot, RayBarrier::Solid, game.GetPosition(), direction, to_target.Length());

  if (!center.hit) {
    CastResult side1 =
        RayCast(*ctx.bot, RayBarrier::Solid, game.GetPosition() + side * radius, direction, to_target.Length());
    CastResult side2 =
        RayCast(*ctx.bot, RayBarrier::Solid, game.GetPosition() - side * radius, direction, to_target.Length());

    if (!side1.hit && !side2.hit) {
      result = behavior::ExecuteResult::Success;
      // if bot is not in center, almost dead, and in line of sight of its target burst
      bool in_center = ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));
      float energy_pct = ctx.bot->GetGame().GetEnergy() / (float)ctx.bot->GetGame().GetShipSettings().MaximumEnergy;
      if (energy_pct < 0.10f && !in_center && game.GetPlayer().bursts > 0) {
        ctx.bot->GetGame().Burst(ctx.bot->GetKeys());
      }
    }
  } else {
    float target_radius = game.GetSettings().ShipSettings[target_player->ship].GetRadius();
    bool shoot = false;
    bool bomb_hit = false;
    Vector2f wall_pos;

    ShotResult bResult =
        ctx.bot->GetShooter().BouncingBombShot(*ctx.bot, target_player->position, target_player->velocity, target_radius);
    ShotResult gResult =
        ctx.bot->GetShooter().BouncingBulletShot(*ctx.bot, target_player->position, target_player->velocity, target_radius);

      if (game.GetMap().GetTileId(game.GetPosition()) != marvin::kSafeTileId) {
      if (bResult.hit) {
          ctx.bot->GetKeys().Press(VK_TAB);
      } else if (gResult.hit) {
          ctx.bot->GetKeys().Press(VK_CONTROL);
        }
      }
 
  }

  return result;
}

behavior::ExecuteResult AimWithGunNode::Execute(behavior::ExecuteContext& ctx) {
  const auto target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

  if (!target_player) {
    return behavior::ExecuteResult::Failure;
  }

  const Player& target = *target_player;
  auto& game = ctx.bot->GetGame();

  const Player& bot_player = game.GetPlayer();

  float target_radius = game.GetSettings().ShipSettings[target.ship].GetRadius();
  float radius = game.GetShipSettings().GetRadius();
  float proj_speed = game.GetSettings().ShipSettings[bot_player.ship].BulletSpeed / 10.0f / 16.0f;

  ShotResult result = ctx.bot->GetShooter().CalculateShot(game.GetPosition(), target.position, bot_player.velocity,
                                                          target.velocity, proj_speed);

  if (!result.hit) {
    ctx.blackboard.Set<Vector2f>("shot_position", result.solution);
    ctx.blackboard.Set<bool>("bullet_shot", false);
    return behavior::ExecuteResult::Failure;
  }
  // RenderWorldLine(game.GetPosition(), game.GetPosition(), solution, RGB(100, 0, 0));
  // RenderText("gun speed  " + std::to_string(proj_speed), GetWindowCenter() - Vector2f(0, 40), RGB(100, 100, 100),
  // RenderText_Centered);

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

  ctx.blackboard.Set<Vector2f>("shot_position", result.solution);

  if (rHit) {
    if (CanShootGun(game, game.GetMap(), game.GetPosition(), result.solution)) {
      ctx.blackboard.Set<bool>("bullet_shot", true);
      return behavior::ExecuteResult::Success;
    }
  }

  ctx.blackboard.Set<bool>("bullet_shot", false);
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult AimWithBombNode::Execute(behavior::ExecuteContext& ctx) {
  const auto target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

  if (!target_player) return behavior::ExecuteResult::Failure;

  const Player& target = *target_player;
  auto& game = ctx.bot->GetGame();
  const Player& bot_player = game.GetPlayer();

  float proj_speed = game.GetSettings().ShipSettings[bot_player.ship].BombSpeed / 10.0f / 16.0f;
  bool has_shot = false;

  ShotResult result = ctx.bot->GetShooter().CalculateShot(game.GetPosition(), target.position, bot_player.velocity,
                                                          target.velocity, proj_speed);



  if (!result.hit) {
    ctx.blackboard.Set<Vector2f>("shot_position", result.solution);
    ctx.blackboard.Set<bool>("bomb_shot", false);
    return behavior::ExecuteResult::Failure;
  }

  //  RenderWorldLine(game.GetPosition(), game.GetPosition(), solution, RGB(100, 0, 100));
  //  RenderText("bomb speed  " + std::to_string(proj_speed), GetWindowCenter() - Vector2f(0, 60), RGB(100, 100, 100),
  //  RenderText_Centered);

  float target_radius = game.GetSettings().ShipSettings[target.ship].GetRadius();

  float radius_multiplier = 1.0f;

  // if the target is cloaking and bot doesnt have xradar make the bot shoot wide
  if (!(game.GetPlayer().status & 4)) {
    if (target.status & 2) {
      radius_multiplier = 3.0f;
    }
  }

  float nearby_radius = target_radius * radius_multiplier;

  Vector2f box_min = result.solution - Vector2f(nearby_radius, nearby_radius);
  Vector2f box_extent(nearby_radius * 1.2f, nearby_radius * 1.2f);
  float dist;
  Vector2f norm;

  bool hit = FloatingRayBoxIntersect(bot_player.position, bot_player.GetHeading(), result.solution, nearby_radius,
                                     &dist, &norm);

  ctx.blackboard.Set<Vector2f>("shot_position", result.solution);

  if (hit) {
    if (CanShootBomb(game, game.GetMap(), game.GetPosition(), result.solution)) {
      ctx.blackboard.Set<bool>("bomb_shot", true);
      return behavior::ExecuteResult::Success;
    }
  }

  ctx.blackboard.Set<bool>("bomb_shot", false);
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

  float energy_pct = (game.GetEnergy() / (float)game.GetShipSettings().InitialEnergy) * 100.0f;

  bool in_center = ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));
  bool in_safe = game.GetMap().GetTileId(game.GetPlayer().position) == marvin::kSafeTileId;
  Vector2f target_position = ctx.blackboard.ValueOr<Vector2f>("shot_position", Vector2f());

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
    hover_distance = 45.0f - (energy_pct / 4.0f);
  } else {
    hover_distance = 10.0f;
  }
  if (hover_distance < 0.0f) hover_distance = 0.0f;

  ctx.bot->Move(target_position, hover_distance);

  ctx.bot->GetSteering().Face(*ctx.bot, target_position);

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
}  // namespace eg
}  // namespace marvin