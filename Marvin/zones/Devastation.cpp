#include "Devastation.h"

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
namespace deva {

const std::vector<std::string> kBotNames = {"LilMarv", "MadMarv", "MarvMaster", "Baked Cake", "Marv1", "Marv2",   "Marv3",   "Marv4",      "Marv5",      "Marv6",
                                            "Marv7",   "Marv8",   "FrogBot"};



const std::vector<Vector2f> oldkBaseSafes0 = {
    Vector2f(512, 512),
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

const std::vector<Vector2f> oldkBaseSafes1 = {
    Vector2f(512, 512),
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





const std::vector<Vector2f> kBaseSafes0 = {
    Vector2f(512, 512),
    Vector2f(139, 420), Vector2f(259, 337),  Vector2f(341, 447), Vector2f(114, 455),  Vector2f(861, 871), 
    Vector2f(886, 393), Vector2f(793, 331),  Vector2f(721, 657), Vector2f(650, 747),  Vector2f(795, 752), // 10
    Vector2f(863, 801), Vector2f(716, 851),  Vector2f(838, 820), Vector2f(776, 968),  Vector2f(492, 600), 
    Vector2f(230, 969), Vector2f(302, 67),   Vector2f(552, 17),  Vector2f(698, 52),   Vector2f(798, 41), // 20
    Vector2f(188, 106), Vector2f(284, 172),  Vector2f(429, 100), Vector2f(507, 196),  Vector2f(759, 151), 
    Vector2f(56, 194),  Vector2f(196, 259),  Vector2f(347, 176), Vector2f(414, 229),  Vector2f(567, 312), // 30
    Vector2f(676, 216), Vector2f(764, 272),  Vector2f(814, 286), Vector2f(980, 231),  Vector2f(746, 377),
    Vector2f(62, 337),  Vector2f(135, 336),  Vector2f(125, 540), Vector2f(105, 607),  Vector2f(187, 645), // 40
    Vector2f(313, 570), Vector2f(601, 520),  Vector2f(831, 490), Vector2f(1008, 450), Vector2f(641, 590),
    Vector2f(878, 603), Vector2f(52, 715),   Vector2f(128, 720), Vector2f(353, 741),  Vector2f(198, 840), // 50
    Vector2f(335, 877), Vector2f(105, 11),   Vector2f(891, 151), Vector2f(957, 43),   Vector2f(944, 907),
    Vector2f(910, 987), Vector2f(418, 694),  Vector2f(57, 781),  Vector2f(630, 680),  Vector2f(517, 704), // 60
    Vector2f(623, 446), Vector2f(492, 877),  Vector2f(67, 968),  Vector2f(166, 1018), Vector2f(633, 900),
    Vector2f(394, 936), Vector2f(286, 1018), Vector2f(526, 942), Vector2f(613, 1002)}; // 69

const std::vector<Vector2f> kBaseSafes1 = {
    Vector2f(512, 512),
    Vector2f(297, 511),  Vector2f(311, 418),  Vector2f(341, 531), Vector2f(127, 523),  Vector2f(861, 957),
    Vector2f(1006, 387), Vector2f(930, 305),  Vector2f(821, 657), Vector2f(705, 821),  Vector2f(799, 756), // 10
    Vector2f(983, 801),  Vector2f(716, 923),  Vector2f(838, 852), Vector2f(808, 968),  Vector2f(475, 673),
    Vector2f(376, 902),  Vector2f(310, 67),   Vector2f(562, 17),  Vector2f(694, 56),   Vector2f(885, 128), // 20
    Vector2f(193, 201),  Vector2f(328, 172),  Vector2f(397, 200), Vector2f(620, 196),  Vector2f(849, 91),
    Vector2f(135, 230),  Vector2f(196, 267),  Vector2f(317, 293), Vector2f(448, 348),  Vector2f(514, 365), // 30
    Vector2f(676, 226),  Vector2f(844, 174),  Vector2f(814, 373), Vector2f(980, 295),  Vector2f(838, 468),
    Vector2f(72, 337),   Vector2f(93, 442),   Vector2f(125, 562), Vector2f(233, 607),  Vector2f(197, 645), // 40
    Vector2f(347, 679),  Vector2f(697, 520),  Vector2f(859, 518), Vector2f(1008, 536), Vector2f(649, 590),
    Vector2f(966, 683),  Vector2f(52, 727),   Vector2f(128, 806), Vector2f(353, 747),  Vector2f(243, 840), // 50
    Vector2f(455, 798),  Vector2f(35, 106),   Vector2f(962, 191), Vector2f(957, 121),  Vector2f(977, 919),
    Vector2f(1010, 987), Vector2f(410, 787),  Vector2f(67, 781),  Vector2f(666, 722),  Vector2f(527, 704), // 60
    Vector2f(633, 446),  Vector2f(611, 774),  Vector2f(77, 968),  Vector2f(176, 1018), Vector2f(654, 921),
    Vector2f(484, 900),  Vector2f(476, 1018), Vector2f(602, 917), Vector2f(718, 986)}; // 69

void DevastationBehaviorBuilder::CreateBehavior(Bot& bot) {
  bot.CreateBasePaths(kBaseSafes0, kBaseSafes1, bot.GetGame().GetSettings().ShipSettings[1].GetRadius() + 0.5f);

  auto is_anchor = std::make_unique<bot::IsAnchorNode>();
  auto bouncing_shot = std::make_unique<bot::BouncingShotNode>();
  auto DEVA_repel_enemy = std::make_unique<deva::DevaRepelEnemyNode>();
  auto DEVA_burst_enemy = std::make_unique<deva::DevaBurstEnemyNode>();
  auto patrol_base = std::make_unique<deva::DevaPatrolBaseNode>();
  auto patrol_center = std::make_unique<bot::PatrolNode>();
  auto DEVA_move_to_enemy = std::make_unique<deva::DevaMoveToEnemyNode>();
  auto TvsT_base_path = std::make_unique<bot::TvsTBasePathNode>();

  auto find_enemy_in_base = std::make_unique<bot::FindEnemyInBaseNode>();

  auto DEVA_toggle_status = std::make_unique<deva::DevaToggleStatusNode>();
  auto DEVA_attach = std::make_unique<deva::DevaAttachNode>();
  auto DEVA_warp = std::make_unique<deva::DevaWarpNode>();
  auto DEVA_freqman = std::make_unique<deva::DevaFreqMan>();
  auto team_sort = std::make_unique<bot::SortBaseTeams>();
  auto DEVA_set_region = std::make_unique<deva::DevaSetRegionNode>();

  auto move_method_selector = std::make_unique<behavior::SelectorNode>(DEVA_move_to_enemy.get());
  auto los_weapon_selector =
      std::make_unique<behavior::SelectorNode>(DEVA_burst_enemy.get(), DEVA_repel_enemy.get(), shoot_enemy_.get());
  auto parallel_shoot_enemy =
      std::make_unique<behavior::ParallelNode>(los_weapon_selector.get(), move_method_selector.get());

  auto path_to_enemy_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy_.get(), follow_path_.get());
  auto TvsT_base_path_sequence = std::make_unique<behavior::SequenceNode>(TvsT_base_path.get(), follow_path_.get());
  auto enemy_path_logic_selector = std::make_unique<behavior::SelectorNode>(
      mine_sweeper_.get(), TvsT_base_path_sequence.get(), path_to_enemy_sequence.get());

  auto anchor_los_parallel =
      std::make_unique<behavior::ParallelNode>(DEVA_burst_enemy.get(), TvsT_base_path_sequence.get());
  auto anchor_los_sequence = std::make_unique<behavior::SequenceNode>(is_anchor.get(), anchor_los_parallel.get());
  auto anchor_los_selector =
      std::make_unique<behavior::SelectorNode>(anchor_los_sequence.get(), parallel_shoot_enemy.get());

  auto los_shoot_conditional =
      std::make_unique<behavior::SequenceNode>(target_in_los_.get(), anchor_los_selector.get());
  auto bounce_path_parallel =
      std::make_unique<behavior::ParallelNode>(bouncing_shot.get(), enemy_path_logic_selector.get());
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

  auto action_selector = std::make_unique<behavior::SelectorNode>(find_enemy_selector.get(), patrol_selector.get());
  auto root_sequence = std::make_unique<behavior::SequenceNode>(
      commands_.get(), set_ship_.get(), set_freq_.get(), ship_check_.get(), team_sort.get(),
      DEVA_set_region.get(), DEVA_freqman.get(), DEVA_warp.get(), DEVA_attach.get(), respawn_check_.get(),
      DEVA_toggle_status.get(), action_selector.get());

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
  engine_->PushNode(std::move(anchor_los_parallel));
  engine_->PushNode(std::move(anchor_los_sequence));
  engine_->PushNode(std::move(anchor_los_selector));
  engine_->PushNode(std::move(bouncing_shot));
  engine_->PushNode(std::move(bounce_path_parallel));
  engine_->PushNode(std::move(TvsT_base_path));
  engine_->PushNode(std::move(TvsT_base_path_sequence));
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
  engine_->PushNode(std::move(DEVA_attach));
  engine_->PushNode(std::move(DEVA_warp));
  engine_->PushNode(std::move(DEVA_freqman));
  engine_->PushNode(std::move(team_sort));
  engine_->PushNode(std::move(DEVA_set_region));
  engine_->PushNode(std::move(action_selector));
}

behavior::ExecuteResult DevaSetRegionNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

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


          g_RenderState.RenderDebugText("  DevaSetRegionNode: %llu", timer.GetElapsedTime());
          return behavior::ExecuteResult::Success;
        }
      }
    }
  }

  g_RenderState.RenderDebugText("  DevaSetRegionNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult DevaFreqMan::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  bool dueling = game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01;

  if (bb.ValueOr<bool>("FreqLock", false) || (bb.ValueOr<bool>("IsAnchor", false) && dueling)) {
    g_RenderState.RenderDebugText("  DevaFreqMan: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  uint64_t offset = ctx.bot->GetTime().DevaFreqTimer(kBotNames);
  // uint64_t offset = ctx.bot->GetTime().UniqueIDTimer(game.GetPlayer().id);

  std::vector<uint16_t> fList = bb.ValueOr<std::vector<uint16_t>>("FreqList", std::vector<uint16_t>());

  // if player is on a non duel team size greater than 2, breaks the zone balancer
  if (game.GetPlayer().frequency != 00 && game.GetPlayer().frequency != 01) {
    if (fList[game.GetPlayer().frequency] > 1) {
      if (ctx.bot->GetTime().TimedActionDelay("freqchange", offset)) {
        game.SetEnergy(100.0f);
        game.SetFreq(FindOpenFreq(fList, 2));
      }

      g_RenderState.RenderDebugText("  DevaFreqMan: %llu", timer.GetElapsedTime());
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
              g_RenderState.RenderDebugText("  DevaFreqMan: %llu", timer.GetElapsedTime());
              return behavior::ExecuteResult::Failure;
            }
          }
        }

        if (ctx.bot->GetTime().TimedActionDelay("freqchange", offset)) {
          game.SetEnergy(100.0f);
          game.SetFreq(FindOpenFreq(fList, 2));
        }

        g_RenderState.RenderDebugText("  DevaFreqMan: %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Failure;
      }
    }
  }

  g_RenderState.RenderDebugText("  DevaFreqMan: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult DevaWarpNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

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

  g_RenderState.RenderDebugText("  DevaWarpNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult DevaAttachNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

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

          g_RenderState.RenderDebugText("  DevaAttachNode: %llu", timer.GetElapsedTime());
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

        g_RenderState.RenderDebugText("  DevaAttachNode: %llu", timer.GetElapsedTime());
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

    g_RenderState.RenderDebugText("  DevaAttachNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  g_RenderState.RenderDebugText("  DevaAttachNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

void DevaAttachNode::SetAttachTarget(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  std::vector<Vector2f> path = ctx.bot->GetBasePath();

  std::vector<Player> team_list = bb.ValueOr<std::vector<Player>>("TeamList", std::vector<Player>());
  std::vector<Player> combined_list = bb.ValueOr<std::vector<Player>>("CombinedList", std::vector<Player>());

   if (path.empty()) {
    if (!team_list.empty()) {
      game.SetSelectedPlayer(team_list[0].id);
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
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  if (bb.ValueOr<bool>("UseMultiFire", false)) {
    if (!game.GetPlayer().multifire_status) {
      game.MultiFire();

      g_RenderState.RenderDebugText("  DevaToggleStatusNode: %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    }
  } else {
    if (game.GetPlayer().multifire_status) {
      game.MultiFire();

      g_RenderState.RenderDebugText("  DevaToggleStatusNode: %llu", timer.GetElapsedTime());
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

      g_RenderState.RenderDebugText("  DevaToggleStatusNode: %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    }

    return behavior::ExecuteResult::Success;
  }
  if (!stealthing && has_stealth) {
    if (ctx.bot->GetTime().TimedActionDelay("stealth", 800)) {
      game.Stealth();

      g_RenderState.RenderDebugText("  DevaToggleStatusNode: %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    }

    return behavior::ExecuteResult::Success;
  }
  if (!cloaking && has_cloak) {
    if (ctx.bot->GetTime().TimedActionDelay("cloak", 800)) {
      game.Cloak(ctx.bot->GetKeys());

      g_RenderState.RenderDebugText("  DevaToggleStatusNode: %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    }

    g_RenderState.RenderDebugText("  DevaToggleStatusNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  g_RenderState.RenderDebugText("  DevaToggleStatusNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult DevaPatrolBaseNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  Vector2f enemy_safe = bb.ValueOr<Vector2f>("EnemySafe", Vector2f());
  Path path = bb.ValueOr<Path>("Path", Path());

  path = ctx.bot->GetPathfinder().CreatePath(path, game.GetPosition(), enemy_safe, game.GetShipSettings().GetRadius());

  bb.Set<Path>("Path", path);

  g_RenderState.RenderDebugText("  DevaPatrolBaseNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult DevaRepelEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  if (!bb.ValueOr<bool>("InCenter", true)) {
    if (bb.ValueOr<bool>("IsAnchor", false)) {
      if (game.GetEnergyPercent() < 20.0f && game.GetPlayer().repels > 0) {
        game.Repel(ctx.bot->GetKeys());

        g_RenderState.RenderDebugText("  DevaRepelEnemyNode: %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Success;
      }
    } else if (bb.ValueOr<bool>("UseRepel", false)) {
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
  auto& bb = ctx.blackboard;

  if (!bb.ValueOr<bool>("InCenter", true)) {
    const Player* target = bb.ValueOr<const Player*>("Target", nullptr);
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
  auto& bb = ctx.blackboard;

  float energy_pct = game.GetEnergy() / (float)game.GetShipSettings().MaximumEnergy;

  const Player* target = bb.ValueOr<const Player*>("Target", nullptr);

  if (!target) {
    g_RenderState.RenderDebugText("  DevaMoveToEnemyNode (no target): %llu", timer.GetElapsedTime());
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

  g_RenderState.RenderDebugText("  DevaMoveToEnemyNode: %llu", timer.GetElapsedTime());
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
