#include "Hyperspace.h"

#include <chrono>
#include <cstring>
#include <limits>

#include "../../Bot.h"
#include "../../Debug.h"
#include "../../GameProxy.h"
#include "../../Map.h"
#include "../../RayCaster.h"
#include "../../RegionRegistry.h"
#include "../../Shooter.h"
#include "../../platform/ContinuumGameProxy.h"
#include "../../platform/Platform.h"
#include "../../MapCoord.h"
#include "HyperGateNavigator.h"

namespace marvin {
namespace hs {

 // reference coord for region registry
const MapCoord kHyperTunnelCoord = MapCoord(16, 16);
// gates from center to tunnel
const std::vector<MapCoord> kCenterGates = {MapCoord(388, 395), MapCoord(572, 677)};
// gates to go back to center
const std::vector<MapCoord> kTunnelToCenterGates = {MapCoord(62, 351), MapCoord(961, 350)};
// gates to 7 bases plus top area in order
const std::vector<MapCoord> kBaseTunnelGates = {MapCoord(961, 63),  MapCoord(960, 673), MapCoord(960, 960),
                                                MapCoord(512, 959), MapCoord(64, 960),  MapCoord(64, 672),
                                                MapCoord(65, 65), MapCoord(512, 64)};


void HyperspaceBehaviorBuilder::CreateBehavior(Bot& bot) {
  std::vector<MapCoord> patrol_nodes = {MapCoord(585, 540), MapCoord(400, 570)};

  // bot.GetBlackboard().Set<std::vector<Vector2f>>("PatrolNodes", patrol_nodes);
  //  bot.GetBlackboard().Set<uint16_t>("Freq", 999);
  //  bot.GetBlackboard().Set<uint16_t>("PubTeam0", 90);
  //   bot.GetBlackboard().Set<uint16_t>("PubTeam1", 91);
  //  bot.GetBlackboard().Set<uint16_t>("Ship", ship);
  //   bot.GetBlackboard().Set<Vector2f>("Spawn", Vector2f(512, 512));

  bot.GetBlackboard().SetPatrolNodes(patrol_nodes);
  bot.GetBlackboard().SetPubTeam0(90);
  bot.GetBlackboard().SetPubTeam1(91);
  bot.GetBlackboard().SetCenterSpawn(MapCoord(512, 512));
  // fastest allowed for hypersppace
  bot.GetBlackboard().SetSetShipCoolDown(5000);
  bot.GetBlackboard().SetUpdateLancsFlag(true);

  auto move_to_enemy = std::make_unique<bot::MoveToEnemyNode>();
  auto anchor_base_path = std::make_unique<bot::AnchorBasePathNode>();
  auto find_enemy_in_base = std::make_unique<bot::FindEnemyInBaseNode>();

  auto is_anchor = std::make_unique<bot::IsAnchorNode>();
  auto bouncing_shot = std::make_unique<bot::BouncingShotNode>();
  auto HS_base_patrol = std::make_unique<hs::HSFlaggerBasePatrolNode>();
  auto patrol_center = std::make_unique<bot::PatrolNode>();
  auto HS_toggle = std::make_unique<hs::HSToggleNode>();
  auto HS_flagger_attach = std::make_unique<hs::HSAttachNode>();
  auto HS_warp = std::make_unique<hs::HSWarpToCenterNode>();
  auto HS_shipman = std::make_unique<hs::HSShipManagerNode>();
  auto HS_freqman = std::make_unique<hs::HSFreqManagerNode>();
  auto HS_set_defense_position = std::make_unique<hs::HSSetDefensePositionNode>();
  auto HS_player_sort = std::make_unique<hs::HSPlayerSortNode>();
  auto team_sort = std::make_unique<bot::SortBaseTeams>();
  auto HS_set_region = std::make_unique<hs::HSSetRegionNode>();

  auto HS_is_flagging = std::make_unique<hs::HSFlaggingCheckNode>();
  auto HS_move_to_base = std::make_unique<hs::HSMoveToBaseNode>();
  auto HS_gather_flags = std::make_unique<hs::HSGatherFlagsNode>();
  auto HS_drop_flags = std::make_unique<hs::HSDropFlagsNode>();

  auto move_method_selector = std::make_unique<behavior::SelectorNode>(move_to_enemy.get());
  auto los_weapon_selector = std::make_unique<behavior::SelectorNode>(shoot_enemy_.get());
  auto parallel_shoot_enemy = std::make_unique<behavior::ParallelNode>(los_weapon_selector.get(), move_method_selector.get());

  auto path_to_enemy_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy_.get(), follow_path_.get());
  auto anchor_base_path_sequence =
      std::make_unique<behavior::SequenceNode>(is_anchor.get(), anchor_base_path.get(), follow_path_.get());
  auto enemy_path_logic_selector = std::make_unique<behavior::SelectorNode>(
      mine_sweeper_.get(), anchor_base_path_sequence.get(), path_to_enemy_sequence.get());

  auto anchor_los_parallel = std::make_unique<behavior::ParallelNode>(anchor_base_path_sequence.get());
  auto anchor_los_sequence = std::make_unique<behavior::SequenceNode>(is_anchor.get(), anchor_los_parallel.get());
  auto anchor_los_selector =
      std::make_unique<behavior::SelectorNode>(anchor_los_sequence.get(), parallel_shoot_enemy.get());

  auto los_shoot_conditional =
      std::make_unique<behavior::SequenceNode>(target_in_los_.get(), anchor_los_selector.get());
  auto bounce_path_parallel =
      std::make_unique<behavior::ParallelNode>(bouncing_shot.get(), enemy_path_logic_selector.get());
  auto path_or_shoot_selector =
      std::make_unique<behavior::SelectorNode>(los_shoot_conditional.get(), bounce_path_parallel.get());


  auto find_enemy_in_center_sequence =
      std::make_unique<behavior::SequenceNode>(find_enemy_in_center_.get(), path_or_shoot_selector.get());
  auto find_enemy_in_base_sequence =  
      std::make_unique<behavior::SequenceNode>(find_enemy_in_base.get(),path_or_shoot_selector.get());
  auto HS_flagger_find_enemy_selector =
      std::make_unique<behavior::SelectorNode>(find_enemy_in_center_sequence.get(), find_enemy_in_base_sequence.get()); 
  //auto find_enemy_selector =
  //    std::make_unique<behavior::SelectorNode>(find_enemy_in_center_sequence.get(), find_enemy_in_base_sequence.get());

  auto HS_base_patrol_sequence =
      std::make_unique<behavior::SequenceNode>(HS_base_patrol.get(), follow_path_.get());
  auto HS_gather_flags_sequence =
      std::make_unique<behavior::SequenceNode>(HS_gather_flags.get(), follow_path_.get());
  auto HS_drop_flags_sequence = std::make_unique<behavior::SequenceNode>(HS_drop_flags.get(), follow_path_.get());
  auto HS_move_to_base_sequence = std::make_unique<behavior::SequenceNode>(HS_move_to_base.get(), follow_path_.get());
  auto patrol_center_sequence = std::make_unique<behavior::SequenceNode>(patrol_center.get(), follow_path_.get());
  //auto patrol_selector =
    //  std::make_unique<behavior::SelectorNode>(patrol_center_sequence.get(), patrol_base_sequence.get());



  auto flagger_selector = std::make_unique<behavior::SelectorNode>(
      HS_gather_flags_sequence.get(), HS_drop_flags_sequence.get(), HS_flagger_attach.get(),
      HS_move_to_base_sequence.get(), HS_flagger_find_enemy_selector.get(), HS_base_patrol_sequence.get());
  auto flagger_sequence = std::make_unique<behavior::SequenceNode>(HS_is_flagging.get(), flagger_selector.get());




  auto action_selector = std::make_unique<behavior::SelectorNode>(
      flagger_sequence.get(), find_enemy_in_center_sequence.get(), patrol_center_sequence.get());

  auto root_sequence = std::make_unique<behavior::SequenceNode>(
      commands_.get(), respawn_check_.get(), spectator_check_.get(), dettach_.get(), HS_player_sort.get(), team_sort.get(),
      HS_set_region.get(), HS_set_defense_position.get(), HS_freqman.get(), HS_shipman.get(), HS_warp.get(),
      HS_toggle.get(), action_selector.get());

  engine_->PushRoot(std::move(root_sequence));

  engine_->PushNode(std::move(move_method_selector));
  engine_->PushNode(std::move(los_weapon_selector));
  engine_->PushNode(std::move(parallel_shoot_enemy));
  engine_->PushNode(std::move(los_shoot_conditional));
  engine_->PushNode(std::move(path_to_enemy_sequence));
  engine_->PushNode(std::move(path_or_shoot_selector));
  engine_->PushNode(std::move(find_enemy_in_center_sequence));
  engine_->PushNode(std::move(move_to_enemy));
  engine_->PushNode(std::move(anchor_base_path));
  engine_->PushNode(std::move(is_anchor));
  engine_->PushNode(std::move(anchor_los_parallel));
  engine_->PushNode(std::move(anchor_los_sequence));
  engine_->PushNode(std::move(anchor_los_selector));
  engine_->PushNode(std::move(bouncing_shot));
  engine_->PushNode(std::move(bounce_path_parallel));
  engine_->PushNode(std::move(anchor_base_path_sequence));
  engine_->PushNode(std::move(enemy_path_logic_selector));
  engine_->PushNode(std::move(find_enemy_in_base));
  engine_->PushNode(std::move(HS_flagger_find_enemy_selector));
  engine_->PushNode(std::move(find_enemy_in_base_sequence));
  engine_->PushNode(std::move(HS_drop_flags_sequence));
  engine_->PushNode(std::move(HS_drop_flags));
  //engine_->PushNode(std::move(find_enemy_selector));
  engine_->PushNode(std::move(flagger_selector));
  engine_->PushNode(std::move(HS_base_patrol));
  engine_->PushNode(std::move(patrol_center));
  engine_->PushNode(std::move(HS_base_patrol_sequence));
  engine_->PushNode(std::move(patrol_center_sequence));
  engine_->PushNode(std::move(HS_gather_flags_sequence));
  engine_->PushNode(std::move(HS_move_to_base_sequence));
  engine_->PushNode(std::move(HS_move_to_base));
  engine_->PushNode(std::move(HS_gather_flags));
 // engine_->PushNode(std::move(patrol_selector));
  engine_->PushNode(std::move(flagger_sequence));
  engine_->PushNode(std::move(HS_toggle));
  engine_->PushNode(std::move(HS_flagger_attach));
  engine_->PushNode(std::move(HS_warp));
  engine_->PushNode(std::move(team_sort));
  engine_->PushNode(std::move(HS_shipman));
  engine_->PushNode(std::move(HS_freqman));
  engine_->PushNode(std::move(HS_set_defense_position));
  engine_->PushNode(std::move(HS_player_sort));
  engine_->PushNode(std::move(HS_set_region));
  engine_->PushNode(std::move(HS_is_flagging));
  engine_->PushNode(std::move(action_selector));
}

behavior::ExecuteResult HSFlaggingCheckNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  if (game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91) {
    g_RenderState.RenderDebugText("  HSFlaggingCheckNode(Flagging): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }
  g_RenderState.RenderDebugText("  HSFlaggingCheckNode(Not Flagging): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult HSMoveToBaseNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  const Player* anchor = bb.GetAnchor();

  bool in_center = bb.GetInCenter();
  bool in_tunnel = ctx.bot->GetRegions().IsConnected(game.GetPlayer().position, kHyperTunnelCoord);
  float radius = game.GetShipSettings().GetRadius();
  std::size_t index = bb.GetBDBaseIndex();

  if (!in_center && !in_tunnel) {
    g_RenderState.RenderDebugText("  HSPatrolBaseNode(In Base): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  if (anchor && anchor->id != game.GetPlayer().id) {
    if (ctx.bot->GetRegions().IsConnected(game.GetPosition(), anchor->position) &&
        game.GetPosition().Distance(anchor->position) > 25.0f) {
      ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPlayer().position, anchor->position, radius);
      g_RenderState.RenderDebugText("  HSPatrolBaseNode(Heading To Anchor): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Success;
    } else {
      g_RenderState.RenderDebugText("  HSPatrolBaseNode(Defending Anchor): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    }
  }

  if (in_center) {
    ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPlayer().position, kCenterGates[0], radius);
    g_RenderState.RenderDebugText("  HSPatrolBaseNode(Heading To Gate): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  if (in_tunnel) {
    ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPlayer().position, kBaseTunnelGates[index], radius);
    g_RenderState.RenderDebugText("  HSPatrolBaseNode(Heading To Gate): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }


  g_RenderState.RenderDebugText("  HSMoveToBaseNode(Failure): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult HSPlayerSortNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  // std::size_t base_index = bb.ValueOr<std::size_t>("BaseIndex", 0);
  std::size_t base_index = ctx.bot->GetBasePaths().GetBaseIndex();
  const TeamGoals& goals = ctx.bot->GetTeamGoals().GetGoals();

  const Player* enemy_anchor = nullptr;

  if (game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91) {
    AnchorResult result = GetAnchors(*ctx.bot);
    // only update the anchor list if it processes the correct chat message

    if (result.found) {
      const Player* anchor = SelectAnchor(result.anchors, *ctx.bot);
      bb.SetAnchor(anchor);
      bb.SetTeamHasSummoner(result.has_summoner);
      bb.SetUpdateLancsFlag(false);
    }
  }

  for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
    const Player& player = game.GetPlayers()[i];

    // player is on a flag team and not a fake player
    if ((player.frequency == 90 || player.frequency == 91) && player.name[0] != '<') {
      if (player.frequency != game.GetPlayer().frequency) {
        if (ctx.bot->GetRegions().IsConnected(player.position, goals.entrances[base_index])) {
          // check for an enemy lanc of not then select enemy spid or lev
          if (player.ship == 6) {
            enemy_anchor = &game.GetPlayers()[i];
          } else if (!enemy_anchor && player.ship == 2 || player.ship == 3) {
            enemy_anchor = &game.GetPlayers()[i];
          }
        }
      }
    }

    bb.SetEnemyAnchor(enemy_anchor);

    g_RenderState.RenderDebugText("  HSPlayerSortNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }
}

/*
* This function is usually returning empty until it catches the correct chat message
* so it includes a flag to know when the message was successfully read.
*/
AnchorResult HSPlayerSortNode::GetAnchors(Bot& bot) {
  auto& game = bot.GetGame();
  auto& bb = bot.GetBlackboard();

  AnchorResult result;
  result.found = false;
  result.has_summoner = false;
  
  uint16_t ship = game.GetPlayer().ship;
  bool evoker_usable = ship <= 1 || ship == 4 || ship == 5;
  const std::string no_lancs_msg = "There are no Lancasters on your team.";

  // let the other node know it updated the
  if (bot.GetTime().TimedActionDelay("sendchatmessage", (rand() % 2000) + 5000)) {
    game.SendChatMessage("?lancs");
  }

  //                 (summoner)      (evoker)   (in a lanc but no summoner/evoker)
  // example format: (S) Baked Cake, (E) marv1, marv2
  for (ChatMessage& chat : game.GetChat()) {

    // make sure message is a server message
    if (chat.type != ChatType::Arena) {
      continue;
    }
    if (chat.message == no_lancs_msg) {
      result.found = true;
    }
    if (result.found) {
      break;
    }

    std::vector<std::string_view> list = SplitString(chat.message, ", ");

    for (std::size_t i = 0; i < list.size(); i++) {
      
      std::string_view entry = list[i];
      std::vector<const Player*>* type_list = &result.anchors.no_energy;
      const Player* anchor = nullptr;
      std::string_view name = entry;

      
      
      if (entry.size() > 4 && entry[0] == '(' && entry[2] == ')') {
        name = entry.substr(4);

        if (entry[1] == 'S') {
          type_list = &result.anchors.full_energy;
          result.has_summoner = true;
        }
        if (entry[1] == 'E') {
          if (evoker_usable) {
            type_list = &result.anchors.full_energy;
          }
        }
      }
      
      anchor = game.GetPlayerByName(name);
      
      if (anchor) {       
        // the message is confirmed as the correct lancs message
        if (i == list.size() - 1) {
          result.found = true;
        }
        // player is too large to attach to a spid or lev, don't add it to the list
        if (!evoker_usable && (anchor->ship == 2 || anchor->ship == 3)) {
          continue;
        }

        type_list->push_back(anchor);
        
      } else {
        result.anchors.Clear();
        result.has_summoner = false;
        break;
      }
    }
  }
  return result;
}

const Player* HSPlayerSortNode::SelectAnchor(const AnchorSet& anchors, Bot& bot) {
  auto& game = bot.GetGame();

  const Player* anchor = FindAnchorInBase(anchors.full_energy, bot);
  if (!anchor) {
    anchor = FindAnchorInBase(anchors.no_energy, bot);
  }
  if (!anchor) {
    anchor = FindAnchorInAnyBase(anchors.full_energy, bot);
  }
  if (!anchor) {
    anchor = FindAnchorInAnyBase(anchors.no_energy, bot);
  }
  if (!anchor) {
    anchor = FindAnchorInTunnel(anchors.full_energy, bot);
  }
  if (!anchor) {
    anchor = FindAnchorInTunnel(anchors.no_energy, bot);
  }
  if (!anchor) {
    anchor = FindAnchorInCenter(anchors.full_energy, bot);
  }
  if (!anchor) {
    anchor = FindAnchorInCenter(anchors.no_energy, bot);
  }
  return anchor;
}

const Player* HSPlayerSortNode::FindAnchorInBase(const std::vector<const Player*>& anchors, Bot& bot) {
  auto& game = bot.GetGame();

  const Player* anchor = nullptr;

  const std::vector<MapCoord>& base_regions = bot.GetTeamGoals().GetGoals().entrances;
  auto ns = path::PathNodeSearch::Create(bot, bot.GetBasePath());

  float closest_anchor_distance_to_enemy = std::numeric_limits<float>::max();
  
  for (std::size_t i = 0; i < anchors.size(); i++) {
    const Player* player = anchors[i];
    MapCoord reference = base_regions[bot.GetBasePaths().GetBaseIndex()];
    if (!bot.GetRegions().IsConnected(player->position, reference)) {
      continue;
    }

    for (std::size_t i = 0; i < bot.GetBlackboard().GetEnemyList().size(); i++) {
      const Player* enemy = bot.GetBlackboard().GetEnemyList()[i];
      if (!bot.GetRegions().IsConnected(enemy->position, reference)) {
        continue;
      }

      float distance = ns->GetPathDistance(enemy->position, player->position);
      if (distance < closest_anchor_distance_to_enemy) {
        anchor = player;
        closest_anchor_distance_to_enemy = distance;
      }
    }
  }
  return anchor;
}

const Player* HSPlayerSortNode::FindAnchorInAnyBase(const std::vector<const Player*>& anchors, Bot& bot) {
  auto& game = bot.GetGame();

  const Player* anchor = nullptr;
  const std::vector<MapCoord>& base_regions = bot.GetTeamGoals().GetGoals().entrances;

  for (std::size_t i = 0; i < anchors.size(); i++) {
    const Player* player = anchors[i];
    for (MapCoord base_coord : base_regions) {
      if (bot.GetRegions().IsConnected(player->position, base_coord)) {
        anchor = player;
        break;
      }
    }
  }
  return anchor;
}

const Player* HSPlayerSortNode::FindAnchorInTunnel(const std::vector<const Player*>& anchors, Bot& bot) {
  auto& game = bot.GetGame();

  const Player* anchor = nullptr;

  for (std::size_t i = 0; i < anchors.size(); i++) {
    const Player* player = anchors[i];
    if (bot.GetRegions().IsConnected(player->position, kHyperTunnelCoord)) {
      anchor = player;
      break;
    }
  }
  return anchor;
}

const Player* HSPlayerSortNode::FindAnchorInCenter(const std::vector<const Player*>& anchors, Bot& bot) {
  auto& game = bot.GetGame();

  const Player* anchor = nullptr;
  float closest_distance_to_gate = std::numeric_limits<float>::max();

  for (std::size_t i = 0; i < anchors.size(); i++) {
    const Player* player = anchors[i];
    if (!bot.GetRegions().IsConnected(player->position, MapCoord(512, 512))) {
      continue;
    }

    float distance = player->position.Distance(kCenterGates[0]);

    if (distance < closest_distance_to_gate) {
      anchor = player;
      closest_distance_to_gate = distance;
    }
  }
  return anchor;
}

behavior::ExecuteResult HSSetRegionNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  GameProxy& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  RegionRegistry& regions = ctx.bot->GetRegions();

  const std::vector<MapCoord>& entrances = ctx.bot->GetTeamGoals().GetGoals().entrances;
  const std::vector<MapCoord>& flagrooms = ctx.bot->GetTeamGoals().GetGoals().flag_rooms;

  bool in_center = regions.IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));

   bb.SetInCenter(in_center);

   Vector2f position = game.GetPosition();
   const Player* anchor = bb.GetAnchor();
   const Player* enemy_anchor = bb.GetEnemyAnchor();

   if (anchor) {
    position = anchor->position;
   } else if (enemy_anchor) {
    position = enemy_anchor->position;
   }

   // if in a base set that base as the index
   for (std::size_t i = 0; i < entrances.size(); i++) {
    if (regions.IsConnected(position, entrances[i])) {
      ctx.bot->GetBasePaths().SetBase(i);
      break;
    }
   }
  
  g_RenderState.RenderDebugText("  HSSetRegionNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSSetDefensePositionNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  if (game.GetPlayer().frequency != 90 && game.GetPlayer().frequency != 91) {
    g_RenderState.RenderDebugText("  HSDefensePositionNode(Not Flagging): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  const std::vector<MapCoord>& entrances = ctx.bot->GetTeamGoals().GetGoals().entrances;
  const std::vector<MapCoord>& flagrooms = ctx.bot->GetTeamGoals().GetGoals().flag_rooms;
  std::size_t base_index = ctx.bot->GetBasePaths().GetBaseIndex();

  if (!ctx.bot->GetRegions().IsConnected(game.GetPosition(), entrances[base_index])) {
    g_RenderState.RenderDebugText("  HSDefensePositionNode(Not In Base): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  const Player* team_anchor = bb.GetAnchor();
  const Player* enemy_anchor = bb.GetEnemyAnchor();

    if (!team_anchor || !enemy_anchor) {
      g_RenderState.RenderDebugText("  HSDefensePositionNode(No Anchor): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Success;
    }


    if (ctx.bot->GetRegions().IsConnected(team_anchor->position, entrances[base_index])) {
      if (ctx.bot->GetRegions().IsConnected(enemy_anchor->position, entrances[base_index])) {
      auto ns = path::PathNodeSearch::Create(*ctx.bot, ctx.bot->GetBasePath());
      std::size_t team_node = ns->FindNearestNodeBFS(team_anchor->position);
      std::size_t enemy_node = ns->FindNearestNodeBFS(enemy_anchor->position);

      // assumes that entrances are always at the start of the base path (path[0])
      // and flag rooms are always at the end
      if (team_node < enemy_node) {
        bb.SetTeamSafe(entrances[base_index]);
        bb.SetEnemySafe(flagrooms[base_index]);
        // bb.Set<Vector2f>("TeamSafe", entrances[base_index]);
        // bb.Set<Vector2f>("EnemySafe", flagrooms[base_index]);
      } else {
        bb.SetTeamSafe(flagrooms[base_index]);
        bb.SetEnemySafe(entrances[base_index]);
        // bb.Set<Vector2f>("TeamSafe", flagrooms[base_index]);
        // bb.Set<Vector2f>("EnemySafe", entrances[base_index]);
      }
      }
    }

  g_RenderState.RenderDebugText("  HSDefensePositionNode(Flagging): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSFreqManagerNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  //uint64_t unique_timer = ctx.bot->GetTime().UniqueIDTimer(game.GetPlayer().id);
  uint64_t unique_timer = (rand() % 400) + 100;
  
  int flagger_count = bb.GetBaseTeamsCount();
  bool flagging = game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91;

  bool update_flag = bb.GetUpdateLancsFlag();

  const Player* anchor = bb.GetAnchor();
  uint16_t anchor_id = 0;
  if (anchor) {
    anchor_id = anchor->id;
  }

  // join a flag team
  if (flagger_count < 14 && !flagging && bb.GetCanFlag()) {
    if (ctx.bot->GetTime().TimedActionDelay("joingame", unique_timer)) {
      game.SetEnergy(100.0f);
      game.SendChatMessage("?flag");

      g_RenderState.RenderDebugText("  HSFreqMan(Join Flag Team): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    }
    g_RenderState.RenderDebugText("  HSFreqMan(Waiting To Join Flag Team): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  // leave a flag team
  if (flagging) {
    if ((flagger_count > 16 && game.GetPlayer().ship != 6) || !bb.GetCanFlag()) {
      if (ctx.bot->GetTime().TimedActionDelay("leavegame", unique_timer)) {
        game.SetEnergy(100.0f);
        game.SetFreq(FindOpenFreq(bb.GetFreqList(), 0));
        //bb.SetFreq(FindOpenFreq(bb.GetFreqList(), 0));

        g_RenderState.RenderDebugText("  HSFreqMan(Leave Flag Team): %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Failure;
      }
      g_RenderState.RenderDebugText("  HSFreqMan(Waiting To Leave Flag Team): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Success;
    }
  }

  g_RenderState.RenderDebugText("  HSFreqMan(No Action Taken): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSShipManagerNode::Execute(behavior::ExecuteContext& ctx) {
  return behavior::ExecuteResult::Success;
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  // uint64_t unique_timer = ctx.bot->GetTime().UniqueIDTimer(game.GetPlayer().id);
  uint64_t unique_timer = (rand() % 1000) + 5000;

  if (game.GetPlayer().frequency != 90 && game.GetPlayer().frequency != 91) {
    if (game.GetPlayer().ship == 6) {
      if (ctx.bot->GetTime().TimedActionDelay("hsswitchtoanchor", unique_timer)) {
        game.SetShip(SelectShip(game.GetPlayer().ship));
        g_RenderState.RenderDebugText("  HSFlaggerShipMan(In Lanc And Not Flagging): %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Failure;
      }
    }
    g_RenderState.RenderDebugText("  HSFlaggerShipMan(Not Flagging): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  // this flag is used to wait until bot has sent ?lancs to chat and read the resulting server message
  bool update_flag = bb.GetUpdateLancsFlag();
  const std::vector<int>& ship_counts = bb.GetTeamShipCounts();

  // use flag to wait for bot to update its anchor list before running
  if (update_flag) {
    return behavior::ExecuteResult::Success;
  }



  //uint64_t unique_timer = (rand() % 1000) + 5000;
  uint16_t ship = game.GetPlayer().ship;
  bool evoker_usable = ship <= 1 || ship == 4 || ship == 5;
  
  const Player* anchor = bb.GetAnchor();
  uint16_t anchor_id = 0;
  if (anchor) {
    anchor_id = anchor->id;
  }

  // switch to lanc if team doesnt have one
  if (ship != 6 && !bb.GetTeamHasSummoner()) {
   // if (!anchor.p || (!evoker_usable && anchor.type != AnchorType::Summoner) || anchor.type == AnchorType::None) {
      if (ctx.bot->GetTime().TimedActionDelay("hsswitchtoanchor", unique_timer)) {
        // bb.Set<uint16_t>("Ship", 6);
        //bb.SetShip(Ship::Lancaster);
        game.SetEnergy(100.0f);
        game.SetShip((uint16_t)Ship::Lancaster);
        bb.SetUpdateLancsFlag(true);
        g_RenderState.RenderDebugText("  HSFlaggerShipMan(Switch To Lanc): %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Failure;
      }
      g_RenderState.RenderDebugText("  HSFlaggerShipMan(Awaiting Switch To Lanc):");
   // }
  }

  // if there is more than one lanc on the team, switch back to original ship
  if (ship_counts[6] > 1 && bb.GetTeamHasSummoner() && anchor_id != game.GetPlayer().id && ship == 6) {
    if (ctx.bot->GetTime().TimedActionDelay("hsswitchtowarbird", unique_timer)) {
      game.SetEnergy(100.0f);
      game.SetShip(SelectShip(game.GetPlayer().ship));
      //bb.SetShip((Ship)SelectShip(game.GetPlayer().ship));
      bb.SetUpdateLancsFlag(true);
      g_RenderState.RenderDebugText("  HSFlaggerShipMan(Switch From Lanc): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    }
    g_RenderState.RenderDebugText("  HSFlaggerShipMan(Awaiting Switch From Lanc):");
  }

  // hs has such a long ship change cooldown, if this node decides no ship change is needed
  // tell the setship node to stay in the current ship
  //bb.SetShip((Ship)game.GetPlayer().ship);

  g_RenderState.RenderDebugText("  HSFlaggerShipMan(No Action Taken): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

uint16_t HSShipManagerNode::SelectShip(uint16_t current) {
  int num = rand() % 8;

  while (num == current) {
    num = rand() % 8;
  }
  return num;
}

behavior::ExecuteResult HSWarpToCenterNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  if (!bb.GetInCenter() && game.GetPlayer().frequency != 90 && game.GetPlayer().frequency != 91) {
    if (ctx.bot->GetTime().TimedActionDelay("warptocenter", 200)) {
      game.SetEnergy(100.0f);
      game.Warp();

      g_RenderState.RenderDebugText("  HSWarpToCenter(Warping): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    }
  }
  g_RenderState.RenderDebugText("  HSWarpToCenter(No Action Taken): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSAttachNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  const Player* anchor = bb.GetAnchor();
  const std::vector<MapCoord>& entrances = ctx.bot->GetTeamGoals().GetGoals().entrances;
  std::size_t base_index = bb.GetBDBaseIndex();

  if (!anchor) {
    g_RenderState.RenderDebugText("  HSAttachNode(No Anchor): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  if (game.GetPlayer().frequency != 90 && game.GetPlayer().frequency != 91) {
    g_RenderState.RenderDebugText("  HSAttachNode(Not Flagging): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  if (ctx.bot->GetRegions().IsConnected(game.GetPosition(), entrances[base_index])) {
    g_RenderState.RenderDebugText("  HSAttachNode(In Active Base): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  bool anchor_in_safe = game.GetMap().GetTileId(anchor->position) == kSafeTileId;

  if (!ctx.bot->GetRegions().IsConnected(game.GetPosition(), anchor->position) && !anchor_in_safe && IsValidPosition(anchor->position)) {
    if (ctx.bot->GetTime().RepeatedActionDelay("attach", 2000)) {
      game.SetEnergy(100.0f);
      game.SendChatMessage(":" + anchor->name + ":?attach");

      g_RenderState.RenderDebugText("  HSAttachNode(Attaching): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    }
  }

  if (game.GetPlayer().attach_id != 65535) {
    if (bb.GetSwarm()) {
      game.SetEnergy(15.0f);
    }

    game.SendKey(VK_F7);

    g_RenderState.RenderDebugText("  HSAttachNode(Dettaching): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  g_RenderState.RenderDebugText("  HSAttachNode(No Action Taken): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult HSToggleNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  const Player* anchor = bb.GetAnchor();

  if (anchor && anchor->id == game.GetPlayer().id) {
    bb.SetCombatRole(CombatRole::Anchor);
  } else {
    bb.SetCombatRole(CombatRole::Rusher);
  }

  if (bb.GetUseMultiFire()) {
    if (!game.GetPlayer().multifire_status && game.GetPlayer().multifire_capable) {
      game.SendKey(VK_DELETE);
      return behavior::ExecuteResult::Failure;
    }
  } else if (game.GetPlayer().multifire_status && game.GetPlayer().multifire_capable) {
    game.SendKey(VK_DELETE);
    return behavior::ExecuteResult::Failure;
  }

  bool x_active = (game.GetPlayer().status & 4) != 0;
  bool has_xradar = (game.GetShipSettings().XRadarStatus & 1);
  bool stealthing = (game.GetPlayer().status & 1) != 0;
  bool cloaking = (game.GetPlayer().status & 2) != 0;
  bool has_stealth = (game.GetShipSettings().StealthStatus & 2);
  bool has_cloak = (game.GetShipSettings().CloakStatus & 2);

  if (!stealthing && has_stealth) {
    if (ctx.bot->GetTime().TimedActionDelay("stealth", 800)) {
      game.Stealth();

      g_RenderState.RenderDebugText("  HSToggleNode(Toggle Stealth): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    }
  }

  if (has_cloak) {
    if (!cloaking) {
      if (ctx.bot->GetTime().TimedActionDelay("cloak", 800)) {
        game.Cloak(ctx.bot->GetKeys());

        g_RenderState.RenderDebugText("  HSToggleNode(Toggle Cloak): %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Failure;
      }
    }

    if (game.GetPlayer().ship == 6) {
      if (ctx.bot->GetTime().TimedActionDelay("anchorflashposition", 30000)) {
        game.Cloak(ctx.bot->GetKeys());

        g_RenderState.RenderDebugText("  HSToggleNode(Toggle Cloak): %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Failure;
      }
    }
  }
  // TODO: make this better
  // if (!x_active && has_xradar && no_spam) {
  //   game.XRadar();
  //    ctx.blackboard.Set("SpamCheck", time + 300);
  // return behavior::ExecuteResult::Success;
  // }
  g_RenderState.RenderDebugText("  HSToggleNode(No Action Taken): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSFlaggerBasePatrolNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  float radius = game.GetShipSettings().GetRadius();
  std::size_t index = bb.GetBDBaseIndex();
  const std::vector<MapCoord>& entrances = ctx.bot->GetTeamGoals().GetGoals().entrances;
  const std::vector<MapCoord>& flagrooms = ctx.bot->GetTeamGoals().GetGoals().flag_rooms;

  if (!ctx.bot->GetRegions().IsConnected(game.GetPosition(), flagrooms[index])) {
    g_RenderState.RenderDebugText("  HSPatrolBaseNode(Not in base): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  // bot has flags so head to flagroom
  if (game.GetPlayer().flags > 0) {
    ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPlayer().position, flagrooms[index], radius);
    g_RenderState.RenderDebugText("  HSPatrolBaseNode(Heading To Flag Room): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  } else {
    ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPlayer().position, entrances[index], radius);
    g_RenderState.RenderDebugText("  HSPatrolBaseNode(Heading To Entrance): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  g_RenderState.RenderDebugText("  HSPatrolBaseNode(Failed): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult HSDropFlagsNode::Execute(behavior::ExecuteContext& ctx) {
  return behavior::ExecuteResult::Failure;
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  const Player* anchor = bb.GetAnchor();

  if (game.GetPlayer().flags < 1) {
    g_RenderState.RenderDebugText("  HSDropFlagsNode(Not Holding Flags): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }  

  if (anchor && !ctx.bot->GetRegions().IsConnected(game.GetPosition(), anchor->position)) {
    if (ctx.bot->GetTime().RepeatedActionDelay("attachwithflags", 3000)) {
      game.SetEnergy(100.0f);
      game.SendChatMessage(":" + anchor->name + ":?attach");
    }
    g_RenderState.RenderDebugText("  HSDropFlagsNode(Attaching To Anchor): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  const std::vector<MapCoord>& entrances = ctx.bot->GetTeamGoals().GetGoals().entrances;
  std::size_t base_index = ctx.bot->GetBasePaths().GetBaseIndex();
  float radius = game.GetShipSettings().GetRadius();

  if (ctx.bot->GetRegions().IsConnected(game.GetPosition(), entrances[base_index])) {
    if (entrances[base_index] != bb.GetTeamSafe()) {
      ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPosition(), bb.GetTeamSafe(), radius);
      g_RenderState.RenderDebugText("  HSDropFlagsNode(Dropping Flags): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Success;
    }
  }

  g_RenderState.RenderDebugText("  HSDropFlagsNode(Not Dropping Flags): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult HSGatherFlagsNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  uint16_t ship = game.GetPlayer().ship;
  uint16_t id = game.GetPlayer().id;
  float radius = game.GetShipSettings().GetRadius();
  const Player* anchor = bb.GetAnchor();
  bool flagging = (game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91);

  if (!flagging) {
    g_RenderState.RenderDebugText("  HSGatherFlagsNode(Not Flagging): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  if (!InFlagCollectingShip(ship)) {
    g_RenderState.RenderDebugText("  HSGatherFlagsNode(Not In Flag Collecting Ship): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  if (game.GetPlayer().flags >= 3) {
    g_RenderState.RenderDebugText("  HSGatherFlagsNode(Flag Limit Reached): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  for (const Player* player : bb.GetTeamList()) {
    if (player->id == id) continue;

    if (player->id < id && InFlagCollectingShip(player->ship)) {
      g_RenderState.RenderDebugText("  HSGatherFlagsNode(Not Selected Flag Gatherer): %llu", timer.GetElapsedTime());
    //  return behavior::ExecuteResult::Failure;
    }
  }

  const Flag* flag = SelectFlag(ctx);

  if (flag) {
    if (!ctx.bot->GetRegions().IsConnected(game.GetPosition(), flag->position)) {
      if (anchor && ctx.bot->GetRegions().IsConnected(anchor->position, flag->position)) {
        if (ctx.bot->GetTime().RepeatedActionDelay("attachforflags", 3000)) {
          game.SetEnergy(100.0f);
          game.SendChatMessage(":" + anchor->name + ":?attach");
        }
        g_RenderState.RenderDebugText("  HSGatherFlagsNode(Attaching To Anchor): %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Failure;
      } else if (ctx.bot->GetRegions().IsConnected(flag->position, MapCoord(512, 512))) {
        if (ctx.bot->GetTime().RepeatedActionDelay("warpforflags", 300)) {
          game.SetEnergy(100.0f);
          game.Warp();
        }
        g_RenderState.RenderDebugText("  HSGatherFlagsNode(Warping To Center): %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Failure;
      }
    }
    HyperGateNavigator navigator(ctx.bot->GetRegions());
    Vector2f waypoint = navigator.GetWayPoint(game.GetPosition(), flag->position);
    ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPosition(), waypoint, radius);
    g_RenderState.RenderDebugText("  HSGatherFlagsNode(Heading To Flag ID: %u): %llu", flag->id,
                                  timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  g_RenderState.RenderDebugText("  HSGatherFlagsNode(No Flags Found): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

// start by grabbing flags in connected area first
// TODO: implement distance checking with pathfinder to find closest flag and store it in blackboard
// if stored flag gets picked up rerun for next closest flag
const Flag* HSGatherFlagsNode::SelectFlag(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  const Flag* result = nullptr;
  const Flag* connected_flag = nullptr;
  const Flag* other_flag = nullptr;

  const std::vector<MapCoord>& entrances = ctx.bot->GetTeamGoals().GetGoals().entrances;
  std::size_t base_index = ctx.bot->GetBasePaths().GetBaseIndex();

  for (const Flag& flag : game.GetDroppedFlags()) {
    if (flag.frequency == game.GetPlayer().frequency) {
      continue;
    }
    if (flag.frequency != game.GetPlayer().frequency &&
        ctx.bot->GetRegions().IsConnected(flag.position, entrances[base_index])) {
      // flags are gaurded by enemy team
      if (entrances[base_index] == bb.GetTeamSafe()) {
      //  continue;
      }
    }

    if (ctx.bot->GetRegions().IsConnected(game.GetPosition(), flag.position)) {
      connected_flag = &flag;
      break;
    } else {
      other_flag = &flag;
    }
  }

  if (connected_flag) {
    result = connected_flag;
  } else if (other_flag) {
    result = other_flag;
  }

  return result;
}

bool HSGatherFlagsNode::InFlagCollectingShip(uint64_t ship) {
  return ship == 0 || ship == 1 || ship == 7;
 }
}  // namespace hs
}  // namespace marvin
