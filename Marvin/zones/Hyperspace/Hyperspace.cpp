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
#include "../../blackboard/HyperspaceBlackboard.h"

namespace marvin {
namespace hs {

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

  auto move_to_enemy = std::make_unique<bot::MoveToEnemyNode>();
  auto anchor_base_path = std::make_unique<bot::AnchorBasePathNode>();
  auto find_enemy_in_base = std::make_unique<bot::FindEnemyInBaseNode>();

  auto is_anchor = std::make_unique<bot::IsAnchorNode>();
  auto bouncing_shot = std::make_unique<bot::BouncingShotNode>();
  auto patrol_base = std::make_unique<hs::HSPatrolBaseNode>();
  auto patrol_center = std::make_unique<bot::PatrolNode>();
  auto HS_toggle = std::make_unique<hs::HSToggleNode>();
  auto HS_attach = std::make_unique<hs::HSAttachNode>();
  auto HS_warp = std::make_unique<hs::HSWarpToCenter>();
  auto HS_shipman = std::make_unique<hs::HSFlaggerShipMan>();
  auto HS_freqman = std::make_unique<hs::HSFreqMan>();
  auto HS_defense_position = std::make_unique<hs::HSDefensePositionNode>();
  auto flagger_sort = std::make_unique<hs::HSFlaggerSort>();
  auto HS_set_region = std::make_unique<hs::HSSetRegionNode>();

  auto move_method_selector = std::make_unique<behavior::SelectorNode>(move_to_enemy.get());
  auto los_weapon_selector = std::make_unique<behavior::SelectorNode>(shoot_enemy_.get());
  auto parallel_shoot_enemy =
      std::make_unique<behavior::ParallelNode>(los_weapon_selector.get(), move_method_selector.get());

  auto path_to_enemy_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy_.get(), follow_path_.get());
  auto anchor_base_path_sequence = std::make_unique<behavior::SequenceNode>(anchor_base_path.get(), follow_path_.get());
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

  auto action_selector = std::make_unique<behavior::SelectorNode>(find_enemy_selector.get(), patrol_selector.get());
  auto root_sequence = std::make_unique<behavior::SequenceNode>(
      commands_.get(), set_ship_.get(), set_freq_.get(), ship_check_.get(), flagger_sort.get(), HS_set_region.get(),
      HS_defense_position.get(), HS_freqman.get(), HS_shipman.get(), HS_warp.get(), HS_attach.get(),
      respawn_check_.get(), HS_toggle.get(), action_selector.get());

  engine_->PushRoot(std::move(root_sequence));

  engine_->PushNode(std::move(move_method_selector));
  engine_->PushNode(std::move(los_weapon_selector));
  engine_->PushNode(std::move(parallel_shoot_enemy));
  engine_->PushNode(std::move(los_shoot_conditional));
  engine_->PushNode(std::move(path_to_enemy_sequence));
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
  engine_->PushNode(std::move(find_enemy_in_base_sequence));
  engine_->PushNode(std::move(find_enemy_selector));
  engine_->PushNode(std::move(patrol_base));
  engine_->PushNode(std::move(patrol_center));
  engine_->PushNode(std::move(patrol_base_sequence));
  engine_->PushNode(std::move(patrol_center_sequence));
  engine_->PushNode(std::move(patrol_selector));
  engine_->PushNode(std::move(HS_toggle));
  engine_->PushNode(std::move(HS_attach));
  engine_->PushNode(std::move(HS_warp));
  engine_->PushNode(std::move(HS_shipman));
  engine_->PushNode(std::move(HS_freqman));
  engine_->PushNode(std::move(HS_defense_position));
  engine_->PushNode(std::move(flagger_sort));
  engine_->PushNode(std::move(HS_set_region));
  engine_->PushNode(std::move(action_selector));
}

behavior::ExecuteResult HSFlaggerSort::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  auto& hs_bb = ctx.bot->GetHSBlackboard();

  // std::size_t base_index = bb.ValueOr<std::size_t>("BaseIndex", 0);
  std::size_t base_index = ctx.bot->GetBasePaths().GetBaseIndex();
  const TeamGoals& goals = ctx.bot->GetTeamGoals().GetGoals();

  const Player* enemy_anchor = nullptr;

  AnchorResult result = GetAnchors(*ctx.bot);
  // only update the anchor list if it processes the correct chat message
  if (result.found) {
    const AnchorPlayer& anchor = SelectAnchor(result.anchors, *ctx.bot);
    hs_bb.SetAnchor(anchor);
    hs_bb.SetUpdateLancsFlag(false);
  }

  int flagger_count = 0;
  int spid_count = 0;
  int lanc_count = 0;

  for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
    const Player& player = game.GetPlayers()[i];

    // max allowed for HS
    if (player.frequency <= 91) {
      // fList[player.frequency]++;
      bb.IncrementFreqList(player.frequency);
    }

    // player is on a flag team and not a fake player
    if ((player.frequency == 90 || player.frequency == 91) && player.name[0] != '<') {
      flagger_count++;

      // player is on same team
      if (player.frequency == game.GetPlayer().frequency) {
        if (player.ship == 2) {
          spid_count++;
        } else if (player.ship == 6) {
          lanc_count++;
        }
        // player is on enemy team and connected to the current base
      } else if (ctx.bot->GetRegions().IsConnected(player.position, goals.entrances[base_index])) {
        // check for an enemy lanc of not then select enemy spid or lev
        if (player.ship == 6) {
          enemy_anchor = &game.GetPlayers()[i];
        } else if (!enemy_anchor && player.ship == 2 || player.ship == 3) {
          enemy_anchor = &game.GetPlayers()[i];
        }
      }
    }
  }

  hs_bb.SetEnemyAnchor(enemy_anchor);
  hs_bb.SetFlaggerCount(flagger_count);
  hs_bb.SetTeamLancCount(lanc_count);
  hs_bb.SetTeamSpidCount(spid_count);

  g_RenderState.RenderDebugText("  HSFlaggerSortNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

/*
* This function is usually returning junk/empty values until it catches the correct chat message
* so it includes a flag to know when the return value sholud be saved.
*/
AnchorResult HSFlaggerSort::GetAnchors(Bot& bot) {
  auto& game = bot.GetGame();

  AnchorResult result;
  result.found = false;
  
  uint16_t ship = game.GetPlayer().ship;
  bool evoker_usable = ship <= 1 || ship == 4 || ship == 5;
  const std::string no_lancs_msg = "There are no Lancasters on your team.";

  // let the other node know it updated the
  if (bot.GetTime().TimedActionDelay("sendchatmessage", 5000)) {
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

    std::vector<std::string_view> list = SplitString(chat.message, ",");

    for (std::size_t i = 0; i < list.size(); i++) {
      
      std::string_view entry = list[i];
      std::vector<AnchorPlayer>* type_list = &result.anchors.no_energy;
      AnchorPlayer anchor;
      anchor.type = AnchorType::None;
      std::string_view name = entry;
      
      if (entry.size() > 4 && entry[0] == '(' && entry[2] == ')') {
        name = (std::string_view)entry.substr(4);

        if (entry[1] == 'S') {
          anchor.type = AnchorType::Summoner;
          type_list = &result.anchors.full_energy;
        }
        if (entry[1] == 'E') {
          anchor.type = AnchorType::Evoker;
          if (evoker_usable) {
            type_list = &result.anchors.full_energy;
          }
        }
      }
      
      anchor.p = game.GetPlayerByName(name);

      if (anchor.p) { 
        if (i == list.size() - 1) {
          result.found = true;
        }
        // player too large to attach to a spid or lev
        if (!evoker_usable && (anchor.p->ship == 2 || anchor.p->ship == 3)) {
          continue;
        }
        type_list->push_back(anchor);
        
      } else {
        result.anchors.Clear();
        break;
      }
    }
  }
  return result;
}

AnchorPlayer HSFlaggerSort::SelectAnchor(const AnchorSet& anchors, Bot& bot) {
  auto& game = bot.GetGame();

  AnchorPlayer anchor = FindAnchorInBase(anchors.full_energy, bot);
  if (!anchor.p) {
    anchor = FindAnchorInBase(anchors.no_energy, bot);
  }
  if (!anchor.p) {
    anchor = FindAnchorInAnyBase(anchors.full_energy, bot);
  }
  if (!anchor.p) {
    anchor = FindAnchorInAnyBase(anchors.no_energy, bot);
  }
  if (!anchor.p) {
    anchor = FindAnchorInTunnel(anchors.full_energy, bot);
  }
  if (!anchor.p) {
    anchor = FindAnchorInTunnel(anchors.no_energy, bot);
  }
  if (!anchor.p) {
    anchor = FindAnchorInCenter(anchors.full_energy, bot);
  }
  if (!anchor.p) {
    anchor = FindAnchorInCenter(anchors.no_energy, bot);
  }
  return anchor;
}

AnchorPlayer HSFlaggerSort::FindAnchorInBase(const std::vector<AnchorPlayer>& anchors, Bot& bot) {
  auto& game = bot.GetGame();

  AnchorPlayer anchor;

  const std::vector<MapCoord>& base_regions = bot.GetTeamGoals().GetGoals().entrances;
  auto ns = path::PathNodeSearch::Create(bot, bot.GetBasePath());

  float closest_anchor_distance_to_enemy = std::numeric_limits<float>::max();
  
  for (std::size_t i = 0; i < anchors.size(); i++) {
    const AnchorPlayer& player = anchors[i];
    MapCoord reference = base_regions[bot.GetBasePaths().GetBaseIndex()];
    if (!bot.GetRegions().IsConnected(player.p->position, reference)) {
      continue;
    }

    for (std::size_t i = 0; i < bot.GetBlackboard().GetEnemyList().size(); i++) {
      const Player* enemy = bot.GetBlackboard().GetEnemyList()[i];
      if (!bot.GetRegions().IsConnected(enemy->position, reference)) {
        continue;
      }

      float distance = ns->GetPathDistance(enemy->position, player.p->position);
      if (distance < closest_anchor_distance_to_enemy) {
        anchor = player;
        closest_anchor_distance_to_enemy = distance;
      }
    }
  }
  return anchor;
}

AnchorPlayer HSFlaggerSort::FindAnchorInAnyBase(const std::vector<AnchorPlayer>& anchors, Bot& bot) {
  auto& game = bot.GetGame();

  AnchorPlayer anchor;
  const std::vector<MapCoord>& base_regions = bot.GetTeamGoals().GetGoals().entrances;

  for (std::size_t i = 0; i < anchors.size(); i++) {
    const AnchorPlayer& player = anchors[i];
    for (MapCoord base_coord : base_regions) {
      if (bot.GetRegions().IsConnected(player.p->position, base_coord)) {
        anchor = player;
        break;
      }
    }
  }
  return anchor;
}

AnchorPlayer HSFlaggerSort::FindAnchorInTunnel(const std::vector<AnchorPlayer>& anchors, Bot& bot) {
  auto& game = bot.GetGame();

  AnchorPlayer anchor;

  for (std::size_t i = 0; i < anchors.size(); i++) {
    const AnchorPlayer& player = anchors[i];
    if (bot.GetRegions().IsConnected(player.p->position, MapCoord(16, 16))) {
      anchor = player;
      break;
    }
  }
  return anchor;
}

AnchorPlayer HSFlaggerSort::FindAnchorInCenter(const std::vector<AnchorPlayer>& anchors, Bot& bot) {
  auto& game = bot.GetGame();

  AnchorPlayer anchor;

  for (std::size_t i = 0; i < anchors.size(); i++) {
    const AnchorPlayer& player = anchors[i];
    if (bot.GetRegions().IsConnected(player.p->position, MapCoord(512, 512))) {
      anchor = player;
      break;
    }
  }
  return anchor;
}

behavior::ExecuteResult HSSetRegionNode::Execute(behavior::ExecuteContext& ctx) {
  GameProxy& game = ctx.bot->GetGame();
  Blackboard& bb = ctx.bot->GetBlackboard();
  RegionRegistry& regions = ctx.bot->GetRegions();

  const std::vector<MapCoord>& entrances = ctx.bot->GetTeamGoals().GetGoals().entrances;
  const std::vector<MapCoord>& flagrooms = ctx.bot->GetTeamGoals().GetGoals().flag_rooms;

  bool in_center = regions.IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));

   bb.SetInCenter(in_center);

  // not in center
  if (!in_center && game.GetPlayer().active) {
      // if in a base set that base as the index
    for (std::size_t i = 0; i < entrances.size(); i++) {
      if (regions.IsConnected(game.GetPosition(), entrances[i])) {
        ctx.bot->GetBasePaths().SetBase(i);
        break;
      }
    }
  }

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSDefensePositionNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  auto& hs_bb = ctx.bot->GetHSBlackboard();
  auto ns = path::PathNodeSearch::Create(*ctx.bot, ctx.bot->GetBasePath());

  const std::vector<MapCoord>& entrances = ctx.bot->GetTeamGoals().GetGoals().entrances;
  const std::vector<MapCoord>& flagrooms = ctx.bot->GetTeamGoals().GetGoals().flag_rooms;
  std::size_t base_index = ctx.bot->GetBasePaths().GetBaseIndex();

  const AnchorPlayer& team_anchor = hs_bb.GetAnchor();
  const Player* enemy_anchor = hs_bb.GetEnemyAnchor();

  if (team_anchor.p && enemy_anchor) {
    if (ctx.bot->GetRegions().IsConnected((MapCoord)team_anchor.p->position, (MapCoord)entrances[base_index])) {
      if (ctx.bot->GetRegions().IsConnected((MapCoord)enemy_anchor->position, (MapCoord)entrances[base_index])) {
        std::size_t team_node = ns->FindNearestNodeBFS(team_anchor.p->position);
        std::size_t enemy_node = ns->FindNearestNodeBFS(enemy_anchor->position);

        // assumes that entrances are always at the start of the base path (path[0])
        // and flag rooms are always at the end
        if (team_node < enemy_node) {
          bb.SetTeamSafe(entrances[base_index]);
          bb.SetEnemySafe(flagrooms[base_index]);
          //bb.Set<Vector2f>("TeamSafe", entrances[base_index]);
          //bb.Set<Vector2f>("EnemySafe", flagrooms[base_index]);
        } else {
          bb.SetTeamSafe(flagrooms[base_index]);
          bb.SetEnemySafe(entrances[base_index]);
         // bb.Set<Vector2f>("TeamSafe", flagrooms[base_index]);
         // bb.Set<Vector2f>("EnemySafe", entrances[base_index]);
        }
      }
    }
  }

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSFreqMan::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  auto& hs_bb = ctx.bot->GetHSBlackboard();

  uint64_t unique_timer = ctx.bot->GetTime().UniqueIDTimer(game.GetPlayer().id);
  
  int flagger_count = hs_bb.GetFlaggerCount();
  bool flagging = game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91;

  const AnchorPlayer& anchor = hs_bb.GetAnchor();
  uint16_t anchor_id = 0;
  if (anchor.p) {
    anchor_id = anchor.p->id;
  }

  // join a flag team
  if (flagger_count < 14 && !flagging) {
    if (ctx.bot->GetTime().TimedActionDelay("joingame", unique_timer)) {
      game.SetEnergy(100.0f);
      game.SendChatMessage("?flag");

      return behavior::ExecuteResult::Failure;
    }
  }

  // leave a flag team
  if (flagger_count > 16 && flagging && game.GetPlayer().id != anchor_id) {
    if (ctx.bot->GetTime().TimedActionDelay("leavegame", unique_timer)) {
      game.SetEnergy(100.0f);
      game.SetFreq(FindOpenFreq(bb.GetFreqList(), 0));

      return behavior::ExecuteResult::Failure;
    }
  }

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSFlaggerShipMan::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  auto& hs_bb = ctx.bot->GetHSBlackboard();

  if (game.GetPlayer().frequency != 90 && game.GetPlayer().frequency != 91) {
    return behavior::ExecuteResult::Success;
  }

  // int lanc_count = bb.ValueOr<int>("LancCount", 0);
  // int spider_count = bb.ValueOr<int>("SpiderCount", 0);
  // uint16_t last_ship = bb.ValueOr<uint16_t>("LastShip", 0);
  int lanc_count = hs_bb.GetTeamLancCount();
  int spid_count = hs_bb.GetTeamSpidCount();
  bool update_flag = hs_bb.GetUpdateLancsFlag();

  // use flag to wait for bot to update its anchor list before running
  if (update_flag) {
    return behavior::ExecuteResult::Success;
  }

  uint64_t unique_timer = ctx.bot->GetTime().UniqueIDTimer(game.GetPlayer().id);
  
  const AnchorPlayer& anchor = hs_bb.GetAnchor();
  uint16_t anchor_id = 0;
  if (anchor.p) {
    anchor_id = anchor.p->id;
  }

  // switch to lanc if team doesnt have one
  if (!anchor.p || anchor.type != AnchorType::Summoner) {
    if (ctx.bot->GetTime().TimedActionDelay("hsswitchtoanchor", unique_timer)) {
      //bb.Set<uint16_t>("Ship", 6);
      bb.SetShip(Ship::Lancaster);
      hs_bb.SetUpdateLancsFlag(true);
      return behavior::ExecuteResult::Success;
    }
  }

  // switch to spider if team doesnt have one
  if (spid_count == 0 && anchor_id != game.GetPlayer().id) {
    if (ctx.bot->GetTime().TimedActionDelay("hsswitchtospider", unique_timer)) {
      //bb.Set<uint16_t>("Ship", 2);
      bb.SetShip(Ship::Spider);
      hs_bb.SetUpdateLancsFlag(true);
      return behavior::ExecuteResult::Success;
    }
  }

  // if there is more than one lanc on the team, switch back to original ship
  if (lanc_count > 1 && game.GetPlayer().ship == 6 && anchor_id != game.GetPlayer().id) {
    if (ctx.bot->GetTime().TimedActionDelay("hsswitchtowarbird", unique_timer)) {
      bb.SetShip((Ship)SelectShip(game.GetPlayer().ship));
      hs_bb.SetUpdateLancsFlag(true);
      return behavior::ExecuteResult::Success;
    }
  }

  // if there is more than two spiders on the team, switch back to original ship
  if (spid_count > 2 && game.GetPlayer().ship == 2) {
    if (ctx.bot->GetTime().TimedActionDelay("switchtolast", unique_timer)) {
      bb.SetShip((Ship)SelectShip(game.GetPlayer().ship));
      hs_bb.SetUpdateLancsFlag(true);
      return behavior::ExecuteResult::Success;
    }
  }

  return behavior::ExecuteResult::Success;
}

uint16_t HSFlaggerShipMan::SelectShip(uint16_t current) {
  int num = rand() % 8;

  while (num == current) {
    num = rand() % 8;
  }
  return num;
}

behavior::ExecuteResult HSWarpToCenter::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  if (!bb.GetInCenter() && game.GetPlayer().frequency != 90 && game.GetPlayer().frequency != 91) {
    if (ctx.bot->GetTime().TimedActionDelay("warptocenter", 200)) {
      game.SetEnergy(100.0f);
      game.Warp();

      return behavior::ExecuteResult::Failure;
    }
  }
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSAttachNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  auto& hs_bb = ctx.bot->GetHSBlackboard();

  const AnchorPlayer& anchor = hs_bb.GetAnchor();

  if (anchor.p && (game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91)) {
    bool anchor_in_safe = game.GetMap().GetTileId(anchor.p->position) == kSafeTileId;

    if (!ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(), (MapCoord)anchor.p->position) &&
        !anchor_in_safe) {
      if (ctx.bot->GetTime().TimedActionDelay("attach", 50)) {
        game.SetEnergy(100.0f);
        game.SendChatMessage(":" + anchor.p->name + ":?attach");

        return behavior::ExecuteResult::Failure;
      }
    }
  }

  if (game.GetPlayer().attach_id != 65535) {
    if (bb.GetSwarm()) {
      game.SetEnergy(15.0f);
    }

    game.SendKey(VK_F7);

    return behavior::ExecuteResult::Failure;
  }

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSToggleNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  auto& hs_bb = ctx.bot->GetHSBlackboard();

  const AnchorPlayer anchor = hs_bb.GetAnchor();

  if (anchor.p && anchor.p->id == game.GetPlayer().id) {
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
      return behavior::ExecuteResult::Failure;
    }
  }

  if (has_cloak) {
    if (!cloaking) {
      if (ctx.bot->GetTime().TimedActionDelay("cloak", 800)) {
        game.Cloak(ctx.bot->GetKeys());

        return behavior::ExecuteResult::Failure;
      }
    }

    if (game.GetPlayer().ship == 6) {
      if (ctx.bot->GetTime().TimedActionDelay("anchorflashposition", 30000)) {
        game.Cloak(ctx.bot->GetKeys());

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
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSPatrolBaseNode::Execute(behavior::ExecuteContext& ctx) {
  return behavior::ExecuteResult::Success;
}

}  // namespace hs
}  // namespace marvin
