#include "Devastation.h"

#include <chrono>
#include <cstring>
#include <limits>
#include <filesystem>

#include "../../Bot.h"
#include "../../Debug.h"
#include "../../GameProxy.h"
#include "../../Map.h"
#include "../../RayCaster.h"
#include "../../RegionRegistry.h"
#include "../../Shooter.h"
#include "../../platform/ContinuumGameProxy.h"
#include "../../platform/Platform.h"
#include "BaseDuelWarpCoords.h"

namespace marvin {
namespace deva {


void DevastationBehaviorBuilder::CreateBehavior(Bot& bot) {
  //const BaseSpawns& spawn = bot.GetBaseDuelSpawns().GetSpawns();
  float radius = bot.GetGame().GetShipSettings().GetRadius();
  //bot.GetBlackboard().SetCombatRole(CombatRole::Rusher);

  //if (!bot.GetBlackboard().Has("combatrole")) {
  //  bot.GetBlackboard().Set<CombatRole>("combatrole", CombatRole::Rusher);
 // }
 
  //bot.CreateBasePaths(spawn.t0, spawn.t1, radius);

  bot.GetGame().SendChatMessage("?chat=marvin");

  std::string name = Lowercase(bot.GetGame().GetPlayer().name);
  uint16_t ship = bot.GetGame().GetPlayer().ship;

    if (name == "lilmarv" && ship == 8) {
      ship = 1;
    }

    std::vector<MapCoord> patrol_nodes = {MapCoord(568, 568), MapCoord(454, 568), MapCoord(454, 454),
                                          MapCoord(568, 454), MapCoord(568, 568), MapCoord(454, 454),
                                          MapCoord(568, 454), MapCoord(454, 568), MapCoord(454, 454),
                                          MapCoord(568, 568), MapCoord(454, 568), MapCoord(568, 454)};

    bot.GetBlackboard().Set<bool>(BBKey::UseMultiFire, false);
    bot.GetBlackboard().Set<bool>(BBKey::UseStealth, false);
    bot.GetBlackboard().Set<bool>(BBKey::UseCloak, false);
    bot.GetBlackboard().Set<bool>(BBKey::UseXRadar, true);
    bot.GetBlackboard().Set<bool>(BBKey::UseDecoy, true);
    bot.GetBlackboard().Set<bool>(BBKey::UseRepel, false);
    bot.GetBlackboard().Set<bool>(BBKey::UseBurst, true);
    bot.GetBlackboard().SetDefaultValue<bool>(BBKey::UseMultiFire, false);
    bot.GetBlackboard().SetDefaultValue<bool>(BBKey::UseStealth, false);
    bot.GetBlackboard().SetDefaultValue<bool>(BBKey::UseCloak, false);
    bot.GetBlackboard().SetDefaultValue<bool>(BBKey::UseXRadar, true);
    bot.GetBlackboard().SetDefaultValue<bool>(BBKey::UseDecoy, true);
    bot.GetBlackboard().SetDefaultValue<bool>(BBKey::UseRepel, false);
    bot.GetBlackboard().SetDefaultValue<bool>(BBKey::UseBurst, true);

    bot.GetBlackboard().Set<std::vector<MapCoord>>(BBKey::PatrolNodes, patrol_nodes);
    bot.GetBlackboard().Set<uint16_t>(BBKey::PubEventTeam0, 00);
    bot.GetBlackboard().Set<uint16_t>(BBKey::PubEventTeam1, 01);
    bot.GetBlackboard().Set<uint16_t>(BBKey::RequestedShip, ship);
    bot.GetBlackboard().Set<Vector2f>(BBKey::CenterSpawnPoint, Vector2f(512, 512));
    bot.GetBlackboard().Set<RequestedCommand>(BBKey::RequestedCommand, RequestedCommand::ShipChange);

  
  
    // behavior nodes
  auto failure = std::make_unique<bot::FailureNode>();
  auto DEVA_debug = std::make_unique<deva::DevaDebugNode>();
  auto is_anchor = std::make_unique<bot::IsAnchorNode>();
  auto not_in_center = std::make_unique<bot::NotInCenterNode>();
  auto cast_weapon_influence = std::make_unique<bot::CastWeaponInfluenceNode>();
  auto bouncing_shot = std::make_unique<bot::BouncingShotNode>();
  auto DEVA_repel_enemy = std::make_unique<deva::DevaRepelEnemyNode>();
  auto DEVA_burst_enemy = std::make_unique<deva::DevaBurstEnemyNode>();
  auto patrol_base = std::make_unique<deva::DevaPatrolBaseNode>();
  auto patrol_center = std::make_unique<bot::PatrolNode>();
  auto DEVA_move_to_enemy = std::make_unique<deva::DevaMoveToEnemyNode>();
  auto anchor_base_path = std::make_unique<bot::AnchorBasePathNode>();
  auto rusher_base_path = std::make_unique<bot::RusherBasePathNode>();

  auto find_enemy_in_base = std::make_unique<bot::FindEnemyInBaseNode>();

  auto DEVA_toggle_status = std::make_unique<deva::DevaToggleStatusNode>();
  auto DEVA_attach = std::make_unique<deva::DevaAttachNode>();
  auto DEVA_warp = std::make_unique<deva::DevaWarpNode>();
  auto DEVA_freqman = std::make_unique<deva::DevaFreqMan>();
  auto team_sort = std::make_unique<bot::SortBaseTeams>();
  auto DEVA_set_region = std::make_unique<deva::DevaSetRegionNode>();



  // logic nodes
  auto move_method_selector = std::make_unique<behavior::SelectorNode>(DEVA_move_to_enemy.get());
  auto los_weapon_selector =
      std::make_unique<behavior::SelectorNode>(DEVA_burst_enemy.get(), DEVA_repel_enemy.get(), shoot_enemy_.get());
  auto parallel_shoot_enemy =
      std::make_unique<behavior::ParallelAnyNode>(los_weapon_selector.get(), move_method_selector.get());

  auto path_to_enemy_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy_.get(), follow_path_.get());
  auto rusher_base_path_sequence = std::make_unique<behavior::SequenceNode>(rusher_base_path.get(), follow_path_.get());
  auto anchor_base_path_sequence = std::make_unique<behavior::SequenceNode>(anchor_base_path.get(), follow_path_.get());

  auto enemy_path_logic_selector = std::make_unique<behavior::SelectorNode>(mine_sweeper_.get(), rusher_base_path_sequence.get(),
                                               anchor_base_path_sequence.get(), path_to_enemy_sequence.get());

  auto anchor_los_parallel = std::make_unique<behavior::ParallelAnyNode>(DEVA_burst_enemy.get(), mine_sweeper_.get(),
                                                                      anchor_base_path_sequence.get());
  auto anchor_los_sequence =
      std::make_unique<behavior::SequenceNode>(is_anchor.get(), not_in_center.get(), anchor_los_parallel.get());
  auto los_role_selector =
      std::make_unique<behavior::SelectorNode>(anchor_los_sequence.get(), parallel_shoot_enemy.get());

  auto los_shoot_conditional =
      std::make_unique<behavior::SequenceNode>(target_in_los_.get(), los_role_selector.get());
  auto bounce_path_parallel =
      std::make_unique<behavior::ParallelAnyNode>(bouncing_shot.get(), enemy_path_logic_selector.get());

  auto path_or_shoot_selector = std::make_unique<behavior::SelectorNode>(
      DEVA_repel_enemy.get(), los_shoot_conditional.get(), bounce_path_parallel.get());

  auto find_enemy_in_base_sequence =
      std::make_unique<behavior::SequenceNode>(find_enemy_in_base.get(), path_or_shoot_selector.get());
  auto find_enemy_in_center_sequence =
      std::make_unique<behavior::SequenceNode>(find_enemy_in_center_.get(), path_or_shoot_selector.get());
  auto find_enemy_selector =
      std::make_unique<behavior::SelectorNode>(find_enemy_in_center_sequence.get(), find_enemy_in_base_sequence.get());

  auto patrol_base_sequence = std::make_unique<behavior::SequenceNode>(patrol_base.get(), follow_path_.get());
  auto patrol_center_sequence = std::make_unique<behavior::SequenceNode>(patrol_center.get(), follow_path_.get());
  auto patrol_selector =
      std::make_unique<behavior::SelectorNode>(patrol_center_sequence.get(), patrol_base_sequence.get());

  auto enemy_or_patrol_selector = std::make_unique<behavior::SelectorNode>(find_enemy_selector.get(), patrol_selector.get());

  auto anchor_sequence = std::make_unique<behavior::SequenceNode>(is_anchor.get(), not_in_center.get(), cast_weapon_influence.get(),
                                                                  failure.get());
  auto role_selector = std::make_unique<behavior::SelectorNode>(anchor_sequence.get(), enemy_or_patrol_selector.get());
  auto root_sequence = std::make_unique<behavior::SequenceNode>(
      DEVA_debug.get(), commands_.get(), spectator_query_.get(), team_sort.get(),
      DEVA_set_region.get(), DEVA_freqman.get(), DEVA_attach.get(), respawn_query_.get(), DEVA_warp.get(),
      DEVA_toggle_status.get(), role_selector.get());

  engine_->PushRoot(std::move(root_sequence));

  engine_->PushNode(std::move(move_method_selector));
  engine_->PushNode(std::move(los_weapon_selector));
  engine_->PushNode(std::move(parallel_shoot_enemy));
  engine_->PushNode(std::move(los_shoot_conditional));
  engine_->PushNode(std::move(path_to_enemy_sequence));
  engine_->PushNode(std::move(path_or_shoot_selector));
  engine_->PushNode(std::move(DEVA_move_to_enemy));
  engine_->PushNode(std::move(DEVA_repel_enemy));
  engine_->PushNode(std::move(DEVA_burst_enemy));
  engine_->PushNode(std::move(is_anchor));
  engine_->PushNode(std::move(failure));
  engine_->PushNode(std::move(not_in_center));
  engine_->PushNode(std::move(anchor_los_parallel));
  engine_->PushNode(std::move(anchor_los_sequence));
  engine_->PushNode(std::move(los_role_selector));
  engine_->PushNode(std::move(bouncing_shot));
  engine_->PushNode(std::move(bounce_path_parallel));
  engine_->PushNode(std::move(anchor_base_path));
  engine_->PushNode(std::move(rusher_base_path));
  engine_->PushNode(std::move(anchor_base_path_sequence));
  engine_->PushNode(std::move(rusher_base_path_sequence));
  engine_->PushNode(std::move(enemy_path_logic_selector));
  engine_->PushNode(std::move(find_enemy_in_base));
  engine_->PushNode(std::move(find_enemy_in_base_sequence));
  engine_->PushNode(std::move(find_enemy_in_center_sequence));
  engine_->PushNode(std::move(find_enemy_selector));
  engine_->PushNode(std::move(patrol_base));
  engine_->PushNode(std::move(patrol_center));
  engine_->PushNode(std::move(patrol_base_sequence));
  engine_->PushNode(std::move(patrol_center_sequence));
  engine_->PushNode(std::move(patrol_selector));
  engine_->PushNode(std::move(DEVA_toggle_status));
  engine_->PushNode(std::move(cast_weapon_influence));
  engine_->PushNode(std::move(DEVA_attach));
  engine_->PushNode(std::move(DEVA_warp));
  engine_->PushNode(std::move(DEVA_freqman));
  engine_->PushNode(std::move(team_sort));
  engine_->PushNode(std::move(DEVA_set_region));
  engine_->PushNode(std::move(DEVA_debug));
  engine_->PushNode(std::move(enemy_or_patrol_selector));
  engine_->PushNode(std::move(anchor_sequence));
  engine_->PushNode(std::move(role_selector));
}

behavior::ExecuteResult DevaDebugNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& bb = ctx.bot->GetBlackboard();
  auto& game = ctx.bot->GetGame();

    #if DEBUG_RENDER_FIND_ENEMY_IN_BASE_NODE
  const Player* target = bb.ValueOr<const Player*>("Target", nullptr);
  Path base_path = ctx.bot->GetBasePath();

  if (target) {
    float r = game.GetShipSettings(target->ship).GetRadius();

    Vector2f box_start = target->position - Vector2f(r, r);
    Vector2f box_end = target->position + Vector2f(r, r);

    RenderWorldBox(game.GetPosition(), box_start, box_end, RGB(255, 0, 0));

    //box_start = base_path[target_node] - Vector2f(r, r);
    //box_end = base_path[target_node] + Vector2f(r, r);

    //RenderWorldBox(game.GetPosition(), box_start, box_end, RGB(0, 255, 0));
  }
#endif

  #if DEBUG_DISABLE_BEHAVIOR
  g_RenderState.RenderDebugText("  DevaDebugNode(fail): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  #endif
    g_RenderState.RenderDebugText("  DevaDebugNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
}


behavior::ExecuteResult DevaSetRegionNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  const std::vector<TeamGoals>& coords = ctx.bot->GetGoals().GetTeamGoals();

  if (coords.empty()) {
    g_RenderState.RenderDebugText("  DevaSetRegionNode(empty): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  int highest_count = 0;
  std::size_t active_base = 0;


  // get base with highest player count
  for (std::size_t i = 0; i < coords.size(); i++) {
    const TeamGoals& pair = coords[i];
    int count = 0;
    for (const Player& player : game.GetPlayers()) {
      if ((player.frequency != 00 && player.frequency != 01) || player.ship > 7) continue;
      if (ctx.bot->GetRegions().IsConnected(player.position, pair.t0)) {
        count++;
      }
    }

    if (count > highest_count) {
      highest_count = count;
      active_base = i;
    }
  }

  ctx.bot->GetBasePaths().SetBase(active_base);

  MapCoord t0 = coords[active_base].t0;
  MapCoord t1 = coords[active_base].t1;

  if (game.GetPlayer().frequency == 0) {
    bb.Set<MapCoord>(BBKey::TeamGuardPoint, t0);
    bb.Set<MapCoord>(BBKey::EnemyGuardPoint, t1);
  }
  else if (game.GetPlayer().frequency == 1) {
    bb.Set<MapCoord>(BBKey::TeamGuardPoint, t1);
    bb.Set<MapCoord>(BBKey::EnemyGuardPoint, t0);
  }

  g_RenderState.RenderDebugText("  DevaSetRegionNode(completed): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}


behavior::ExecuteResult DevaFreqMan::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  bool dueling = game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01;
  //std::vector<uint16_t> team_mates = GetTeamMates(ctx);
  //uint64_t offset = ctx.bot->GetTime().UniqueIDTimer(ctx.bot->GetGame(), team_mates);
  //g_RenderState.RenderDebugText("BOT OFFSET:  %llu", offset);
  uint16_t test = 1000;
  uint64_t max_offset = kBotNames.size() * 200;
  //const std::vector<uint16_t>& fList = bb.GetFreqList();
  std::vector<uint16_t> fList = bb.ValueOr<std::vector<uint16_t>>(BBKey::FreqPopulationCount, std::vector<uint16_t>{});
  CombatRole role = bb.ValueOr<CombatRole>(BBKey::CombatRole, CombatRole::Rusher);

  bool frequency_locked = bb.ValueOr<bool>(BBKey::FrequencyLocked, false);

  //if (bb.ValueOr<bool>("FreqLock", false) || (bb.ValueOr<bool>("IsAnchor", false) && dueling)) {
  if (frequency_locked || (role == CombatRole::Anchor && dueling)) {
    g_RenderState.RenderDebugText("  DevaFreqMan:(locked/anchoring) %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  // do this before looking at joining a dueling team
  if (ShouldLeaveCrowdedFreq(ctx)) {
    std::vector<uint16_t> team_mates = GetTeamMates(ctx);
    uint64_t offset = ctx.bot->GetTime().UniqueIDTimer(ctx.bot->GetGame(), team_mates);

   // game.SendChatMessage("My timer was to leave a crowded freq: " + std::to_string(offset));
 
    if (ctx.bot->GetTime().TimedActionDelay("leave_crowded", offset)) {
   //   game.SendChatMessage("My timer was triggered: " + std::to_string(offset));
      game.SetFreq(FindOpenFreq(fList, 2));
    }
    g_RenderState.RenderDebugText("  DevaFreqMan:(Find Open Freq) %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  if (ShouldJoinTeam0(ctx)) {
    std::vector<uint16_t> joiners = GetDuelTeamJoiners(ctx);
    uint64_t offset = ctx.bot->GetTime().UniqueIDTimer(ctx.bot->GetGame(), joiners);

     // game.SendChatMessage("My timer to join team 0: " + std::to_string(offset));

    if (ctx.bot->GetTime().TimedActionDelay("join0", offset)) {
    //  game.SendChatMessage("My timer was triggered: " + std::to_string(offset));
      game.SetFreq(0);
    }
    g_RenderState.RenderDebugText("  DevaFreqMan:(Join Team 0) %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  if (ShouldJoinTeam1(ctx)) {
    std::vector<uint16_t> joiners = GetDuelTeamJoiners(ctx);
    uint64_t offset = ctx.bot->GetTime().UniqueIDTimer(ctx.bot->GetGame(), joiners);

   // game.SendChatMessage("My timer to join team 1: " + std::to_string(offset));

    if (ctx.bot->GetTime().TimedActionDelay("join1", offset)) {
   //   game.SendChatMessage("My timer was triggered: " + std::to_string(offset));
      game.SetFreq(1);
    }
    g_RenderState.RenderDebugText("  DevaFreqMan:(Join Team 1) %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  if (ShouldLeaveDuelTeam(ctx)) {
    std::vector<uint16_t> team_mates = GetTeamMates(ctx);
    uint64_t offset = ctx.bot->GetTime().UniqueIDTimer(ctx.bot->GetGame(), team_mates);
    std::vector<uint16_t> joiners = GetDuelTeamJoiners(ctx);
    if (!joiners.empty()) offset += max_offset + 500;  // add time for shipresets


   // game.SendChatMessage("My timer to leave the duel team: " + std::to_string(offset));


    if (ctx.bot->GetTime().TimedActionDelay("leaveduelteam", offset)) {
     // game.SendChatMessage("My timer was triggered: " + std::to_string(offset));
      game.SetFreq(FindOpenFreq(fList, 2));
    }
    g_RenderState.RenderDebugText("  DevaFreqMan:(Leave Duel Team) %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }





  #if 0
  uint64_t offset = ctx.bot->GetTime().DevaFreqTimer(ctx.bot->GetGame(), kBotNames);
  //uint64_t offset = ctx.bot->GetTime().UniqueIDTimer(ctx.bot->GetGame(), game.GetPlayer().id);

  //std::vector<uint16_t> fList = bb.ValueOr<std::vector<uint16_t>>("FreqList", std::vector<uint16_t>());
  const std::vector<uint16_t>& fList = bb.GetFreqList();

  // if player is on a non duel team size greater than 2, breaks the zone balancer
  if (game.GetPlayer().frequency != 00 && game.GetPlayer().frequency != 01) {
    if (fList[game.GetPlayer().frequency] > 1) {
      g_RenderState.RenderDebugText("  FList 0: %i", fList[game.GetPlayer().frequency]);
      if (ctx.bot->GetTime().TimedActionDelay("get_off_large_freq", offset)) {
        game.SetFreq(FindOpenFreq(fList, 2));
      }

      g_RenderState.RenderDebugText("  DevaFreqMan:(jump from oversized team) %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    }
  }

  // duel team is uneven
  if (fList[0] != fList[1]) {
    if (!dueling) {
      if (ctx.bot->GetTime().TimedActionDelay("join_duel_team", offset)) {
        if (fList[0] < fList[1]) {
          game.SetFreq(0);
        } else {
          // get on freq 1
          game.SetFreq(1);
        }
      }
      g_RenderState.RenderDebugText("  DevaFreqMan:(jump onto dueling team) %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
      // } else if (bb.ValueOr<bool>("InCenter", true) ||
    } else if (bb.GetInCenter()) {  // bot is on a duel team
      // bb.ValueOr<std::vector<Player>>("TeamList", std::vector<Player>()).size() == 0 ||
      // bb.ValueOr<bool>("TeamInBase", false)) {  // bot is on a duel team

      if ((game.GetPlayer().frequency == 00 && fList[0] > fList[1]) ||
          (game.GetPlayer().frequency == 01 && fList[0] < fList[1])) {
          #if 0
        // look for players not on duel team, make bot wait longer to see if the other player will get in
        for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
          const Player& player = game.GetPlayers()[i];

          if (player.frequency > 01 && player.frequency < 100) {
            float energy_pct = player.energy / game.GetShipSettings(player.ship).MaximumEnergy * 100.0f;
            if (energy_pct < 100.0f) {
              g_RenderState.RenderDebugText("  DevaFreqMan:(wait for other players) %llu", timer.GetElapsedTime());
              return behavior::ExecuteResult::Failure;
            }
          }
        }
        #endif
        if (ctx.bot->GetTime().TimedActionDelay("get_off_duel_team", offset)) {
          game.SetFreq(FindOpenFreq(fList, 2));
        }

        g_RenderState.RenderDebugText("  FList 0: %i", fList[0]);
        g_RenderState.RenderDebugText("  Flist 1: %i", fList[1]);
        g_RenderState.RenderDebugText("  DevaFreqMan:(jump off dueling team) %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Failure;
      }
    }
  }
  #endif

  g_RenderState.RenderDebugText("  DevaFreqMan:(success) %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

bool DevaFreqMan::ShouldJoinTeam0(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  bool dueling = game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01;
  //const std::vector<uint16_t>& fList = bb.GetFreqList();
  std::vector<uint16_t> fList = bb.ValueOr<std::vector<uint16_t>>(BBKey::FreqPopulationCount, std::vector<uint16_t>{});

  if (fList[0] < fList[1] && !dueling) {
      return true;
  }
 // g_RenderState.RenderDebugText("  DevaFreqMan:(Not Chosen) %llu", timer.GetElapsedTime());
  return false;
}

bool DevaFreqMan::ShouldJoinTeam1(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  bool dueling = game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01;
  //const std::vector<uint16_t>& fList = bb.GetFreqList();
  std::vector<uint16_t> fList = bb.ValueOr<std::vector<uint16_t>>(BBKey::FreqPopulationCount, std::vector<uint16_t>{});

  if (fList[1] < fList[0] && !dueling) {
      return true;
  }
  return false;
}

bool DevaFreqMan::ShouldLeaveDuelTeam(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  //const std::vector<uint16_t>& fList = bb.GetFreqList();
  std::vector<uint16_t> fList = bb.ValueOr<std::vector<uint16_t>>(BBKey::FreqPopulationCount, std::vector<uint16_t>{});

  uint16_t freq = game.GetPlayer().frequency;
  bool should_leave = (fList[0] > fList[1] && freq == 00) || (fList[1] > fList[0] && freq == 01);

  if (should_leave) {
      return true;
  }
  return false;
}

bool DevaFreqMan::ShouldLeaveCrowdedFreq(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  //const std::vector<uint16_t>& fList = bb.GetFreqList();
  std::vector<uint16_t> fList = bb.ValueOr<std::vector<uint16_t>>(BBKey::FreqPopulationCount, std::vector<uint16_t>{});

  bool dueling = game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01;
  bool crowded = fList[game.GetPlayer().frequency] > 1;

  if (crowded && !dueling) {
      return true;
  }
  return false;
}

#if 0
bool DevaFreqMan::ShouldJoinTeam0(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  bool dueling = game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01;
  const std::vector<uint16_t>& fList = bb.GetFreqList();

  if (fList[0] < fList[1] && !dueling) {
    g_RenderState.RenderDebugText("  DevaFreqMan:(Should Join Team 0) %llu", timer.GetElapsedTime());
    std::vector<uint16_t> joiners = GetDuelTeamJoiners(ctx);
    uint16_t chosen = GetLowestID(ctx, joiners);

    if (chosen == game.GetPlayer().id) {
      g_RenderState.RenderDebugText("  DevaFreqMan:(Chosen) %llu", timer.GetElapsedTime());
      return true;
    }
  }
  g_RenderState.RenderDebugText("  DevaFreqMan:(Not Chosen) %llu", timer.GetElapsedTime());
  return false;
}

bool DevaFreqMan::ShouldJoinTeam1(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  bool dueling = game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01;
  const std::vector<uint16_t>& fList = bb.GetFreqList();

  if (fList[1] < fList[0] && !dueling) {
    std::vector<uint16_t> joiners = GetDuelTeamJoiners(ctx);
    uint16_t chosen = GetLowestID(ctx, joiners);

    if (chosen == game.GetPlayer().id) {
      return true;
    }
  }
  return false;
}

bool DevaFreqMan::ShouldLeaveDuelTeam(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  const std::vector<uint16_t>& fList = bb.GetFreqList();

  uint16_t freq = game.GetPlayer().frequency;
  bool should_leave = (fList[0] > fList[1] && freq == 00) || (fList[1] > fList[0] && freq == 01);

  if (should_leave) {
    std::vector<uint16_t> joiners = GetDuelTeamJoiners(ctx);

    if (!joiners.empty()) return false;

    std::vector<uint16_t> team_mates = GetDuelTeamMates(ctx);

    uint16_t chosen = GetLowestID(ctx, team_mates);

    if (chosen == game.GetPlayer().id) {
      return true;
    }
  }
  return false;
}

bool DevaFreqMan::ShouldLeaveCrowdedFreq(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  const std::vector<uint16_t>& fList = bb.GetFreqList();

  bool dueling = game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01;
  bool crowded = fList[game.GetPlayer().frequency] > 1;

  if (crowded && !dueling) {
    std::vector<uint16_t> team_mates = GetTeamMates(ctx);
    uint16_t chosen = GetLowestID(ctx, team_mates);

    if (chosen == game.GetPlayer().id) {
      return true;
    }
  }
  return false;
}

#endif

// get a list of bots that also want to join
std::vector<uint16_t> DevaFreqMan::GetDuelTeamJoiners(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  std::vector<uint16_t> list;

  for (std::size_t i = 0; i < kBotNames.size(); i++) {
    const Player* player = game.GetPlayerByName(kBotNames[i]);
    if (!player) continue;
    
    bool dueling = player->frequency == 00 || player->frequency == 01;

    if (!dueling && player->ship < 8) {
      list.push_back(player->id);
    }
  }
  return list;
}

std::vector<uint16_t> DevaFreqMan::GetTeamMates(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  std::vector<uint16_t> list;

  for (std::size_t i = 0; i < kBotNames.size(); i++) {
    const Player* player = game.GetPlayerByName(kBotNames[i]);
    if (!player) continue;

    if (player->frequency == game.GetPlayer().frequency) {
      list.push_back(player->id);
     // g_RenderState.RenderDebugText("Team Mate Found: " + kBotNames[i]);
    }
  }

  return list;
}

// a list of dueling team mates that can leave without disrupting the game
std::vector<uint16_t> DevaFreqMan::GetDuelTeamMates(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  std::vector<uint16_t> list;

  for (std::size_t i = 0; i < kBotNames.size(); i++) {
    const Player* player = game.GetPlayerByName(kBotNames[i]);
    if (!player) continue;

    bool in_center = ctx.bot->GetRegions().IsConnected(Vector2f(512, 512), player->position);

    if (player->frequency == game.GetPlayer().frequency && in_center) {
      list.push_back(player->id);
    }
  }

  return list;
}

uint16_t DevaFreqMan::GetLowestID(behavior::ExecuteContext& ctx, std::vector<uint16_t> IDs) {

  if (IDs.empty()) return ctx.bot->GetGame().GetPlayer().id;

  uint16_t lowest = IDs[0];

  for (std::size_t i = 0; i < IDs.size(); i++) {
    if (IDs[i] < lowest) {
      lowest = IDs[i];
    }
  }

  return lowest;
}

uint16_t DevaFreqMan::ChooseDueler(behavior::ExecuteContext& ctx, std::vector<uint16_t> IDs) {

  uint16_t lowest = ctx.bot->GetGame().GetPlayer().id;
  uint16_t last = GetLastPlayerInBase(ctx);

  if (lowest == last) lowest = 0xFFFF;

  for (std::size_t i = 0; i < IDs.size(); i++) {
    if (IDs[i] < lowest && IDs[i] != last) {
      lowest = IDs[i];
    }
  }

  return lowest;
}

uint16_t DevaFreqMan::GetLastPlayerInBase(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  Path base_path = ctx.bot->GetBasePath();

  if (base_path.empty()) {
    return 0xFFFF;
  }

  int count = 0;
  uint16_t id = 0xFFFF;

  for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
    const Player& player = game.GetPlayers()[i];

    bool in_base = ctx.bot->GetRegions().IsConnected(player.position, base_path[0]);

    if (in_base && !player.dead) {
      count++;
      id = player.id;
    }

    if (count > 1) return 0xFFFF;
  }

  return id;
}

behavior::ExecuteResult DevaWarpNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  auto& time = ctx.bot->GetTime();

  Vector2f center_spawn = bb.ValueOr<Vector2f>(BBKey::CenterSpawnPoint, Vector2f(512, 512));
  bool in_center = ctx.bot->GetRegions().IsConnected(game.GetPosition(), center_spawn);

  uint64_t base_empty_timer = 30000;
  bool enemy_in_base = bb.ValueOr<bool>(BBKey::EnemyInBase, false);

  //if (!bb.ValueOr<bool>("InCenter", true)) {
  if (!in_center) {
    //if (!bb.ValueOr<bool>("EnemyInBase", false)) {
    if (!enemy_in_base) {
      if (time.TimedActionDelay("baseemptywarp", base_empty_timer)) {
        game.Warp();
      }
    }
  }

  g_RenderState.RenderDebugText("  DevaWarpNode:(Enemy Not In Base) %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult DevaAttachNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  Vector2f center_spawn = bb.ValueOr<Vector2f>(BBKey::CenterSpawnPoint, Vector2f(512, 512));
  bool in_center = ctx.bot->GetRegions().IsConnected(game.GetPosition(), center_spawn);
  bool enemy_in_base = bb.ValueOr<bool>(BBKey::EnemyInBase, false);
  bool team_in_base = bb.ValueOr<bool>(BBKey::TeamInBase, false);
  bool swarm = bb.ValueOr<bool>(BBKey::ActivateSwarm, false);


  // if dueling then run checks for attaching
  if (game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01) {
    //if (bb.ValueOr<bool>("TeamInBase", false)) {
    if (team_in_base && enemy_in_base) {

      //if (bb.ValueOr<bool>("Swarm", false)) {
      if (swarm) {
        if (game.GetPlayer().dead) {
          if (ctx.bot->GetTime().TimedActionDelay("respawn", 800)) {
            game.ResetShip();
          }

          g_RenderState.RenderDebugText("  DevaAttachNode:(Swarming) %llu", timer.GetElapsedTime());
          return behavior::ExecuteResult::Failure;
        }
      }

      // if this check is valid the bot will halt here untill it attaches
      //if (bb.ValueOr<bool>("InCenter", true)) {
      if (in_center && !game.GetPlayer().dead) {

        SetAttachTarget(ctx);
        game.Attach();
        
        g_RenderState.RenderDebugText("  DevaAttachNode:(Attaching) %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Failure;
      }
    }
  }

  // if attached to a player detach
  if (game.GetPlayer().attach_id != 65535) {
    //if (bb.ValueOr<bool>("Swarm", false)) {
    bool swarm = bb.ValueOr<bool>(BBKey::ActivateSwarm, false);
    if (swarm) {
      game.SetEnergy(15, "swarm");
    }

    game.SendKey(VK_F7);

    g_RenderState.RenderDebugText("  DevaAttachNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  g_RenderState.RenderDebugText("  DevaAttachNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

void DevaAttachNode::SetAttachTarget(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  auto& pf = ctx.bot->GetPathfinder();

  const std::vector<Vector2f>& path = ctx.bot->GetBasePath();
  auto ns = path::PathNodeSearch::Create(*ctx.bot, path);

  std::vector<const Player*> team_list = bb.ValueOr<std::vector<const Player*>>(BBKey::TeamPlayerList, std::vector<const Player*>());
  //std::vector<const Player*> combined_list = bb.ValueOr<std::vector<const Player*>>(BBKey::EnemyTeamPlayerList, std::vector<const Player*>());
  //const std::vector<const Player*>& team_list = bb.GetTeamList();
  //const std::vector<const Player*>& combined_list = bb.GetCombinedList();

  CombatRole role = bb.ValueOr<CombatRole>(BBKey::CombatRole, CombatRole::Rusher);


   if (path.empty()) {
    if (!team_list.empty()) {
      game.SetSelectedPlayer(team_list[0]->id);
    }
    return;
   }

  float closest_distance = std::numeric_limits<float>::max();
  float closest_team_distance_to_team = std::numeric_limits<float>::max();
  float closest_team_distance_to_enemy = std::numeric_limits<float>::max();
  float closest_enemy_distance_to_team = std::numeric_limits<float>::max();
  float closest_enemy_distance_to_enemy = std::numeric_limits<float>::max();

  bool enemy_leak = false;
  bool team_leak = false;

  uint16_t closest_team_to_team = 0;
  Vector2f closest_enemy_to_team;
  uint16_t closest_team_to_enemy = 0;
  uint16_t closest_enemy_to_enemy = 0;

  // find closest player to team and enemy safe
  for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
    const Player* player = &game.GetPlayers()[i];

    if ((player->frequency != 00 && player->frequency != 01) || player->ship > 7) continue; 

    // bool player_in_center = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));
    bool player_in_base = ctx.bot->GetRegions().IsConnected(player->position, path[0]);

    if (player_in_base) {
      // if (!player_in_center && IsValidPosition(player.position) && player.ship < 8) {

      float distance_to_team = ns->GetPathDistance(player->position, bb.ValueOr<MapCoord>(BBKey::TeamGuardPoint, MapCoord()));
      float distance_to_enemy = ns->GetPathDistance(player->position, bb.ValueOr<MapCoord>(BBKey::EnemyGuardPoint, MapCoord()));
      //float distance_to_team = ns->GetPathDistance(player->position, bb.GetTeamSafe());
      //float distance_to_enemy = ns->GetPathDistance(player->position, bb.GetEnemySafe());

      if (player->frequency == game.GetPlayer().frequency) {
        // float distance_to_team = ctx.deva->PathLength(player.position, ctx.deva->GetTeamSafe());
        // float distance_to_enemy = ctx.deva->PathLength(player.position, ctx.deva->GetEnemySafe());

        if (distance_to_team < closest_team_distance_to_team) {
          closest_team_distance_to_team = distance_to_team;
          //closest_team_to_team = bb.ValueOr<std::vector<Player>>("CombinedList", std::vector<Player>())[i].id;
          closest_team_to_team = player->id;
        }
        if (distance_to_enemy < closest_team_distance_to_enemy) {
          closest_team_distance_to_enemy = distance_to_enemy;
          //closest_team_to_enemy = bb.ValueOr<std::vector<Player>>("CombinedList", std::vector<Player>())[i].id;
          closest_team_to_enemy = player->id;
        }
      } else {
        // float distance_to_team = ctx.deva->PathLength(player.position, ctx.deva->GetTeamSafe());
        // float distance_to_enemy = ctx.deva->PathLength(player.position, ctx.deva->GetEnemySafe());

        if (distance_to_team < closest_enemy_distance_to_team) {
          closest_enemy_distance_to_team = distance_to_team;
          closest_enemy_to_team = player->position;
        }
        if (distance_to_enemy < closest_enemy_distance_to_enemy) {
          closest_enemy_distance_to_enemy = distance_to_enemy;
          //closest_enemy_to_enemy = bb.ValueOr<std::vector<Player>>("CombinedList", std::vector<Player>())[i].id;
          closest_enemy_to_enemy = player->id;
        }
      }
    }
  }

  if (closest_team_distance_to_team > closest_enemy_distance_to_team) {
    enemy_leak = true;
  }

  if (closest_enemy_distance_to_enemy > closest_team_distance_to_enemy) {
    team_leak = true;
  }

 // if (team_leak || enemy_leak || bb.ValueOr<bool>("IsAnchor", false)) {
  if (team_leak || enemy_leak || role == CombatRole::Anchor) {
    game.SetSelectedPlayer(closest_team_to_team);
    return;
  }

  // find closest team player to the enemy closest to team safe
  for (std::size_t i = 0; i < team_list.size(); i++) {
    const Player* player = team_list[i];

    // bool player_in_center = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));

    bool player_in_base = ctx.bot->GetRegions().IsConnected(player->position, path[0]);
    bool allow_lag_attaching = bb.ValueOr<bool>(BBKey::AllowLagAttaching, true);

    if ((allow_lag_attaching && player_in_base) || (player_in_base && !player->dead)) {
      // as long as the player is not in center and has a valid position, it will hold its position for a moment after
      // death if (!player_in_center && IsValidPosition(player.position) && player.ship < 8) {

      // float distance_to_enemy_position = ctx.deva->PathLength(player.position, closest_enemy_to_team);

      float distance_to_enemy_position = 0.0f;

      distance_to_enemy_position = ns->GetPathDistance(player->position, closest_enemy_to_team);

      // get the closest player
      if (distance_to_enemy_position < closest_distance) {
        closest_distance = distance_to_enemy_position;
        game.SetSelectedPlayer(player->id);
      }
    }
  }
}

behavior::ExecuteResult DevaToggleStatusNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  uint8_t status = game.GetPlayer().status;

  bool x_active = (status & Status_XRadar) != 0;
  bool stealthing = (status & Status_Stealth) != 0;
  bool cloaking = (status & Status_Cloak) != 0;
  bool multi_on = game.GetPlayer().multifire_status;

  bool has_xradar = game.GetPlayer().capability.xradar;
  bool has_stealth = game.GetPlayer().capability.stealth;
  bool has_cloak = game.GetPlayer().capability.cloak;

  bool use_xradar = bb.ValueOr<bool>(BBKey::UseXRadar, true);
  bool use_stealth = bb.ValueOr<bool>(BBKey::UseStealth, false);
  bool use_cloak = bb.ValueOr<bool>(BBKey::UseCloak, false);
  bool use_multi = bb.ValueOr<bool>(BBKey::UseMultiFire, false);

  if (multi_on && !use_multi || !multi_on && use_multi) {
      game.MultiFire();

      g_RenderState.RenderDebugText("  DevaToggleStatusNode: %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
  }

  // in deva these are free so just turn them on
  if (has_xradar) {
    if (!x_active && use_xradar || x_active && !use_xradar) {
      if (ctx.bot->GetTime().TimedActionDelay("xradar", 800)) {
        game.XRadar();

        g_RenderState.RenderDebugText("  DevaToggleStatusNode(toggle xradar): %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Failure;
      }

      g_RenderState.RenderDebugText("  DevaToggleStatusNode(setting xradar): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Success;
    }
  }

  if (has_stealth) {
    if (!stealthing && use_stealth || stealthing && !use_stealth) {
      if (ctx.bot->GetTime().TimedActionDelay("stealth", 800)) {
        game.Stealth();

        g_RenderState.RenderDebugText("  DevaToggleStatusNode: %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Failure;
      }

      return behavior::ExecuteResult::Success;
    }
  }
  if (has_cloak) {
    if (!cloaking && use_cloak || cloaking && !use_cloak) {
      if (ctx.bot->GetTime().TimedActionDelay("cloak", 800)) {
        game.Cloak(ctx.bot->GetKeys());

        g_RenderState.RenderDebugText("  DevaToggleStatusNode: %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Failure;
      }

      g_RenderState.RenderDebugText("  DevaToggleStatusNode: %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Success;
    }
  }

  g_RenderState.RenderDebugText("  DevaToggleStatusNode(No Action Taken): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult DevaPatrolBaseNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  bool last_in_base = bb.ValueOr<bool>(BBKey::LastInBase, false);
  //bool last_in_base = bb.GetLastInBase();
  //MapCoord enemy_safe = bb.GetEnemySafe();
  //MapCoord team_safe = bb.GetTeamSafe();
  MapCoord team_safe = bb.ValueOr<MapCoord>(BBKey::TeamGuardPoint, MapCoord());
  MapCoord enemy_safe = bb.ValueOr<MapCoord>(BBKey::EnemyGuardPoint, MapCoord());
  bool on_safe_tile = game.GetMap().GetTileId(game.GetPlayer().position) == kSafeTileId;

  if (on_safe_tile && game.GetPosition().Distance(enemy_safe) <= 5.0f) {
    ctx.bot->GetKeys().Press(VK_CONTROL);
    ctx.bot->GetPathfinder().ClearPath();
    g_RenderState.RenderDebugText("  DevaPatrolBaseNode:(On Enemy Safe) %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  if (last_in_base) {
    ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPosition(), team_safe, game.GetShipSettings().GetRadius());
  } else {
    ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPosition(), enemy_safe, game.GetShipSettings().GetRadius());
  }

  g_RenderState.RenderDebugText("  DevaPatrolBaseNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult DevaRepelEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  Vector2f center_spawn = bb.ValueOr<Vector2f>(BBKey::CenterSpawnPoint, Vector2f(512, 512));
  bool in_center = ctx.bot->GetRegions().IsConnected(game.GetPosition(), center_spawn);

  bool use_repel = bb.ValueOr<bool>(BBKey::UseRepel, false);

  CombatRole role = bb.ValueOr<CombatRole>(BBKey::CombatRole, CombatRole::Rusher);

  //if (!bb.ValueOr<bool>("InCenter", true)) {
    if (!in_center) {
    //if (bb.ValueOr<bool>("IsAnchor", false)) {
    if (role == CombatRole::Anchor) {
      if (game.GetEnergyPercent() < 20.0f && game.GetPlayer().repels > 0) {
        game.Repel(ctx.bot->GetKeys());

        g_RenderState.RenderDebugText("  DevaRepelEnemyNode: %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Success;
      }
    //} else if (bb.ValueOr<bool>("UseRepel", false)) {
    } else if (use_repel) {
      if (game.GetEnergyPercent() < 10.0f && game.GetPlayer().repels > 0 && game.GetPlayer().bursts == 0) {
        game.Repel(ctx.bot->GetKeys());

        g_RenderState.RenderDebugText("  DevaRepelEnemyNode: %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Success;
      }
    }
  }

  g_RenderState.RenderDebugText("  DevaRepelEnemyNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult DevaBurstEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  Vector2f center_spawn = bb.ValueOr<Vector2f>(BBKey::CenterSpawnPoint, Vector2f(512, 512));
  bool in_center = ctx.bot->GetRegions().IsConnected(game.GetPosition(), center_spawn);

  //if (!bb.ValueOr<bool>("InCenter", true)) {
  if (!in_center) {
    const Player* target = bb.ValueOr<const Player*>(BBKey::TargetPlayer, nullptr);
    //const Player* target = bb.GetTarget();
    if (!target) {
      g_RenderState.RenderDebugText("  DevaBurstEnemyNode: %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    }

    if ((target->position.Distance(game.GetPosition()) < 7.0f || game.GetEnergyPercent() < 10.0f) &&
        game.GetPlayer().bursts > 0) {
      game.Burst(ctx.bot->GetKeys());

      g_RenderState.RenderDebugText("  DevaBurstEnemyNode: %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Success;
    }
  }

  g_RenderState.RenderDebugText("  DevaBurstEnemyNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult DevaMoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  Vector2f center_spawn = bb.ValueOr<Vector2f>(BBKey::CenterSpawnPoint, Vector2f(512, 512));
  bool in_center = ctx.bot->GetRegions().IsConnected(game.GetPosition(), center_spawn);

  float energy_pct = game.GetEnergy() / (float)game.GetShipSettings().MaximumEnergy;
  CombatRole role = bb.ValueOr<CombatRole>(BBKey::CombatRole, CombatRole::Rusher);

  // attempt to give each bot a random hover distance
  // TODO: change hover distance based on spre / bounty

  float base_distance = (float)(game.GetPlayer().id % 10) + 10.0f;
  float bounty = (float)game.GetPlayer().bounty - 750.0f;
  base_distance += (bounty / 100.0f);

  if (base_distance < 5.0f) base_distance = 5.0f;
  if (base_distance > 30.0f) base_distance = 30.0f;

  const Player* target = bb.ValueOr<const Player*>(BBKey::TargetPlayer, nullptr);
  //const Player* target = bb.GetTarget();

  if (!target) {
    g_RenderState.RenderDebugText("  DevaMoveToEnemyNode (no target): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  float player_energy_pct = target->energy / (float)game.GetShipSettings(target->ship).MaximumEnergy;

  bool in_safe = game.GetMap().GetTileId(game.GetPlayer().position) == marvin::kSafeTileId;

  Vector2f shot_position = bb.ValueOr<Vector2f>(BBKey::AimingSolution, Vector2f());
  //Vector2f shot_position = bb.GetSolution();

  float hover_distance = 0.0f;

  if (in_safe) {
    hover_distance = 0.0f;
  } else {
    // if (bb.ValueOr<bool>("InCenter", true)) {
    if (in_center) {
      float diff = 10.0f * (energy_pct - player_energy_pct);
      hover_distance = base_distance - diff;

      if (hover_distance < 1.0f) {
        hover_distance = 1.0f;
      }

    } else {
      // if (bb.ValueOr<bool>("IsAnchor", false)) {
      if (role == CombatRole::Anchor) {
        hover_distance = 20.0f;
      } else {
        hover_distance = 7.0f;
      }
    }
  }

  CombatDifficulty difficulty = bb.ValueOr<CombatDifficulty>(BBKey::CombatDifficulty, CombatDifficulty::Normal);

  if (difficulty == CombatDifficulty::Nerf) {
    hover_distance += 5;
  }

  ctx.bot->Move(shot_position, hover_distance);

  ctx.bot->GetSteering().Face(*ctx.bot, shot_position);

  g_RenderState.RenderDebugText("  DevaMoveToEnemyNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

bool DevaMoveToEnemyNode::IsAimingAt(GameProxy& game, const Player& shooter, const Player& target, Vector2f* dodge) {
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
        FloatingRayBoxIntersect(shooter.position - side, direction, target.position, radius, &distance,
                                               nullptr)) {
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

}  // namespace deva
}  // namespace marvin
