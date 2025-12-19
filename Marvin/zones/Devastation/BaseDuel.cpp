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
//#include "..//..//common.h"

namespace marvin {
namespace deva {

    const std::string BD_RUN_QUERY = ";bdstart-query";
    const std::string BD_RUN_CONFIRM = ";bdrunning";


void BaseDuelBehaviorBuilder::CreateBehavior(Bot& bot) {

  auto spectator_lock = std::make_unique<bot::SpectatorLockNode>();

  auto startbd = std::make_unique<deva::StartBDNode>();

  auto pausebd = std::make_unique<deva::PauseBDNode>();

  auto runbd = std::make_unique<deva::RunBDNode>();

  auto endbd = std::make_unique<deva::EndBDNode>();

  auto query = std::make_unique<deva::BDQueryResponderNode>();

  auto warpto = std::make_unique<deva::BDWarpTeamNode>();


  auto bd_selector = std::make_unique<behavior::SelectorNode>(startbd.get(), warpto.get(), runbd.get(), endbd.get());

  auto root_sequence = 
      std::make_unique<behavior::SequenceNode>(commands_.get(), spectator_lock.get(),
                                                                query.get(), pausebd.get(), bd_selector.get());

  engine_->PushRoot(std::move(root_sequence));
  engine_->PushNode(std::move(bd_selector));

  engine_->PushNode(std::move(spectator_lock));
  engine_->PushNode(std::move(startbd));
  engine_->PushNode(std::move(pausebd));
  engine_->PushNode(std::move(runbd));
  engine_->PushNode(std::move(endbd));
  engine_->PushNode(std::move(query));
  engine_->PushNode(std::move(warpto));
}

behavior::ExecuteResult StartBDNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  BDState state = bb.ValueOr<BDState>(BBKey::BDHostingState, BDState::Stopped);

  if (state != BDState::Start) {
    g_RenderState.RenderDebugText("  DevaStartDBNode(not running): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  bb.Set<int>(BBKey::BDTeam0Score, 0);
  bb.Set<int>(BBKey::BDTeam1Score, 0);

  if (ctx.bot->GetTime().RepeatedActionDelay("bdgamequery", 10000)) {
    game.SendChatMessage(BD_RUN_QUERY);
    g_RenderState.RenderDebugText("  DevaStartDBNode(sending query): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  ChatMessage msg = game.FindChatMessage(BD_RUN_CONFIRM);

  if (!msg.message.empty()) {
    for (std::string name : kBotNames) {
      if (Lowercase(name) == Lowercase(msg.player)) {
        game.SendChatMessage("I cannot start a base duel game because " + msg.player + " is already hosting one.");
        bb.Set<BDState>(BBKey::BDHostingState, BDState::Stopped);
        g_RenderState.RenderDebugText("  DevaStartDBNode(shutting down): %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Success;
      }
    }
  }

  if (ctx.bot->GetTime().TimedActionDelay("bdgamestartdelay", 3000)) {
    bb.Set<BDState>(BBKey::BDHostingState, BDState::Running);
    bb.Set<BDWarpTo>(BBKey::BDWarpToArea, BDWarpTo::Base);
    g_RenderState.RenderDebugText("  DevaStartDBNode(starting game): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }


  g_RenderState.RenderDebugText("  DevaStartDBNode(waiting): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult PauseBDNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  bool paused = bb.ValueOr<BDState>(BBKey::BDHostingState, BDState::Stopped) == BDState::Paused;

  if (paused) {
    g_RenderState.RenderDebugText("  DevaPauseDBNode(paused): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  g_RenderState.RenderDebugText("  DevaPauseDBNode(not paused): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult EndBDNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

    auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

   BDState state = bb.ValueOr<BDState>(BBKey::BDHostingState, BDState::Stopped);

  if (state != BDState::Ended) {
    g_RenderState.RenderDebugText("  DevaEndDBNode(not running): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  if (ctx.bot->GetTime().RepeatedActionDelay("bdendwarp", 10000)) {
    bb.Set<BDWarpTo>(BBKey::BDWarpToArea, BDWarpTo::Center);
    g_RenderState.RenderDebugText("  DevaEndDBNode(setting warp): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  if (bb.ValueOr<BDWarpTo>(BBKey::BDWarpToArea, BDWarpTo::None) == BDWarpTo::None) {
    bb.Set<BDState>(BBKey::BDHostingState, BDState::Stopped);
  }


  g_RenderState.RenderDebugText("  DevaEndDBNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}




behavior::ExecuteResult RunBDNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  const TeamGoals& warp = ctx.bot->GetTeamGoals().GetGoals();

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  BDState state = bb.ValueOr<BDState>(BBKey::BDHostingState, BDState::Stopped);
  BDWarpTo warpto = bb.ValueOr<BDWarpTo>(BBKey::BDWarpToArea, BDWarpTo::None);

  int team0_score = bb.ValueOr<int>(BBKey::BDTeam0Score, 0);
  int team1_score = bb.ValueOr<int>(BBKey::BDTeam1Score, 0);

  if (state != BDState::Running) {
    g_RenderState.RenderDebugText("  DevaRunDBNode(not running): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  uint64_t respawn_time = game.GetRespawnTime();


  // check the score for win conditions

  bool end_condition = team0_score >= 5 || team1_score >= 5;

  if (end_condition) {
     PrintFinalScore(ctx);
    // ClearScore(ctx);
    bb.Set<BDState>(BBKey::BDHostingState, BDState::Ended);
    // bb.Set<bool>("BDWarpToCenter", true);
    //bb.SetBDState(BDState::Ended);
    g_RenderState.RenderDebugText("  DevaRunDBNode(Game Ended): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  // set conditions for scoring points
  bool team0_dead = true;
  bool team1_dead = true;
  bool team0_on_safe = false;
  bool team1_on_safe = false;

  std::size_t base_index = bb.ValueOr<std::size_t>(BBKey::BDBaseIndex, 0);
  //std::size_t base_index = bb.GetBDBaseIndex();

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
    bb.Set<BDWarpTo>(BBKey::BDWarpToArea, BDWarpTo::Base);
  } else if (team0_on_safe) {
    game.SendChatMessage("Team 0 is on team 1's safe!");
    bb.Set<int>(BBKey::BDTeam0Score, ++team0_score);
    //bb.SetTeam0Score(++team0_score);
    PrintCurrentScore(ctx);
    // bb.Set<bool>("BDWarpToBase", true);
    bb.Set<BDWarpTo>(BBKey::BDWarpToArea, BDWarpTo::Base);
  } else if (team1_on_safe) {
    game.SendChatMessage("Team 1 is on team 0's safe!");
    bb.Set<int>(BBKey::BDTeam1Score, ++team1_score);
    //bb.SetTeam1Score(++team1_score);
    PrintCurrentScore(ctx);
    // bb.Set<bool>("BDWarpToBase", true);
    bb.Set<BDWarpTo>(BBKey::BDWarpToArea, BDWarpTo::Base);
  } else if (team0_dead && team1_dead) {
    game.SendChatMessage("All Dead!");
    PrintCurrentScore(ctx);
    // bb.Set<bool>("BDWarpToBase", true);
    bb.Set<BDWarpTo>(BBKey::BDWarpToArea, BDWarpTo::Base);
  } else if (team0_dead) {
    if (ctx.bot->GetTime().TimedActionDelay("Team0Dead", respawn_time)) {
      game.SendChatMessage("Team 0 dead!");
      bb.Set<int>(BBKey::BDTeam1Score, ++team1_score);
      //bb.SetTeam1Score(++team1_score);
      PrintCurrentScore(ctx);
      // bb.Set<bool>("BDWarpToBase", true);
      bb.Set<BDWarpTo>(BBKey::BDWarpToArea, BDWarpTo::Base);
    }
  } else if (team1_dead) {
    if (ctx.bot->GetTime().TimedActionDelay("Team1Dead", respawn_time)) {
      game.SendChatMessage("Team 1 dead!");
      bb.Set<int>(BBKey::BDTeam0Score, ++team0_score);
      //bb.SetTeam0Score(++team0_score);
      PrintCurrentScore(ctx);
      // bb.Set<bool>("BDWarpToBase", true);
      bb.Set<BDWarpTo>(BBKey::BDWarpToArea, BDWarpTo::Base);
    }
  }
  g_RenderState.RenderDebugText("  DevaRunDBNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

void RunBDNode::PrintCurrentScore(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  int team0_score = bb.ValueOr<int>(BBKey::BDTeam0Score, 0);
  int team1_score = bb.ValueOr<int>(BBKey::BDTeam1Score, 0);
  //int team0_score = bb.GetTeam0Score();
  //int team1_score = bb.GetTeam1Score();

  std::string msg = "Team 0 score: " + std::to_string(team0_score);
  msg += "   Team 1 score: " + std::to_string(team1_score);

  game.SendChatMessage(msg);
}

void RunBDNode::PrintFinalScore(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

   int team0_score = bb.ValueOr<int>(BBKey::BDTeam0Score, 0);
   int team1_score = bb.ValueOr<int>(BBKey::BDTeam1Score, 0);
  //int team0_score = bb.GetTeam0Score();
  //int team1_score = bb.GetTeam1Score();

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

  bb.Set<int>(BBKey::BDTeam0Score, 0);
  bb.Set<int>(BBKey::BDTeam1Score, 0);
}





behavior::ExecuteResult BDQueryResponderNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  ChatMessage msg = game.FindChatMessage(BD_RUN_QUERY);

  if (!msg.message.empty()) {
    for (std::string name : kBotNames) {
      if (Lowercase(name) == Lowercase(msg.player)) {
        game.SendChatMessage(BD_RUN_CONFIRM);
        g_RenderState.RenderDebugText("  DevaQueryResponderDBNode(responding): %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Failure;
      }
    }
  }

  g_RenderState.RenderDebugText("  DevaQueryResponderDBNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult BDWarpTeamNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  BDState state = bb.ValueOr<BDState>(BBKey::BDHostingState, BDState::Stopped);
  BDWarpTo warpto = bb.ValueOr<BDWarpTo>(BBKey::BDWarpToArea, BDWarpTo::None);

  if (state == BDState::Paused) {
    g_RenderState.RenderDebugText("  DevaWarpDBNode(not running): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  if (warpto == BDWarpTo::Center) {
    if (ctx.bot->GetTime().RepeatedActionDelay("warpalltocenter", 3000)) {
      WarpAllToCenter(ctx);
    }

    if (AllInCenter(ctx)) {
      game.SendChatMessage("All players have been warped to center!");
      bb.Set<BDWarpTo>(BBKey::BDWarpToArea, BDWarpTo::None);
    }
    g_RenderState.RenderDebugText("  DevaWarpDBNode(warping to center): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  if (warpto == BDWarpTo::Base) {
    if (ctx.bot->GetTime().RepeatedActionDelay("warpalltobase", 3000)) {
      WarpAllToBase(ctx);
    }

    if (AllInBase(ctx)) {
      game.SendChatMessage("All players have been warped to base!");
      bb.Set<BDWarpTo>(BBKey::BDWarpToArea, BDWarpTo::None);
    }

    g_RenderState.RenderDebugText("  DevaWarpDBNode(warping to base): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  g_RenderState.RenderDebugText("  DevaWarpDBNode(not warping): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

void BDWarpTeamNode::WarpAllToBase(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  const TeamGoals& warp = ctx.bot->GetTeamGoals().GetGoals();

  std::size_t random_index = rand() % warp.t0.size();
  // std::size_t previous_index = bb.ValueOr<std::size_t>("BDBaseIndex", 0);
  //std::size_t previous_index = bb.GetBDBaseIndex();
  std::size_t previous_index = bb.ValueOr<std::size_t>(BBKey::BDBaseIndex, 0);

  while (random_index == previous_index) {
    random_index = rand() % warp.t0.size();
  }

  bb.Set<std::size_t>(BBKey::BDBaseIndex, random_index);
  //bb.SetBDBaseIndex(random_index);

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
  //bb.SetBDWarpCoolDown(ctx.bot->GetTime().GetTime());
}

bool BDWarpTeamNode::AllInBase(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  //std::size_t index = bb.GetBDBaseIndex();
  std::size_t index = bb.ValueOr<std::size_t>(BBKey::BDBaseIndex, 0);
  const TeamGoals& warp = ctx.bot->GetTeamGoals().GetGoals();

  bool result = true;

  for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
    const Player& player = game.GetPlayers()[i];

    if (player.frequency == 00 || player.frequency == 01) {
      bool connected = ctx.bot->GetRegions().IsConnected(player.position, warp.t0[index]);

      if (!connected) {
        result = false;
        break;
      }
    }
  }

  return result;
}

void BDWarpTeamNode::WarpAllToCenter(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
    const Player& player = game.GetPlayers()[i];

    if (player.frequency == 00 || player.frequency == 01) {
      game.SendPrivateMessage(player.name, "?shipreset");
      game.SendPrivateMessage(player.name, "?warpto 512 512");
    }
  }
}

bool BDWarpTeamNode::AllInCenter(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  bool result = true;

    for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
    const Player& player = game.GetPlayers()[i];

    if (player.frequency == 00 || player.frequency == 01) {
      bool connected = ctx.bot->GetRegions().IsConnected(player.position, MapCoord(512, 512));

      if (!connected) {
        result = false;
        break;
      }
    }
  }

    return result;
}

behavior::ExecuteResult BDClearScoreNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
}

}  // namespace training
}  // namespace marvin