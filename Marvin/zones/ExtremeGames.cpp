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

  uint16_t ship = bot.GetGame().GetPlayer().ship;
  std::vector<MapCoord> patrol_nodes{MapCoord(570, 540), MapCoord(455, 540), MapCoord(455, 500), MapCoord(570, 500)};

    if (ship == 8) {
      ship = 2;
    }

    bot.GetBlackboard().Set<std::vector<MapCoord>>(BBKey::PatrolNodes, patrol_nodes);
    //bot.GetBlackboard().Set<uint16_t>("Freq", 999);
    bot.GetBlackboard().Set<uint16_t>(BBKey::PubEventTeam0, 00);
    bot.GetBlackboard().Set<uint16_t>(BBKey::PubEventTeam1, 01);
    bot.GetBlackboard().Set<uint16_t>(BBKey::RequestedShip, ship);
    bot.GetBlackboard().Set<Vector2f>(BBKey::CenterSpawnPoint, Vector2f(512, 512));
    bot.GetBlackboard().Set<RequestedCommand>(BBKey::RequestedCommand, RequestedCommand::ShipChange);

    //bot.GetBlackboard().Set<uint16_t>("Freq", 999);
    //bot.GetBlackboard().SetPatrolNodes(patrol_nodes);
    //bot.GetBlackboard().SetPubTeam0(00);
    //bot.GetBlackboard().SetPubTeam1(01);
    //bot.GetBlackboard().SetShip(Ship(ship));
    //bot.GetBlackboard().SetShip(Ship(ship));
    //bot.GetBlackboard().SetCommandRequest(CommandRequestType::ShipChange);
    //bot.GetBlackboard().SetCenterSpawn(MapCoord(512, 512));
  

  auto freq_warp_attach = std::make_unique<eg::FreqWarpAttachNode>();
 
  auto aim_with_gun = std::make_unique<eg::AimWithGunNode>();
  auto aim_with_bomb = std::make_unique<eg::AimWithBombNode>();

  auto shoot_gun = std::make_unique<eg::ShootGunNode>();
  auto shoot_bomb = std::make_unique<eg::ShootBombNode>();

  auto move_to_enemy = std::make_unique<eg::MoveToEnemyNode>();

  auto patrol = std::make_unique<bot::PatrolNode>();
  auto set_ship = std::make_unique<bot::SetShipNode>();

  auto move_method_selector = std::make_unique<behavior::SelectorNode>(move_to_enemy.get());
  auto gun_sequence = std::make_unique<behavior::SequenceNode>(aim_with_gun.get(), shoot_gun.get());
  auto bomb_sequence = std::make_unique<behavior::SequenceNode>(aim_with_bomb.get(), shoot_bomb.get());
  auto bomb_gun_sequence = std::make_unique<behavior::SelectorNode>(gun_sequence.get(), bomb_sequence.get());
  auto parallel_shoot_enemy =
      std::make_unique<behavior::ParallelAnyNode>(bomb_gun_sequence.get(), move_method_selector.get());
  auto los_shoot_conditional =
      std::make_unique<behavior::SequenceNode>(target_in_los_.get(), parallel_shoot_enemy.get());
  auto enemy_path_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy_.get(), follow_path_.get());
  auto patrol_path_sequence = std::make_unique<behavior::SequenceNode>(patrol.get(), follow_path_.get());
  auto path_or_shoot_selector =
      std::make_unique<behavior::SelectorNode>(los_shoot_conditional.get(), enemy_path_sequence.get());
  auto handle_enemy =
      std::make_unique<behavior::SequenceNode>(find_enemy_in_center_.get(), path_or_shoot_selector.get());
 
  auto root_selector =
      std::make_unique<behavior::SelectorNode>(freq_warp_attach.get(), handle_enemy.get(), patrol_path_sequence.get());

  auto root_sequence = std::make_unique<behavior::SequenceNode>(
      commands_.get(), respawn_query_.get(), set_ship.get(), spectator_query_.get(), root_selector.get());
                                                              

  engine_->PushRoot(std::move(root_sequence));
  engine_->PushNode(std::move(root_selector));
  engine_->PushNode(std::move(set_ship));
   engine_->PushNode(std::move(freq_warp_attach));
   engine_->PushNode(std::move(aim_with_gun));
  engine_->PushNode(std::move(aim_with_bomb));
  engine_->PushNode(std::move(shoot_gun));
   engine_->PushNode(std::move(shoot_bomb));
   engine_->PushNode(std::move(move_to_enemy));
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

  bool dueling = game.GetPlayer().frequency != 00 && game.GetPlayer().frequency != 01;

  // attach code
  // the region registry doesnt thing the eg center bases are connected to center
  if (in_center && duelers.size() != 0 && team_in_base && dueling) {

    // if ticker is on a player in base attach
    for (std::size_t i = 0; i < duelers.size(); i++) {
      
      const Player& player = duelers[i];
      bool not_connected = ctx.bot->GetRegions().IsConnected(player.position, game.GetPosition());

      if (IsValidPosition(player.position) && not_connected) {
        game.Attach(player.id);
        g_RenderState.RenderDebugText("FreqWarpAttachNode(success): %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Success;
      }
    }

    g_RenderState.RenderDebugText("FreqWarpAttachNode(success): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

if (game.GetPlayer().attach_id != 65535) {
    game.SendKey(VK_F7);

    g_RenderState.RenderDebugText("  HSAttachNode(Dettaching): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
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


behavior::ExecuteResult AimWithGunNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  const Player* target_player = ctx.blackboard.ValueOr<const Player*>(BBKey::TargetPlayer, nullptr);
  //const Player* target_player = ctx.bot->GetBlackboard().GetTarget();

  if (!target_player) {
    g_RenderState.RenderDebugText("AimWithGunNode (No Target): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  const Player& target = *target_player;
  auto& game = ctx.bot->GetGame();

  const Player& bot_player = game.GetPlayer();

  float target_radius = game.GetShipSettings(target.ship).GetRadius();
  float radius = game.GetShipSettings().GetRadius();
  float proj_speed = game.GetShipSettings(bot_player.ship).GetBulletSpeed();

  ShotResult result = ctx.bot->GetShooter().CalculateShot(game.GetPosition(), target.position, bot_player.velocity,
                                                          target.velocity, proj_speed);

  if (!result.hit) {
    ctx.blackboard.Set<Vector2f>(BBKey::AimingSolution, result.solution);
    g_RenderState.RenderDebugText("AimWithGunNode (No Hit): %llu", timer.GetElapsedTime());
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

  if (game.GetShipSettings().HasDoubleBarrel()) {
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

  ctx.blackboard.Set<Vector2f>(BBKey::AimingSolution, result.solution);

  if (rHit) {
    if (CanShootGun(game, game.GetMap(), game.GetPosition(), result.solution)) {
      g_RenderState.RenderDebugText("AimWithGunNode (Hit): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Success;
    }
  }

  g_RenderState.RenderDebugText("AimWithGunNode (No Hit): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult AimWithBombNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  const Player* target_player = ctx.blackboard.ValueOr<const Player*>(BBKey::TargetPlayer, nullptr);
  //const Player* target_player = ctx.bot->GetBlackboard().GetTarget();

  if (!target_player) {
    g_RenderState.RenderDebugText("AimWithBombNode (No Target): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
  }

  const Player& target = *target_player;
  auto& game = ctx.bot->GetGame();
  const Player& bot_player = game.GetPlayer();

  float proj_speed = game.GetShipSettings(bot_player.ship).GetBombSpeed();
  bool has_shot = false;

  ShotResult result = ctx.bot->GetShooter().CalculateShot(game.GetPosition(), target.position, bot_player.velocity,
                                                          target.velocity, proj_speed);



  if (!result.hit) {
    ctx.blackboard.Set<Vector2f>(BBKey::AimingSolution, result.solution);
    g_RenderState.RenderDebugText("AimWithBombNode (No Hit): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  //  RenderWorldLine(game.GetPosition(), game.GetPosition(), solution, RGB(100, 0, 100));
  //  RenderText("bomb speed  " + std::to_string(proj_speed), GetWindowCenter() - Vector2f(0, 60), RGB(100, 100, 100),
  //  RenderText_Centered);

  float target_radius = game.GetShipSettings(target.ship).GetRadius();

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

  ctx.blackboard.Set<Vector2f>(BBKey::AimingSolution, result.solution);

  if (hit) {
    if (CanShootBomb(game, game.GetMap(), game.GetPosition(), result.solution)) {
      g_RenderState.RenderDebugText("AimWithBombNode (Hit): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Success;
    }
  }

  g_RenderState.RenderDebugText("AimWithBombNode (No Hit): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult ShootGunNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  ctx.bot->GetKeys().Press(VK_CONTROL);
  g_RenderState.RenderDebugText("ShootGunNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult ShootBombNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  ctx.bot->GetKeys().Press(VK_TAB);
  g_RenderState.RenderDebugText("ShootBombNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult MoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();

  //const Player* target_player = ctx.bot->GetBlackboard().GetTarget();
  const Player* target_player = ctx.blackboard.ValueOr<const Player*>(BBKey::TargetPlayer, nullptr);

  if (!target_player) {
    g_RenderState.RenderDebugText("MoveToEnemyNode (No Target): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  float energy_pct = (game.GetEnergy() / (float)game.GetShipSettings().InitialEnergy) * 100.0f;

  bool in_center = ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));
  bool in_safe = game.GetMap().GetTileId(game.GetPlayer().position) == marvin::kSafeTileId;
  Vector2f shot_position = ctx.blackboard.ValueOr<Vector2f>(BBKey::AimingSolution, Vector2f());

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

  ctx.bot->Move(target_player->position, hover_distance);

  ctx.bot->GetSteering().Face(*ctx.bot, shot_position);

  g_RenderState.RenderDebugText("MoveToEnemyNode: %llu", timer.GetElapsedTime());
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
}  // namespace eg
}  // namespace marvin