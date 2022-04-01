#include "Hyperspace.h"

#include <chrono>
#include <cstring>
#include <limits>

#include "../Bot.h"
#include "../Debug.h"
#include "../GameProxy.h"
#include "../Map.h"
#include "../RayCaster.h"
#include "../RegionRegistry.h"
#include "../Shooter.h"
#include "../platform/ContinuumGameProxy.h"
#include "../platform/Platform.h"

namespace marvin {
namespace hs {

std::vector<Vector2f> kBaseEntrances = {Vector2f(827, 339), Vector2f(811, 530), Vector2f(729, 893), Vector2f(444, 757),
                                        Vector2f(127, 848), Vector2f(268, 552), Vector2f(181, 330)};

std::vector<Vector2f> kFlagRooms = {Vector2f(826, 229), Vector2f(834, 540), Vector2f(745, 828), Vector2f(489, 832),
                                    Vector2f(292, 812), Vector2f(159, 571), Vector2f(205, 204)};

void HyperspaceBehaviorBuilder::CreateBehavior(Bot& bot) {

  bot.CreateBasePaths(kBaseEntrances, kFlagRooms, bot.GetGame().GetSettings().ShipSettings[6].GetRadius());
  
  uint16_t ship = bot.GetGame().GetPlayer().ship;

    std::vector<Vector2f> patrol_nodes = {Vector2f(585, 540), Vector2f(400, 570)};

    bot.GetBlackboard().Set<std::vector<Vector2f>>("PatrolNodes", patrol_nodes);
    bot.GetBlackboard().Set<uint16_t>("Freq", 999);
    bot.GetBlackboard().Set<uint16_t>("PubTeam0", 90);
    bot.GetBlackboard().Set<uint16_t>("PubTeam1", 91);
    bot.GetBlackboard().Set<uint16_t>("Ship", ship);
    bot.GetBlackboard().Set<Vector2f>("Spawn", Vector2f(512, 512));
  

  auto move_to_enemy = std::make_unique<bot::MoveToEnemyNode>();
  auto TvsT_base_path = std::make_unique<bot::TvsTBasePathNode>();
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
  auto TvsT_base_path_sequence = std::make_unique<behavior::SequenceNode>(TvsT_base_path.get(), follow_path_.get());
  auto enemy_path_logic_selector = std::make_unique<behavior::SelectorNode>(
      mine_sweeper_.get(), TvsT_base_path_sequence.get(), path_to_enemy_sequence.get());

  auto anchor_los_parallel = std::make_unique<behavior::ParallelNode>(TvsT_base_path_sequence.get());
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
      commands_.get(), set_ship_.get(), set_freq_.get(), ship_check_.get(), flagger_sort.get(),
      HS_set_region.get(), HS_defense_position.get(), HS_freqman.get(), HS_shipman.get(), HS_warp.get(),
      HS_attach.get(), respawn_check_.get(), HS_toggle.get(), action_selector.get());

  engine_->PushRoot(std::move(root_sequence));

  engine_->PushNode(std::move(move_method_selector));
  engine_->PushNode(std::move(los_weapon_selector));
  engine_->PushNode(std::move(parallel_shoot_enemy));
  engine_->PushNode(std::move(los_shoot_conditional));
  engine_->PushNode(std::move(path_to_enemy_sequence));
  engine_->PushNode(std::move(find_enemy_in_center_sequence));
  engine_->PushNode(std::move(move_to_enemy));
  engine_->PushNode(std::move(TvsT_base_path));
  engine_->PushNode(std::move(is_anchor));
  engine_->PushNode(std::move(anchor_los_parallel));
  engine_->PushNode(std::move(anchor_los_sequence));
  engine_->PushNode(std::move(anchor_los_selector));
  engine_->PushNode(std::move(bouncing_shot));
  engine_->PushNode(std::move(bounce_path_parallel));
  engine_->PushNode(std::move(TvsT_base_path_sequence));
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
  auto& bb = ctx.blackboard;

  std::size_t base_index = bb.ValueOr<std::size_t>("BaseIndex", 0);

  const Player* base_anchor = nullptr;
  const Player* other_anchor = nullptr;
  const Player* enemy_anchor = nullptr;

  std::vector<uint16_t> fList(100, 0);

  int flagger_count = 0;
  int team_spiders = 0;
  int team_lancs = 0;

  if (ctx.bot->GetTime().TimedActionDelay("sendchatmessage", 10000)) {
    game.SendChatMessage("?lancs");
  }

  for (ChatMessage chat : game.GetChat()) {
    if (chat.type == 0) {
      if (chat.message.compare(0, 3, "(S)") == 0 || chat.message.compare(0, 3, "(E)") == 0) {
        std::vector<std::string> anchors;
        std::string name;

        for (std::size_t i = 4; i < chat.message.size(); i++) {
          if (chat.message[i] == ' ') {
            anchors.push_back(name);
            name.clear();
            i += 3;
          } else {
            name.push_back(chat.message[i]);
          }
        }
      }
    }
  }

  for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
    const Player& player = game.GetPlayers()[i];

    if (player.frequency < 100) fList[player.frequency]++;

    if ((player.frequency == 90 || player.frequency == 91) && player.name[0] != '<') {
      flagger_count++;

      if (player.frequency == game.GetPlayer().frequency) {
        if (player.ship == 2) {
          team_spiders++;
        } else if (player.ship == 6) {
          team_lancs++;
          if (player.id != game.GetPlayer().id) {
            if (ctx.bot->GetRegions().IsConnected(player.position, kBaseEntrances[base_index])) {
              base_anchor = &game.GetPlayers()[i];
            } else {
              for (std::size_t i = 0; i < kBaseEntrances.size(); ++i) {
                if (ctx.bot->GetRegions().IsConnected(player.position, kBaseEntrances[i])) {
                  base_anchor = &game.GetPlayers()[i];
                }
              }
            }
            other_anchor = &game.GetPlayers()[i];
          }
        }
      } else if (ctx.bot->GetRegions().IsConnected(player.position, kBaseEntrances[base_index])) {
        if (player.ship == 6) {
          enemy_anchor = &game.GetPlayers()[i];
        } else {
          enemy_anchor = &game.GetPlayers()[i];
        }
      }
    }
  }

  if (base_anchor) {
    bb.Set<const Player*>("TeamAnchor", base_anchor);
  } else {
    bb.Set<const Player*>("TeamAnchor", other_anchor);
  }

  bb.Set<const Player*>("EnemyAnchor", enemy_anchor);
  bb.Set<std::vector<uint16_t>>("FreqList", fList);
  bb.Set<int>("FlaggerCount", flagger_count);
  bb.Set<int>("SpiderCount", team_spiders);
  bb.Set<int>("LancCount", team_lancs);

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSSetRegionNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  const std::vector<Vector2f>& entrances = kBaseEntrances;
  const std::vector<Vector2f>& flagrooms = kFlagRooms;

  bool in_center = ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(),
                                                     (MapCoord)bb.ValueOr<Vector2f>("Spawn", Vector2f(512, 512)));

  bb.Set<bool>("InCenter", in_center);

  if (!in_center && game.GetPlayer().active) {
    if (!ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(),
                                           (MapCoord)entrances[bb.ValueOr<std::size_t>("BaseIndex", 0)])) {
      for (std::size_t i = 0; i < entrances.size(); i++) {
        if (ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(), (MapCoord)entrances[i])) {
          bb.Set<std::size_t>("BaseIndex", i);

          // need to set anchor positions here?
          return behavior::ExecuteResult::Success;
        }
      }
    }
  }

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult HSDefensePositionNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  const std::vector<Vector2f>& entrances = kBaseEntrances;
  const std::vector<Vector2f>& flagrooms = kFlagRooms;
  std::size_t base_index = bb.ValueOr<std::size_t>("BaseIndex", 0);

  const Player* team_anchor = bb.ValueOr<const Player*>("TeamAnchor", nullptr);
  const Player* enemy_anchor = bb.ValueOr<const Player*>("EnemyAnchor", nullptr);

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
  auto& bb = ctx.blackboard;

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
  auto& bb = ctx.blackboard;

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
  auto& bb = ctx.blackboard;

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
  auto& bb = ctx.blackboard;

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
  auto& bb = ctx.blackboard;

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
