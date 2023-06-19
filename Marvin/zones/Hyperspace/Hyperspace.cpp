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
  auto HS_shipman = std::make_unique<hs::HSShipMan>();
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
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  auto& hs_bb = ctx.bot->GetHSBlackboard();

     // slow this node down
  if (!ctx.bot->GetTime().TimedActionDelay("hssetregioncooldown", 50)) {
    return behavior::ExecuteResult::Success;
  }

  // std::size_t base_index = bb.ValueOr<std::size_t>("BaseIndex", 0);
  std::size_t base_index = ctx.bot->GetBasePaths().GetBaseIndex();
  const TeamGoals& goals = ctx.bot->GetTeamGoals().GetGoals();

   const Player* enemy_anchor = nullptr;

  AnchorSet anchors = GetAnchors(*ctx.bot);
  const Player* anchor = SelectAnchor(anchors, *ctx.bot);

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
        } else if (!enemy_anchor && player.ship == 2 || player.ship == 4) {
          enemy_anchor = &game.GetPlayers()[i];
        }
      }
    }
  }

  hs_bb.SetAnchor(anchor);
  hs_bb.SetEnemyAnchor(enemy_anchor);
  hs_bb.SetFlaggerCount(flagger_count);
  hs_bb.SetTeamLancCount(lanc_count);
  hs_bb.SetTeamSpidCount(spid_count);

  return behavior::ExecuteResult::Success;
}

AnchorSet HSFlaggerSort::GetAnchors(Bot& bot) {
  auto& game = bot.GetGame();

  AnchorSet anchors;
  uint16_t ship = game.GetPlayer().ship;
  bool evoker_usable = ship <= 1 || ship == 4 && ship == 5;
  bool msg_found = false;

  if (bot.GetTime().TimedActionDelay("sendchatmessage", 10000)) {
    game.SendChatMessage("?lancs");
  }

  //                 (summoner)      (evoker)   (in a lanc but no summoner/evoker)
  // example format: (S) Baked Cake, (E) marv1, marv2
  for (ChatMessage chat : game.GetChat()) {
    // make sure message is a server message
    if (chat.type != ChatType::Arena) {
      continue;
    }
    if (msg_found) {
      break;
    }

    std::vector<std::string_view> list = SplitString(chat.message, ",");

    for (std::size_t i = 0; i < list.size(); i++) {
      std::string_view entry = list[i];
      std::vector<const Player*>* type_list = &anchors.no_energy;
      std::string_view name = entry;

      if (entry.size() > 4 && entry[0] == '(' && entry[2] == ')') {
        name = entry.substr(4);

        if (entry[1] == 'S' || (entry[1] == 'E' && evoker_usable)) {
          type_list = &anchors.full_energy;
        }
      }

      const Player* anchor = game.GetPlayerByName(name);

      if (anchor) {
        type_list->push_back(anchor);
        if (i == list.size() - 1) {
          msg_found = true;
        }
      } else {
        anchors.Clear();
        break;
      }
    }
  }
  return anchors;
}

const Player* HSFlaggerSort::SelectAnchor(const AnchorSet& anchors, Bot& bot) {
  auto& game = bot.GetGame();

  const Player* anchor = nullptr;

  anchor = FindAnchorInBase(anchors.full_energy, bot);
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

const Player* HSFlaggerSort::FindAnchorInBase(const std::vector<const Player*>& anchors, Bot& bot) {
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

    for (int i = 0; i < bot.GetBlackboard().GetEnemyList().size(); i++) {
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

const Player* HSFlaggerSort::FindAnchorInAnyBase(const std::vector<const Player*>& anchors, Bot& bot) {
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

const Player* HSFlaggerSort::FindAnchorInTunnel(const std::vector<const Player*>& anchors, Bot& bot) {
  auto& game = bot.GetGame();

  const Player* anchor = nullptr;

  for (std::size_t i = 0; i < anchors.size(); i++) {
    const Player* player = anchors[i];
    if (bot.GetRegions().IsConnected(player->position, MapCoord(16, 16))) {
      anchor = player;
      break;
    }
  }
  return anchor;
}

const Player* HSFlaggerSort::FindAnchorInCenter(const std::vector<const Player*>& anchors, Bot& bot) {
  auto& game = bot.GetGame();

  const Player* anchor = nullptr;

  for (std::size_t i = 0; i < anchors.size(); i++) {
    const Player* player = anchors[i];
    if (bot.GetRegions().IsConnected(player->position, MapCoord(512, 512))) {
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

  // slow this node down
  if (!ctx.bot->GetTime().TimedActionDelay("hssetregioncooldown", 50)) {
    return behavior::ExecuteResult::Success;
  }

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
  auto ns = path::PathNodeSearch::Create(*ctx.bot, ctx.bot->GetBasePath(), 100);

  const std::vector<MapCoord>& entrances = ctx.bot->GetTeamGoals().GetGoals().entrances;
  const std::vector<MapCoord>& flagrooms = ctx.bot->GetTeamGoals().GetGoals().flag_rooms;
  std::size_t base_index = ctx.bot->GetBasePaths().GetBaseIndex();

  const Player* team_anchor = hs_bb.GetAnchor();
  const Player* enemy_anchor = hs_bb.GetEnemyAnchor();

  if (team_anchor && enemy_anchor) {
    if (ctx.bot->GetRegions().IsConnected((MapCoord)team_anchor->position, (MapCoord)entrances[base_index])) {
      if (ctx.bot->GetRegions().IsConnected((MapCoord)enemy_anchor->position, (MapCoord)entrances[base_index])) {
        std::size_t team_node = FindPathIndex(ctx.bot->GetBasePath(), team_anchor->position);
        std::size_t enemy_node = FindPathIndex(ctx.bot->GetBasePath(), enemy_anchor->position);

        if (team_node < enemy_node) {
          bb.Set<Vector2f>("TeamSafe", entrances[base_index]);
          bb.Set<Vector2f>("EnemySafe", flagrooms[base_index]);
        } else {
          bb.Set<Vector2f>("TeamSafe", flagrooms[base_index]);
          bb.Set<Vector2f>("EnemySafe", entrances[base_index]);
        }
      }
    }
  }

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSFreqMan::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  uint64_t unique_timer = ctx.bot->GetTime().UniqueIDTimer(game.GetPlayer().id);

  bool flagging = game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91;

  // join a flag team
  if (bb.ValueOr("FlaggerCount", 0) < 14 && !flagging) {
    if (ctx.bot->GetTime().TimedActionDelay("joingame", unique_timer)) {
      game.SetEnergy(100.0f);
      game.Flag();

      return behavior::ExecuteResult::Failure;
    }
  }

  // leave a flag team
  if (bb.ValueOr("FlaggerCount", 0) > 16 && flagging && game.GetPlayer().ship != 6) {
    if (ctx.bot->GetTime().TimedActionDelay("leavegame", unique_timer)) {
      game.SetEnergy(100.0f);
      game.SetFreq(FindOpenFreq(bb.ValueOr("FreqList", std::vector<uint16_t>()), 0));

      return behavior::ExecuteResult::Failure;
    }
  }

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSShipMan::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  int lanc_count = bb.ValueOr<int>("LancCount", 0);
  int spider_count = bb.ValueOr<int>("SpiderCount", 0);
  uint16_t last_ship = bb.ValueOr<uint16_t>("LastShip", 0);

  uint64_t unique_timer = ctx.bot->GetTime().UniqueIDTimer(game.GetPlayer().id);
  // return behavior::ExecuteResult::Success;

  if (game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91) {
    // switch to lanc if team doesnt have one
    if (lanc_count == 0 && game.GetPlayer().ship != 6) {
      if (ctx.bot->GetTime().TimedActionDelay("switchtoanchor", unique_timer)) {
        // try to remember what ship it was in
        if (!bb.Has("LastShip")) {
          bb.Set<uint16_t>("LastShip", game.GetPlayer().ship);
        }

        bb.Set<uint16_t>("Ship", 6);
      }
    }

    // switch to spider if team doesnt have one
    if (spider_count == 0 && game.GetPlayer().ship != 6 && game.GetPlayer().ship != 2) {
      if (ctx.bot->GetTime().TimedActionDelay("switchtospider", unique_timer)) {
        bb.Set<uint16_t>("LastShip", game.GetPlayer().ship);
        bb.Set<uint16_t>("Ship", 2);
      }
    }

    // if there is more than one lanc on the team, switch back to original ship
    if (lanc_count > 1 && game.GetPlayer().ship == 6) {
      const Player* anchor = bb.ValueOr("TeamAnchor", nullptr);

      if (anchor) {
        if (ctx.bot->GetRegions().IsConnected(anchor->position,
                                              kBaseEntrances[bb.ValueOr<std::size_t>("BaseIndex", 0)])) {
          if (ctx.bot->GetTime().TimedActionDelay("switchtolast", unique_timer)) {
            bb.Set<uint16_t>("Ship", last_ship);
            bb.Erase("LastShip");
          }
        }
      }
    }

    // if there is more than two spiders on the team, switch back to original ship
    if (spider_count > 2 && game.GetPlayer().ship == 2) {
      if (ctx.bot->GetTime().TimedActionDelay("switchtolast", unique_timer)) {
        bb.Set<uint16_t>("Ship", last_ship);
        bb.Erase("LastShip");
      }
    }
  }

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSWarpToCenter::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  if (!bb.ValueOr<bool>("InCenter", true) && game.GetPlayer().frequency != 90 && game.GetPlayer().frequency != 91) {
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

  const Player* anchor = bb.ValueOr("TeamAnchor", nullptr);

  if (anchor && (game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91)) {
    bool anchor_in_safe = game.GetMap().GetTileId(anchor->position) == kSafeTileId;

    if (!ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(), (MapCoord)anchor->position) &&
        !anchor_in_safe) {
      if (ctx.bot->GetTime().TimedActionDelay("attach", 50)) {
        game.SetEnergy(100.0f);
        game.Attach(anchor->name);

        return behavior::ExecuteResult::Failure;
      }
    }
  }

  if (game.GetPlayer().attach_id != 65535) {
    if (bb.ValueOr<bool>("Swarm", false)) {
      game.SetEnergy(15.0f);
    }

    game.F7();

    return behavior::ExecuteResult::Failure;
  }

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSToggleNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  if (game.GetPlayer().ship == 6 || game.GetPlayer().ship == 2) {
    bb.Set<bool>("IsAnchor", true);
  } else {
    bb.Set<bool>("IsAnchor", false);
  }

  if (bb.ValueOr<bool>("UseMultiFire", true)) {
    if (!game.GetPlayer().multifire_status && game.GetPlayer().multifire_capable) {
      game.MultiFire();
      return behavior::ExecuteResult::Failure;
    }
  } else if (game.GetPlayer().multifire_status && game.GetPlayer().multifire_capable) {
    game.MultiFire();
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
