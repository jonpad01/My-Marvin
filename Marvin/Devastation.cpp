#include "Devastation.h"

#include <chrono>
#include <cstring>
#include <limits>

#include "Bot.h"
#include "Commands.h"
#include "Debug.h"
#include "GameProxy.h"
#include "Map.h"
#include "RayCaster.h"
#include "RegionRegistry.h"
#include "Shooter.h"
#include "platform/ContinuumGameProxy.h"
#include "platform/Platform.h"

namespace marvin {
namespace deva {

const std::vector<Vector2f> kBaseSafes0 = {
    Vector2f(32, 56),    Vector2f(185, 58),   Vector2f(247, 40),  Vector2f(383, 64),  Vector2f(579, 62),
    Vector2f(667, 56),   Vector2f(743, 97),   Vector2f(963, 51),  Vector2f(32, 111),  Vector2f(222, 82),
    Vector2f(385, 148),  Vector2f(499, 137),  Vector2f(624, 185), Vector2f(771, 189), Vector2f(877, 110),
    Vector2f(3, 154),    Vector2f(242, 195),  Vector2f(326, 171), Vector2f(96, 258),  Vector2f(182, 303),
    Vector2f(297, 287),  Vector2f(491, 296),  Vector2f(555, 324), Vector2f(764, 259), Vector2f(711, 274),
    Vector2f(904, 303),  Vector2f(170, 302),  Vector2f(147, 379), Vector2f(280, 310), Vector2f(425, 326),
    Vector2f(569, 397),  Vector2f(816, 326),  Vector2f(911, 347), Vector2f(97, 425),  Vector2f(191, 471),
    Vector2f(359, 445),  Vector2f(688, 392),  Vector2f(696, 545), Vector2f(854, 447), Vector2f(16, 558),
    Vector2f(245, 610),  Vector2f(270, 595),  Vector2f(5, 641),   Vector2f(381, 610), Vector2f(422, 634),
    Vector2f(565, 584),  Vector2f(726, 598),  Vector2f(859, 639), Vector2f(3, 687),   Vector2f(135, 710),
    Vector2f(287, 715),  Vector2f(414, 687),  Vector2f(612, 713), Vector2f(726, 742), Vector2f(1015, 663),
    Vector2f(50, 806),   Vector2f(160, 841),  Vector2f(310, 846), Vector2f(497, 834), Vector2f(585, 861),
    Vector2f(737, 755),  Vector2f(800, 806),  Vector2f(913, 854), Vector2f(91, 986),  Vector2f(175, 901),
    Vector2f(375, 887),  Vector2f(499, 909),  Vector2f(574, 846), Vector2f(699, 975), Vector2f(864, 928),
    Vector2f(221, 1015), Vector2f(489, 1008), Vector2f(769, 983), Vector2f(948, 961), Vector2f(947, 511),
    Vector2f(580, 534)};

const std::vector<Vector2f> kBaseSafes1 = {
    Vector2f(102, 56),   Vector2f(189, 58),  Vector2f(373, 40),   Vector2f(533, 64),  Vector2f(606, 57),
    Vector2f(773, 56),   Vector2f(928, 46),  Vector2f(963, 47),   Vector2f(140, 88),  Vector2f(280, 82),
    Vector2f(395, 148),  Vector2f(547, 146), Vector2f(709, 117),  Vector2f(787, 189), Vector2f(993, 110),
    Vector2f(181, 199),  Vector2f(258, 195), Vector2f(500, 171),  Vector2f(100, 258), Vector2f(280, 248),
    Vector2f(446, 223),  Vector2f(524, 215), Vector2f(672, 251),  Vector2f(794, 259), Vector2f(767, 274),
    Vector2f(972, 215),  Vector2f(170, 336), Vector2f(170, 414),  Vector2f(314, 391), Vector2f(455, 326),
    Vector2f(623, 397),  Vector2f(859, 409), Vector2f(1013, 416), Vector2f(97, 502),  Vector2f(228, 489),
    Vector2f(365, 445),  Vector2f(770, 392), Vector2f(786, 545),  Vector2f(903, 552), Vector2f(122, 558),
    Vector2f(245, 650),  Vector2f(398, 595), Vector2f(130, 640),  Vector2f(381, 694), Vector2f(422, 640),
    Vector2f(626, 635),  Vector2f(808, 598), Vector2f(1004, 583), Vector2f(138, 766), Vector2f(241, 795),
    Vector2f(346, 821),  Vector2f(525, 687), Vector2f(622, 703),  Vector2f(862, 742), Vector2f(907, 769),
    Vector2f(98, 806),   Vector2f(295, 864), Vector2f(453, 849),  Vector2f(504, 834), Vector2f(682, 792),
    Vector2f(746, 810),  Vector2f(800, 812), Vector2f(1004, 874), Vector2f(57, 909),  Vector2f(323, 901),
    Vector2f(375, 996),  Vector2f(503, 909), Vector2f(694, 846),  Vector2f(720, 895), Vector2f(881, 945),
    Vector2f(272, 1015), Vector2f(617, 929), Vector2f(775, 983),  Vector2f(948, 983), Vector2f(1012, 494),
    Vector2f(669, 497)};

void Initialize(Bot& bot) {
  debug_log << "Initializing devastation bot." << std::endl;
  bot.CreateBasePaths(kBaseSafes0, kBaseSafes1, bot.GetGame().GetSettings().ShipSettings[1].GetRadius() + 0.5f);
}

behavior::ExecuteResult DevaSetRegionNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  bool in_center = ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(),
                                                     (MapCoord)bb.ValueOr<Vector2f>("Spawn", Vector2f(512, 512)));

  bb.Set<bool>("InCenter", in_center);

  if (!in_center && game.GetPlayer().active) {
    if (!ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(),
                                           (MapCoord)kBaseSafes0[bb.ValueOr<std::size_t>("BaseIndex", 0)])) {
      for (std::size_t i = 0; i < kBaseSafes0.size(); i++) {
        if (ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(), (MapCoord)kBaseSafes0[i])) {
          bb.Set<std::size_t>("BaseIndex", i);
          if (game.GetPlayer().frequency == 00) {
            bb.Set<Vector2f>("TeamSafe", kBaseSafes0[i]);
            bb.Set<Vector2f>("EnemySafe", kBaseSafes1[i]);
          } else if (game.GetPlayer().frequency == 01) {
            bb.Set<Vector2f>("TeamSafe", kBaseSafes1[i]);
            bb.Set<Vector2f>("EnemySafe", kBaseSafes0[i]);
          }

          return behavior::ExecuteResult::Success;
        }
      }
    }
  }

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult DevaFreqMan::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  bool dueling = game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01;

  if (bb.ValueOr<bool>("FreqLock", false) || (bb.ValueOr<bool>("IsAnchor", false) && dueling)) {
    return behavior::ExecuteResult::Success;
  }

  uint64_t offset = ctx.bot->GetTime().DevaFreqTimer(ctx.bot->GetBotNames());
  // uint64_t offset = ctx.bot->GetTime().UniqueIDTimer(game.GetPlayer().id);

  std::vector<uint16_t> fList = bb.ValueOr<std::vector<uint16_t>>("FreqList", std::vector<uint16_t>());

  // if player is on a non duel team size greater than 2, breaks the zone balancer
  if (game.GetPlayer().frequency != 00 && game.GetPlayer().frequency != 01) {
    if (fList[game.GetPlayer().frequency] > 1) {
      if (ctx.bot->GetTime().TimedActionDelay("freqchange", offset)) {
        game.SetEnergy(100.0f);
        game.SetFreq(FindOpenFreq(fList, 2));
      }

      return behavior::ExecuteResult::Failure;
    }
  }

  // duel team is uneven
  if (fList[0] != fList[1]) {
    if (!dueling) {
      if (ctx.bot->GetTime().TimedActionDelay("freqchange", offset)) {
        game.SetEnergy(100.0f);
        if (fList[0] < fList[1]) {
          game.SetFreq(0);
        } else {
          // get on freq 1
          game.SetFreq(1);
        }
      }

      return behavior::ExecuteResult::Failure;
    } else if (bb.ValueOr<bool>("InCenter", true) ||
               bb.ValueOr<std::vector<Player>>("TeamList", std::vector<Player>()).size() == 0 ||
               bb.ValueOr<bool>("TeamInBase", false)) {  // bot is on a duel team
      if ((game.GetPlayer().frequency == 00 && fList[0] > fList[1]) ||
          (game.GetPlayer().frequency == 01 && fList[0] < fList[1])) {
        // look for players not on duel team, make bot wait longer to see if the other player will get in
        for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
          const Player& player = game.GetPlayers()[i];

          if (player.frequency > 01 && player.frequency < 100) {
            float energy_pct =
                (player.energy / (float)game.GetSettings().ShipSettings[player.ship].MaximumEnergy) * 100.0f;
            if (energy_pct != 100.0f) {
              return behavior::ExecuteResult::Failure;
            }
          }
        }

        if (ctx.bot->GetTime().TimedActionDelay("freqchange", offset)) {
          game.SetEnergy(100.0f);
          game.SetFreq(FindOpenFreq(fList, 2));
        }

        return behavior::ExecuteResult::Failure;
      }
    }
  }

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult DevaWarpNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;
  auto& time = ctx.bot->GetTime();

  uint64_t base_empty_timer = 30000;

  if (!bb.ValueOr<bool>("InCenter", true)) {
    if (!bb.ValueOr<bool>("EnemyInBase", false)) {
      if (time.TimedActionDelay("baseemptywarp", base_empty_timer)) {
        game.SetEnergy(100.0f);
        game.Warp();
      }
    }
  }

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult DevaAttachNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  // if dueling then run checks for attaching
  if (game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01) {
    if (bb.ValueOr<bool>("TeamInBase", false)) {
      if (bb.ValueOr<bool>("GetSwarm", false)) {
        if (!game.GetPlayer().active) {
          if (ctx.bot->GetTime().TimedActionDelay("respawn", 300)) {
            int ship = 1;

            if (game.GetPlayer().ship == 1) {
              ship = 7;
            } else {
              ship = 1;
            }

            game.SetEnergy(100.0f);

            if (!game.SetShip(ship)) {
              ctx.bot->GetTime().TimedActionDelay("respawn", 0);
            }
          }

          return behavior::ExecuteResult::Failure;
        }
      }

      // if this check is valid the bot will halt here untill it attaches
      if (bb.ValueOr<bool>("InCenter", true)) {
        SetAttachTarget(ctx);

        // checks if energy is full
        if (CheckStatus(game, ctx.bot->GetKeys(), true)) {
          game.F7();
        }

        return behavior::ExecuteResult::Failure;
      }
    }
  }

  // if attached to a player detach
  if (game.GetPlayer().attach_id != 65535) {
    if (bb.ValueOr<bool>("Swarm", false)) {
      game.SetEnergy(15.0f);
    }

    game.F7();

    return behavior::ExecuteResult::Failure;
  }

  return behavior::ExecuteResult::Success;
}

void DevaAttachNode::SetAttachTarget(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  std::vector<Vector2f> path = ctx.bot->GetBasePath();

  if (path.empty()) {
    return;
  }

  std::vector<Player> team_list = bb.ValueOr<std::vector<Player>>("TeamList", std::vector<Player>());
  std::vector<Player> combined_list = bb.ValueOr<std::vector<Player>>("CombinedList", std::vector<Player>());

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
  for (std::size_t i = 0; i < combined_list.size(); i++) {
    const Player& player = combined_list[i];
    // bool player_in_center = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));
    bool player_in_base = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, MapCoord(path[0]));

    if (player_in_base) {
      // if (!player_in_center && IsValidPosition(player.position) && player.ship < 8) {

      float distance_to_team = PathLength(path, player.position, bb.ValueOr<Vector2f>("TeamSafe", Vector2f()));
      float distance_to_enemy = PathLength(path, player.position, bb.ValueOr<Vector2f>("EnemySafe", Vector2f()));

      if (player.frequency == game.GetPlayer().frequency) {
        // float distance_to_team = ctx.deva->PathLength(player.position, ctx.deva->GetTeamSafe());
        // float distance_to_enemy = ctx.deva->PathLength(player.position, ctx.deva->GetEnemySafe());

        if (distance_to_team < closest_team_distance_to_team) {
          closest_team_distance_to_team = distance_to_team;
          closest_team_to_team = bb.ValueOr<std::vector<Player>>("CombinedList", std::vector<Player>())[i].id;
        }
        if (distance_to_enemy < closest_team_distance_to_enemy) {
          closest_team_distance_to_enemy = distance_to_enemy;
          closest_team_to_enemy = bb.ValueOr<std::vector<Player>>("CombinedList", std::vector<Player>())[i].id;
        }
      } else {
        // float distance_to_team = ctx.deva->PathLength(player.position, ctx.deva->GetTeamSafe());
        // float distance_to_enemy = ctx.deva->PathLength(player.position, ctx.deva->GetEnemySafe());

        if (distance_to_team < closest_enemy_distance_to_team) {
          closest_enemy_distance_to_team = distance_to_team;
          closest_enemy_to_team = player.position;
        }
        if (distance_to_enemy < closest_enemy_distance_to_enemy) {
          closest_enemy_distance_to_enemy = distance_to_enemy;
          closest_enemy_to_enemy = bb.ValueOr<std::vector<Player>>("CombinedList", std::vector<Player>())[i].id;
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

  if (team_leak || enemy_leak || bb.ValueOr<bool>("IsAnchor", false)) {
    game.SetSelectedPlayer(closest_team_to_team);
    return;
  }

  // find closest team player to the enemy closest to team safe
  for (std::size_t i = 0; i < team_list.size(); i++) {
    const Player& player = team_list[i];

    // bool player_in_center = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));

    bool player_in_base = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, MapCoord(path[0]));

    if (player_in_base) {
      // as long as the player is not in center and has a valid position, it will hold its position for a moment after
      // death if (!player_in_center && IsValidPosition(player.position) && player.ship < 8) {

      // float distance_to_enemy_position = ctx.deva->PathLength(player.position, closest_enemy_to_team);

      float distance_to_enemy_position = 0.0f;

      distance_to_enemy_position = PathLength(path, player.position, closest_enemy_to_team);

      // get the closest player
      if (distance_to_enemy_position < closest_distance) {
        closest_distance = distance_to_enemy_position;
        game.SetSelectedPlayer(player.id);
      }
    }
  }
}

behavior::ExecuteResult DevaToggleStatusNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  if (bb.ValueOr<bool>("UseMultiFire", false)) {
    if (!game.GetPlayer().multifire_status) {
      game.MultiFire();

      return behavior::ExecuteResult::Failure;
    }
  } else {
    if (game.GetPlayer().multifire_status) {
      game.MultiFire();

      return behavior::ExecuteResult::Failure;
    }
  }

  // RenderText(text.c_str(), GetWindowCenter() - Vector2f(0, 200), RGB(100, 100, 100), RenderText_Centered);

  bool x_active = (game.GetPlayer().status & 4) != 0;
  bool stealthing = (game.GetPlayer().status & 1) != 0;
  bool cloaking = (game.GetPlayer().status & 2) != 0;

  bool has_xradar = (game.GetShipSettings().XRadarStatus & 3) != 0;
  bool has_stealth = (game.GetShipSettings().StealthStatus & 1) != 0;
  bool has_cloak = (game.GetShipSettings().CloakStatus & 3) != 0;

  // in deva these are free so just turn them on
  if (!x_active && has_xradar) {
    if (ctx.bot->GetTime().TimedActionDelay("xradar", 800)) {
      game.XRadar();

      return behavior::ExecuteResult::Failure;
    }

    return behavior::ExecuteResult::Success;
  }
  if (!stealthing && has_stealth) {
    if (ctx.bot->GetTime().TimedActionDelay("stealth", 800)) {
      game.Stealth();

      return behavior::ExecuteResult::Failure;
    }

    return behavior::ExecuteResult::Success;
  }
  if (!cloaking && has_cloak) {
    if (ctx.bot->GetTime().TimedActionDelay("cloak", 800)) {
      game.Cloak(ctx.bot->GetKeys());

      return behavior::ExecuteResult::Failure;
    }

    return behavior::ExecuteResult::Success;
  }

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult DevaPatrolBaseNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  Vector2f enemy_safe = bb.ValueOr<Vector2f>("EnemySafe", Vector2f());
  Path path = bb.ValueOr<Path>("Path", Path());

  path = ctx.bot->GetPathfinder().CreatePath(path, game.GetPosition(), enemy_safe, game.GetShipSettings().GetRadius());

  bb.Set<Path>("Path", path);

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult DevaRepelEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  if (!bb.ValueOr<bool>("InCenter", true)) {
    if (bb.ValueOr<bool>("IsAnchor", false)) {
      if (game.GetEnergyPercent() < 20.0f && game.GetPlayer().repels > 0) {
        game.Repel(ctx.bot->GetKeys());

        return behavior::ExecuteResult::Success;
      }
    } else if (bb.ValueOr<bool>("UseRepel", false)) {
      if (game.GetEnergyPercent() < 10.0f && game.GetPlayer().repels > 0 && game.GetPlayer().bursts == 0) {
        game.Repel(ctx.bot->GetKeys());

        return behavior::ExecuteResult::Success;
      }
    }
  }

  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult DevaBurstEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  if (!bb.ValueOr<bool>("InCenter", true)) {
    const Player* target = bb.ValueOr<const Player*>("Target", nullptr);
    if (!target) {
      return behavior::ExecuteResult::Failure;
    }

    if ((target->position.Distance(game.GetPosition()) < 7.0f || game.GetEnergyPercent() < 10.0f) &&
        game.GetPlayer().bursts > 0) {
      game.Burst(ctx.bot->GetKeys());

      return behavior::ExecuteResult::Success;
    }
  }

  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult DevaMoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  float energy_pct = game.GetEnergy() / (float)game.GetShipSettings().MaximumEnergy;

  const Player* target = bb.ValueOr<const Player*>("Target", nullptr);

  if (!target) {
    return behavior::ExecuteResult::Failure;
  }

  float player_energy_pct = target->energy / (float)game.GetShipSettings().MaximumEnergy;

  bool in_safe = game.GetMap().GetTileId(game.GetPlayer().position) == marvin::kSafeTileId;

  Vector2f shot_position = bb.ValueOr<Vector2f>("Solution", Vector2f());

  float hover_distance = 0.0f;

  if (in_safe) {
    hover_distance = 0.0f;
  } else {
    if (bb.ValueOr<bool>("InCenter", true)) {
      float diff = (energy_pct - player_energy_pct) * 10.0f;
      hover_distance = 10.0f - diff;

      if (hover_distance < 0.0f) hover_distance = 0.0f;
    } else {
      if (bb.ValueOr<bool>("IsAnchor", false)) {
        hover_distance = 20.0f;
      } else {
        hover_distance = 7.0f;
      }
    }
  }

  ctx.bot->Move(shot_position, hover_distance);

  ctx.bot->GetSteering().Face(shot_position);

  return behavior::ExecuteResult::Success;
}

bool DevaMoveToEnemyNode::IsAimingAt(GameProxy& game, const Player& shooter, const Player& target, Vector2f* dodge) {
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

}  // namespace deva
}  // namespace marvin
