#include "Hyperspace.h"

#include <chrono>
#include <cstring>
#include <limits>

#include "Bot.h"
#include "Debug.h"
#include "GameProxy.h"
#include "Map.h"
#include "RayCaster.h"
#include "RegionRegistry.h"
#include "Shooter.h"
#include "platform/ContinuumGameProxy.h"
#include "platform/Platform.h"

namespace marvin {
namespace hs {

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

  Chat chat = game.GetChat();

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
            if (ctx.bot->GetRegions().IsConnected(player.position, ctx.bot->GetHSEntrance()[base_index])) {
              base_anchor = &game.GetPlayers()[i];
            } else {
              for (std::size_t i = 0; i < ctx.bot->GetHSEntrance().size(); ++i) {
                if (ctx.bot->GetRegions().IsConnected(player.position, ctx.bot->GetHSEntrance()[i])) {
                  base_anchor = &game.GetPlayers()[i];
                }
              }
            }
            other_anchor = &game.GetPlayers()[i];
          }
        }
      } else if (ctx.bot->GetRegions().IsConnected(player.position, ctx.bot->GetHSEntrance()[base_index])) {
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

  const std::vector<Vector2f>& entrances = ctx.bot->GetHSEntrance();
  const std::vector<Vector2f>& flagrooms = ctx.bot->GetHSFlagRoom();

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

  const std::vector<Vector2f>& entrances = ctx.bot->GetHSEntrance();
  const std::vector<Vector2f>& flagrooms = ctx.bot->GetHSFlagRoom();
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
                                              ctx.bot->GetHSEntrance()[bb.ValueOr<std::size_t>("BaseIndex", 0)])) {
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
