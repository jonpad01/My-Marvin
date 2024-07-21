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
#include "BaseDuel.h"

namespace marvin {
namespace deva {

void BaseDuelBehaviorBuilder::CreateBehavior(Bot& bot) {

  auto spectator_lock = std::make_unique<bot::SpectatorLockNode>();

  auto startbd = std::make_unique<deva::StartBDNode>();

  auto pausebd = std::make_unique<deva::PauseBDNode>();

  auto runbd = std::make_unique<deva::RunBDNode>();

  auto endbd = std::make_unique<deva::EndBDNode>();




  auto bd_selector = std::make_unique<behavior::SelectorNode>(startbd.get(), runbd.get(), endbd.get());

  auto root_sequence = std::make_unique<behavior::SequenceNode>(commands_.get(), spectator_lock.get(), pausebd.get(), bd_selector.get());

  engine_->PushRoot(std::move(root_sequence));
  engine_->PushNode(std::move(bd_selector));

  engine_->PushNode(std::move(spectator_lock));
  engine_->PushNode(std::move(startbd));
  engine_->PushNode(std::move(pausebd));
  engine_->PushNode(std::move(runbd));
}

behavior::ExecuteResult StartBDNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  g_RenderState.RenderDebugText("  DevaStartDBNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult PauseBDNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  g_RenderState.RenderDebugText("  DevaPauseDBNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult EndBDNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  g_RenderState.RenderDebugText("  DevaEndDBNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}




behavior::ExecuteResult RunBDNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  const TeamGoals& warp = ctx.bot->GetTeamGoals().GetGoals();

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  uint64_t respawn_time = game.GetRespawnTime();

  uint64_t current_time = ctx.bot->GetTime().GetTime();
  // uint64_t warp_time = bb.ValueOr<uint64_t>("BDWarpCooldown", 0);
  uint64_t warp_time = bb.GetBDWarpCoolDown();

  // wait a second after warping players to base
  if (current_time - warp_time < 1000) {
    g_RenderState.RenderDebugText("  DevaRunDBNode(WaitingAfterWarp): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  // check if the bot wants to warp players to center
  if (bb.ValueOr<BDWarpTo>("bdwarpto", BDWarpTo::None) == BDWarpTo::Center) {
    // if (bb.GetWarpToState() == BDWarpTo::Center) {
    if (ctx.bot->GetTime().TimedActionDelay("BDWarpToCenter", 5000)) {
      WarpAllToCenter(ctx);
      // bb.SetWarpToState(BDWarpTo::None);
      bb.Set<BDWarpTo>("bdwarpto", BDWarpTo::None);
    }
    g_RenderState.RenderDebugText("  DevaRunDBNode(WarpToCenter): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  // check if the bot wants to warp players to a base
  // if (bb.ValueOr<bool>("BDWarpToBase", false)) {
  if (bb.ValueOr<BDWarpTo>("bdwarpto", BDWarpTo::None) == BDWarpTo::Base) {
    if (ctx.bot->GetTime().TimedActionDelay("BDWarpToBase", 5000)) {
      WarpAllToBase(ctx);
      // bb.Set<bool>("BDWarpToBase", false);
      bb.Set<BDWarpTo>("bdwarpto", BDWarpTo::None);
    }
    g_RenderState.RenderDebugText("  DevaRunDBNode(WarpToBase): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  if (bb.GetBDState() == BDState::Stopped || bb.GetBDState() == BDState::Paused) {
    g_RenderState.RenderDebugText("  DevaRunDBNode(Stopped/Paused): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  // check if the runBD flag is true
  // if (!bb.ValueOr<bool>("RunBD", false)) {
  // check if the game was manually stopped
  // if (bb.ValueOr<bool>("BDManualStop", false)) {
  if (bb.GetBDState() == BDState::Ended) {
    PrintFinalScore(ctx);
    ClearScore(ctx);
    // bb.Set<bool>("BDManualStop", false);
    // bb.Set<bool>("BDWarpToCenter", true);
    bb.Set<BDWarpTo>("bdwarpto", BDWarpTo::Center);
    bb.SetBDState(BDState::Stopped);
  }

  // check the score for win conditions
  // int team0_score = bb.ValueOr<int>("Team0Score", 0);
  // int team1_score = bb.ValueOr<int>("Team1Score", 0);
  int team0_score = bb.GetTeam0Score();
  int team1_score = bb.GetTeam1Score();

  bool end_condition =
      (team0_score >= 5 && (team0_score - team1_score) >= 2) || (team1_score >= 5 && (team1_score - team0_score) >= 2);

  if (end_condition) {
    // PrintFinalScore(ctx);
    // ClearScore(ctx);
    // bb.Set<bool>("RunBD", false);
    // bb.Set<bool>("BDWarpToCenter", true);
    bb.SetBDState(BDState::Ended);
    g_RenderState.RenderDebugText("  DevaRunDBNode(Game Ended): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  // set conditions for scoring points
  bool team0_dead = true;
  bool team1_dead = true;
  bool team0_on_safe = false;
  bool team1_on_safe = false;

  // std::size_t base_index = bb.ValueOr<std::size_t>("BDBaseIndex", 0);
  std::size_t base_index = bb.GetBDBaseIndex();

  for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
    const Player& player = game.GetPlayers()[i];

    bool on_safe_tile = game.GetMap().GetTileId(player.position) == kSafeTileId;

    if (player.frequency == 00) {
      MapCoord enemy_safe = warp.t1[base_index];
      float dist_to_safe = player.position.Distance(enemy_safe);
      bool in_base = ctx.bot->GetRegions().IsConnected(player.position, enemy_safe);

      // kind of hacky could be better
      if (on_safe_tile && dist_to_safe < 5.0f) {
        team0_on_safe = true;
      }
      if (in_base && IsValidPosition(player.position)) {
        team0_dead = false;
      }
    }

    if (player.frequency == 01) {
      MapCoord enemy_safe = warp.t0[base_index];
      float dist_to_safe = player.position.Distance(enemy_safe);
      bool in_base = ctx.bot->GetRegions().IsConnected(player.position, enemy_safe);

      // kind of hacky could be better
      if (on_safe_tile && dist_to_safe < 5.0f) {
        team1_on_safe = true;
      }
      if (in_base && IsValidPosition(player.position)) {
        team1_dead = false;
      }
    }
  }

  // look at flags and determine score increment
  if (team0_on_safe && team1_on_safe) {
    game.SendChatMessage("Both teams have reached the safe tile at the same time!");
    PrintCurrentScore(ctx);
    // bb.Set<bool>("BDWarpToBase", true);
    bb.Set<BDWarpTo>("bdwarpto", BDWarpTo::Base);
  } else if (team0_on_safe) {
    game.SendChatMessage("Team 0 is on team 1's safe!");
    // bb.Set<int>("Team0Score", ++team0_score);
    bb.SetTeam0Score(++team0_score);
    PrintCurrentScore(ctx);
    // bb.Set<bool>("BDWarpToBase", true);
    bb.Set<BDWarpTo>("bdwarpto", BDWarpTo::Base);
  } else if (team1_on_safe) {
    game.SendChatMessage("Team 1 is on team 0's safe!");
    // bb.Set<int>("Team1Score", ++team1_score);
    bb.SetTeam1Score(++team1_score);
    PrintCurrentScore(ctx);
    // bb.Set<bool>("BDWarpToBase", true);
    bb.Set<BDWarpTo>("bdwarpto", BDWarpTo::Base);
  } else if (team0_dead && team1_dead) {
    game.SendChatMessage("All Dead!");
    PrintCurrentScore(ctx);
    // bb.Set<bool>("BDWarpToBase", true);
    bb.Set<BDWarpTo>("bdwarpto", BDWarpTo::Base);
  } else if (team0_dead) {
    if (ctx.bot->GetTime().TimedActionDelay("Team0Dead", respawn_time)) {
      game.SendChatMessage("Team 0 dead!");
      // bb.Set<int>("Team1Score", ++team1_score);
      bb.SetTeam1Score(++team1_score);
      PrintCurrentScore(ctx);
      // bb.Set<bool>("BDWarpToBase", true);
      bb.Set<BDWarpTo>("bdwarpto", BDWarpTo::Base);
    }
  } else if (team1_dead) {
    if (ctx.bot->GetTime().TimedActionDelay("Team1Dead", respawn_time)) {
      game.SendChatMessage("Team 1 dead!");
      // bb.Set<int>("Team0Score", ++team0_score);
      bb.SetTeam0Score(++team0_score);
      PrintCurrentScore(ctx);
      // bb.Set<bool>("BDWarpToBase", true);
      bb.Set<BDWarpTo>("bdwarpto", BDWarpTo::Base);
    }
  }
  g_RenderState.RenderDebugText("  DevaRunDBNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

void RunBDNode::PrintCurrentScore(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  // int team0_score = bb.ValueOr<int>("Team0Score", 0);
  // int team1_score = bb.ValueOr<int>("Team1Score", 0);
  int team0_score = bb.GetTeam0Score();
  int team1_score = bb.GetTeam1Score();

  std::string msg = "Team 0 score: " + std::to_string(team0_score);
  msg += "   Team 1 score: " + std::to_string(team1_score);

  game.SendChatMessage(msg);
}

void RunBDNode::PrintFinalScore(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  // int team0_score = bb.ValueOr<int>("Team0Score", 0);
  // int team1_score = bb.ValueOr<int>("Team1Score", 0);
  int team0_score = bb.GetTeam0Score();
  int team1_score = bb.GetTeam1Score();

  if (team0_score > team1_score) {
    game.SendChatMessage("Team 0 Wins!  Game Over!");
  } else {
    game.SendChatMessage("Team 1 Wins!  Game Over!");
  }

  PrintCurrentScore(ctx);
}

void RunBDNode::ClearScore(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  // bb.Set<int>("Team0Score", 0);
  // bb.Set<int>("Team1Score", 0);
  bb.SetTeam0Score(0);
  bb.SetTeam1Score(0);
}

void RunBDNode::WarpAllToCenter(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
    const Player& player = game.GetPlayers()[i];

    if (player.frequency == 00 || player.frequency == 01) {
      game.SendPrivateMessage(player.name, "?warpto 512 512");
    }
  }
}

void RunBDNode::WarpAllToBase(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  const TeamGoals& warp = ctx.bot->GetTeamGoals().GetGoals();

  std::size_t random_index = rand() % warp.t0.size();
  // std::size_t previous_index = bb.ValueOr<std::size_t>("BDBaseIndex", 0);
  std::size_t previous_index = bb.GetBDBaseIndex();

  while (random_index == previous_index) {
    random_index = rand() % warp.t0.size();
  }

  // bb.Set<std::size_t>("BDBaseIndex", random_index);
  bb.SetBDBaseIndex(random_index);

  for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
    const Player& player = game.GetPlayers()[i];

    if (player.frequency == 00) {
      int team_safe_x = (int)warp.t0[random_index].x;
      int team_safe_y = (int)warp.t0[random_index].y;
      std::string warp_msg = "?warpto " + std::to_string(team_safe_x) + " " + std::to_string(team_safe_y);
      game.SendPrivateMessage(player.name, "?shipreset");
      game.SendPrivateMessage(player.name, warp_msg);
    }

    if (player.frequency == 01) {
      int team_safe_x = (int)warp.t1[random_index].x;
      int team_safe_y = (int)warp.t1[random_index].y;
      std::string warp_msg = "?warpto " + std::to_string(team_safe_x) + " " + std::to_string(team_safe_y);
      game.SendPrivateMessage(player.name, "?shipreset");
      game.SendPrivateMessage(player.name, warp_msg);
    }
  }
  // bb.Set<uint64_t>("BDWarpCooldown", ctx.bot->GetTime().GetTime());
  bb.SetBDWarpCoolDown(ctx.bot->GetTime().GetTime());
}

}  // namespace training
}  // namespace marvin