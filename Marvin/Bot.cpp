#include "Bot.h"

#include <chrono>
#include <cstring>
#include <limits>
#include <thread>

#include "Debug.h"
#include "GameProxy.h"
#include "Map.h"
#include "Shooter.h"
#include "platform/Platform.h"

#include "zones/Devastation/Devastation.h"
#include "zones/Devastation/BaseDuelWarpCoords.h"
#include "zones/ExtremeGames.h"
#include "zones/GalaxySports.h"
#include "zones/Hockey.h"
#include "zones/Hyperspace/Hyperspace.h"
#include "zones/Hyperspace/HyperspaceFlagRooms.h"
#include "zones/PowerBall.h"
#include "zones/Devastation/Training.h"
#include "zones/Devastation/BaseDuel.h"

namespace marvin {

class DefaultBehaviorBuilder : public BehaviorBuilder {
 public:
  void CreateBehavior(Bot& bot) {
    auto move_to_enemy = std::make_unique<bot::MoveToEnemyNode>();

    auto move_method_selector = std::make_unique<behavior::SelectorNode>(move_to_enemy.get());
    auto los_weapon_selector = std::make_unique<behavior::SelectorNode>(shoot_enemy_.get());
    auto parallel_shoot_enemy =
        std::make_unique<behavior::ParallelNode>(los_weapon_selector.get(), move_method_selector.get());

    auto path_to_enemy_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy_.get(), follow_path_.get());

    auto los_shoot_conditional =
        std::make_unique<behavior::SequenceNode>(target_in_los_.get(), parallel_shoot_enemy.get());

    auto path_or_shoot_selector =
        std::make_unique<behavior::SelectorNode>(los_shoot_conditional.get(), path_to_enemy_sequence.get());

    auto find_enemy_in_center_sequence =
        std::make_unique<behavior::SequenceNode>(find_enemy_in_center_.get(), path_or_shoot_selector.get());

    auto action_selector = std::make_unique<behavior::SelectorNode>(find_enemy_in_center_sequence.get());

    auto root_sequence = std::make_unique<behavior::SequenceNode>(commands_.get(),
                                                 spectator_check_.get(), respawn_check_.get(), action_selector.get());

    engine_->PushRoot(std::move(root_sequence));

    engine_->PushNode(std::move(move_to_enemy));
    engine_->PushNode(std::move(move_method_selector));
    engine_->PushNode(std::move(los_weapon_selector));
    engine_->PushNode(std::move(parallel_shoot_enemy));
    engine_->PushNode(std::move(path_to_enemy_sequence));
    engine_->PushNode(std::move(los_shoot_conditional));
    engine_->PushNode(std::move(path_or_shoot_selector));
    engine_->PushNode(std::move(find_enemy_in_center_sequence));
    engine_->PushNode(std::move(action_selector));
  }
};


Bot::Bot(std::shared_ptr<marvin::GameProxy> game) : game_(std::move(game)) {
  srand((unsigned int)(time_.GetTime()));
  base_paths_ = nullptr;
  goals_ = nullptr;

  blackboard_ = std::make_unique<Blackboard>();
  influence_map_ = std::make_unique<InfluenceMap>();

  game_->SendChatMessage("?chat=marvin");

  load_index = 1;
  //Load();
}

// designed to run each case, increment the load index and return, so it doesnt run a bunch of stuff in
// one update.  Cases that run path pathfinder or region registry stuff are broken down even further.
// when the switch hits the default case it sets the load index to 0 which means its finished.
void Bot::Load() {
  PerformanceTimer timer;
 
  switch (load_index) {
    case 0: {
      return;
    }
    case 1: {
     // log.Write("LOADING BOT", timer.GetElapsedTime());

      radius_ = game_->GetRadius();
      command_system_ = std::make_unique<CommandSystem>(game_->GetZone());
      regions_ = std::make_unique<RegionRegistry>(game_->GetMap());
      load_index++;
      break;
    }
    case 2: {
      regions_->Create(game_->GetMap(), radius_, xMin, xMax);
     // log.Write("Regions Loaded", timer.GetElapsedTime());
      xMin += 32;
      xMax += 32;
      if (xMin == 1024) {
        xMin = 0;
        xMax = 32;
        load_index++;
      }
      break;
    }
    case 3: {
      auto processor = std::make_unique<path::NodeProcessor>(game_->GetMap());
     // log.Write("Node Processor Loaded", timer.GetElapsedTime());
      pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor), *regions_);
     // log.Write("Pathfinder Loaded", timer.GetElapsedTime());
      load_index++;
      break;
    }
    case 4: {
      pathfinder_->SetTraversabletiles(game_->GetMap(), radius_, xMin, xMax);
     // log.Write("Pathfinder traversable tiles Loaded", timer.GetElapsedTime());
      xMin += 32;
      xMax += 32;
      if (xMin == 1024) {
        xMin = 0;
        xMax = 32;
        load_index++;
      }
      break;
    }
    case 5: {
      (pathfinder_->CalculateEdges(game_->GetMap(), radius_, xMin, xMax));
     // log.Write("Pathfinder edges Loaded", timer.GetElapsedTime());
      xMin += 32;
      xMax += 32;
      if (xMin == 1024) {
        xMin = 0;
        xMax = 32;
        load_index++;
      }
      break;
    }
    case 6: {
      pathfinder_->SetMapWeights(game_->GetMap(), xMin, xMax, radius_);
     // log.Write("PathFinder Weights Loaded", timer.GetElapsedTime());
      xMin += 32;
      xMax += 32;
      if (xMin == 1024) {
        xMin = 0;
        xMax = 32;
        load_index++;
      }
      break;
    }
    case 7: {
      if (base_paths_) {
        base_paths_ = std::make_unique<BasePaths>(goals_->GetGoals(), radius_, *pathfinder_, game_->GetMap());
      //  log.Write("Base Paths Loaded", timer.GetElapsedTime());
      }
      load_index++;
      break;
    }
    case 8: {

      SelectBehaviorTree();

      ctx_.bot = this;

      //log.Write("BOT LOADED - TOTAL TIME", timer.TimeSinceConstruction());
    //  log.Write("Builder Loaded", timer.GetElapsedTime());
      load_index++;
      break;
    }
    default: {
      load_index = 0;
      last_load_timestamp_ = time_.GetTime();
    }
  }
}

void Bot::Update(bool reload, float dt) {
  PerformanceTimer timer;
  g_RenderState.debug_y = 30.0f;

  keys_.ReleaseAll();
  time_.Update();

  UpdateState state = game_->Update();
  g_RenderState.RenderDebugText("GameUpdate: %llu", timer.GetElapsedTime());

  if (state == UpdateState::Wait) return;
  
  if (state == UpdateState::Reload || reload || blackboard_->ValueOr<bool>("reloadbot", false)) {
    load_index = 1;
    g_RenderState.RenderDebugText("Wait For Game");
    return;
  }

  // the load behavior is determined by the load index
  Load();
  if (load_index != 0) return;

  float radius = game_->GetRadius();
  int ship = game_->GetPlayer().ship;
  
  if (radius != radius_) {
  //  log.Write("SHIP RADIUS CHANGED");
    radius_ = radius;
    //LoadForRadius();
    load_index = 1;
    return;
  }

  //u8 id = game_->GetMap().GetTileId(Vector2f(575, 512));
  //g_RenderState.RenderDebugText("Tile Id: %u", id);

  if (blackboard_->GetCombatDifficulty() == CombatDifficulty::Nerf) {
    update_interval_ = 10;
  } else {
    update_interval_ = 60;
  }

  steering_.Reset();
  ctx_.dt = dt;

  //game_->SetVelocity(Vector2f(200, 200));

  if (game_->GetZone() == Zone::Devastation) {
    if (game_->GetPlayer().name == "FrogBot") {
      if (game_->GetMapFile() != "bdelite.lvl") {
        if (GetTime().TimedActionDelay("arenachange", 200)) {
          game_->SendChatMessage("?go BDElite");
        }
        return;
      }
    }
    if (game_->GetPlayer().name == "MadMarv") {
      if (game_->GetMapFile() != "bdeg.lvl") {
        if (GetTime().TimedActionDelay("arenachange", 200)) {
          game_->SendChatMessage("?go bdeg");
        }
        return;
      }
    }
  }

 #if DUBUG_RENDER_PATH
  RenderPath(game_->GetPosition(), pathfinder_->GetPath());
  g_RenderState.RenderDebugText("Render Path: %llu", timer.GetElapsedTime());
#endif

  #if DEBUG_RENDER_PATHFINDER
      pathfinder_->DebugUpdate(game_->GetPosition());
  g_RenderState.RenderDebugText("PathfinderDebugUpdate: %llu", timer.GetElapsedTime());
  #endif

  #if DEBUG_RENDER_INFLUENCE
  influence_map_.DebugUpdate(game_->GetPosition());
#endif

  #if DEBUG_RENDER_SHOOTER
  shooter_.DebugUpdate(*this);
#endif

  #if DEBUG_RENDER_BASE_PATHS

  Vector2f position = game_->GetPosition();
  const std::vector<std::vector<Vector2f>>& base_paths = GetBasePaths().GetBasePaths();

  for (Path path : base_paths) {
    if (!path.empty() && regions_->IsConnected(path[0], position)) {
      RenderPath(position, path);
    }
  }

#endif

  #if DEBUG_RENDER_REGION_REGISTRY
    regions_->DebugUpdate(game_->GetPosition());
    g_RenderState.RenderDebugText("RegionDebugUpdate: %llu", timer.GetElapsedTime());
#endif

   //   pathfinder_->CreatePath(*this, game_->GetPosition(), MapCoord(839, 223), 0.95);
    // ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPosition(), MapCoord(522, 381), 0.95);
    //  ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPosition(), MapCoord(399, 543), 0.95);
    //pathfinder_->CreatePath(*this, game_->GetPosition(), MapCoord(962, 63), 0.95);
   // return;

  behavior_->Update(ctx_);

  g_RenderState.RenderDebugText("Behavior: %llu", timer.GetElapsedTime());


  //#if 0 // Test wall avoidance. This should be moved somewhere in the behavior tree
  //std::vector<Vector2f> path = ctx_.blackboard.ValueOr<std::vector<Vector2f>>("Path", std::vector<Vector2f>());
  const std::vector<Vector2f>& path = pathfinder_->GetPath();
  constexpr float kNearbyTurn = 20.0f;
  constexpr float kMaxAvoidDistance = 35.0f;

  if (!path.empty() && path[0].DistanceSq(game_->GetPosition()) < kNearbyTurn * kNearbyTurn) {
    steering_.AvoidWalls(*this, kMaxAvoidDistance);
  }
  //#endif

  if (ship != 8) {
   // steering_.Steer(*this, ctx_.blackboard.ValueOr<bool>("SteerBackwards", false));
   // ctx_.blackboard.Set<bool>("SteerBackwards", false);
    steering_.Steer(*this, ctx_.bot->GetBlackboard().GetSteerBackwards());
    ctx_.bot->GetBlackboard().SetSteerBackwards(false);
  }

  g_RenderState.RenderDebugText("Steering: %llu", timer.GetElapsedTime());
}

void Bot::SelectBehaviorTree() {

  std::unique_ptr<BehaviorBuilder> builder;
  std::string map_name = game_->GetMapFile(); 

  switch (game_->GetZone()) {
    case Zone::Devastation: {
      goals_ = std::make_unique<deva::BaseDuelWarpCoords>(game_->GetMapFile());
      if (map_name == "bdelite.lvl") {
       // log.Write("Building Training behavior tree.");
        builder = std::make_unique<training::TrainingBehaviorBuilder>();
      } else {
        if (blackboard_->ValueOr<BDState>("bdstate", BDState::Stopped) == BDState::Start) {
          builder = std::make_unique<deva::BaseDuelBehaviorBuilder>();
        }
        else if (goals_->HasCoords()) {
        //  log.Write("Building Devastation behavior tree.");
          base_paths_ = std::make_unique<BasePaths>(goals_->GetGoals(), radius_, *pathfinder_, game_->GetMap());
          builder = std::make_unique<deva::DevastationBehaviorBuilder>();
        } else {
          game_->SendChatMessage("Warning: I don't have base duel coords for this arena, using default behavior tree.");
        //  log.Write("Building Default behavior tree.");
          builder = std::make_unique<DefaultBehaviorBuilder>();
        }
      }
    } break;
    case Zone::ExtremeGames: {
      builder = std::make_unique<eg::ExtremeGamesBehaviorBuilder>();
    } break;
    case Zone::GalaxySports: {
      // builder = std::make_unique<gs::GalaxySportsBehaviorBuilder>();
    } break;
    case Zone::Hockey: {
      builder = std::make_unique<hz::HockeyBehaviorBuilder>();
    } break;
    case Zone::Hyperspace: {
      goals_ = std::make_unique<hs::HSFlagRooms>();
      base_paths_ = std::make_unique<BasePaths>(goals_->GetGoals(), radius_, *pathfinder_, game_->GetMap());
      builder = std::make_unique<hs::HyperspaceBehaviorBuilder>();
     // log.Write("Building Hyperspace behavior tree.");
    } break;
    case Zone::PowerBall: {
      // builder = std::make_unique<pb::PowerBallBehaviorBuilder>();
    } break;
    default: {
      builder = std::make_unique<DefaultBehaviorBuilder>();
    } break;
  }

  this->behavior_ = builder->Build(*this);

}

void Bot::Move(const Vector2f& target, float target_distance) {
  const Player& bot_player = game_->GetPlayer();
  float distance = bot_player.position.Distance(target);
  float speed_sq = game_->GetPlayer().velocity.LengthSq();

  if (speed_sq < 0.3f * 0.3f) {
    Vector2f direction = Normalize(target - bot_player.position);
    Vector2f new_direction;

    if (fabsf(direction.x) < fabsf(direction.y)) {
      new_direction = Normalize(Reflect(direction, Vector2f(0, 1)));
    } else {
      new_direction = Normalize(Reflect(direction, Vector2f(1, 0)));
    }

    // Face a reflected vector so it rotates away from the wall.
    steering_.Face(*this, bot_player.position + new_direction);
    steering_.Seek(*this, target);
    return;
  }

  if (distance > target_distance) {
    steering_.Arrive(*this, target, 0);
  }

  else if (distance <= target_distance) {
    Vector2f to_target = target - bot_player.position;

    steering_.Arrive(*this, target - Normalize(to_target) * target_distance, 0);
  }
}

#if 0
void Bot::CreateBasePaths(const std::vector<Vector2f>& start_vector, const std::vector<Vector2f>& end_vector,
                          float radius) {
  PerformanceTimer timer;
  base_paths_.clear();

  for (std::size_t i = 0; i < start_vector.size(); i++) {
    Vector2f position_1 = start_vector[i];
    Vector2f position_2 = end_vector[i];

    Path base_path = GetPathfinder().FindPath(game_->GetMap(), std::vector<Vector2f>(), position_1, position_2, radius);

    base_paths_.push_back(base_path);
  }
  
  g_RenderState.RenderDebugText("CreateBasePaths: %llu", timer.GetElapsedTime());
}

#endif

void Bot::FindPowerBallGoal() {
  powerball_goal_;
  powerball_goal_path_;

  float closest_distance = std::numeric_limits<float>::max();

  // player is in original
  if (game_->GetMapFile() == "powerlg.lvl") {
    if (game_->GetPlayer().frequency == 00) {
      powerball_goal_ = Vector2f(945, 512);

      std::vector<Vector2f> goal1_paths{Vector2f(945, 495), Vector2f(926, 512), Vector2f(962, 511), Vector2f(944, 528)};

      for (std::size_t i = 0; i < goal1_paths.size(); i++) {
        float distance = goal1_paths[i].Distance(game_->GetPosition());
        if (distance < closest_distance) {
          powerball_goal_path_ = goal1_paths[i];
          closest_distance = distance;
        }
      }
    } else {
      powerball_goal_ = Vector2f(72, 510);

      std::vector<Vector2f> goal0_paths{Vector2f(89, 510), Vector2f(72, 526), Vector2f(55, 509), Vector2f(73, 492)};

      for (std::size_t i = 0; i < goal0_paths.size(); i++) {
        float distance = goal0_paths[i].Distance(game_->GetPosition());
        if (distance < closest_distance) {
          powerball_goal_path_ = goal0_paths[i];
          closest_distance = distance;
        }
      }
    }
  }
  // player is in pub
  else if (game_->GetMapFile() == "combo_pub.lvl") {
    std::vector<Vector2f> arenas{Vector2f(512, 133), Vector2f(512, 224), Vector2f(512, 504), Vector2f(512, 775)};
    std::vector<Vector2f> goal_0{Vector2f(484, 137), Vector2f(398, 263), Vector2f(326, 512), Vector2f(435, 780)};
    std::vector<Vector2f> goal_1{Vector2f(540, 137), Vector2f(625, 263), Vector2f(697, 512), Vector2f(587, 780)};

    for (std::size_t i = 0; i < arenas.size(); i++) {
      if (GetRegions().IsConnected((MapCoord)game_->GetPosition(), arenas[i])) {
        if (game_->GetPlayer().frequency == 00) {
          powerball_goal_ = goal_1[i];
          powerball_goal_path_ = powerball_goal_;
        } else {
          powerball_goal_ = goal_0[i];
          powerball_goal_path_ = powerball_goal_;
        }
      }
    }
  }
}

bool Bot::MaxEnergyCheck() {
  float max_energy = game_->GetMaxEnergy();

  float energy_pct = ((float)game_->GetEnergy() / max_energy) * 100.0f;
  bool result = false;

  if ((game_->GetPlayer().status & 2) != 0) {
    game_->Cloak(keys_);
  }
  // will cause eg to return true when prized and energy is above initial
  else if (energy_pct >= 100.0f) {
    result = true;
  }
  if (game_->GetPlayer().dead) {
    result = false;
  }
  return result;
}

namespace bot {

behavior::ExecuteResult FailureNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  g_RenderState.RenderDebugText("  FailureNode(Fail): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult SuccessNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  g_RenderState.RenderDebugText("  SuccessNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult CommandNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
 
 // bool executed = false;

  for (ChatMessage& chat : game.GetCurrentChat()) {
    if (ctx.bot->GetCommandSystem().ProcessMessage(*ctx.bot, chat)) {
      // Return failure on execute so the behavior will start over next tick with the commands processed completely.
      g_RenderState.RenderDebugText("  CommandNode(fail): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    }
  }

  g_RenderState.RenderDebugText("  CommandNode(success): %llu", timer.GetElapsedTime());

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult SetArenaNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  const std::string& arena = bb.GetArena();
  std::string mapfile = game.GetMapFile();
  // trim the .lvl extension
  mapfile = mapfile.substr(0, mapfile.size() - 4);


  if (bb.GetCommandRequest() == CommandRequestType::ArenaChange) {
    if (mapfile != arena) {
      if (ctx.bot->GetTime().TimedActionDelay("arenachange", 200)) {
        game.SetEnergy(100, "Set Arena Node");
        game.SendChatMessage("?go " + arena);
      }
      g_RenderState.RenderDebugText("  SetShipNode(fail): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    } else {
      bb.SetCommandRequest(CommandRequestType::None);
    }
  }

  g_RenderState.RenderDebugText("  SetArenaNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
  }

behavior::ExecuteResult SetShipNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  uint16_t cShip = game.GetPlayer().ship;
  //uint16_t dShip = bb.ValueOr<uint16_t>("Ship", 0);
  uint16_t dShip = (uint16_t)bb.GetShip();

  uint64_t ship_cooldown = bb.GetSetShipDown();
  if (cShip == 8) {
    ship_cooldown = 1000;
  }

  //if (bb.GetCommandRequest() == CommandRequestType::ShipChange) {
    if (cShip != dShip && cShip == 8) {
      game.SetShip(dShip);
      g_RenderState.RenderDebugText("  SetShipNode(fail): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;

    } else {
     // bb.SetCommandRequest(CommandRequestType::None);
    }
  //}

  g_RenderState.RenderDebugText("  SetShipNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult SetFreqNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  const char* balancer_msg = "Changing to that team would disrupt the balance between it and your current team.";

  //uint16_t freq = bb.ValueOr<uint16_t>("Freq", 999);
  uint16_t freq = bb.GetFreq();

  if (bb.GetCommandRequest() == CommandRequestType::FreqChange) {
    //if (freq != 999) {
    if (freq != game.GetPlayer().frequency) {
      if (ctx.bot->GetTime().TimedActionDelay("setfreq", 200)) {
        game.SetEnergy(100, "Set Freq Node");
        game.SetFreq(freq);
      }

      for (ChatMessage& chat : game.GetCurrentChat()) {
        if (chat.message == balancer_msg && chat.type == ChatType::Arena) {
          game.SendChatMessage("The zone balancer has prevented me from joining that team.");
          // bb.Set<uint16_t>("Freq", 999);
          //bb.SetFreq(999);
          bb.SetCommandRequest(CommandRequestType::None);
          break;
        }
      }

     // if (freq == int(game.GetPlayer().frequency)) {
        // bb.Set<uint16_t>("Freq", 999);
     //   bb.SetFreq(999);
    //  }

      g_RenderState.RenderDebugText("  SetFreqNode(fail): %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Failure;
    } else {
      bb.SetCommandRequest(CommandRequestType::None);
    }
  }
  g_RenderState.RenderDebugText("  SetFreqNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult DettachNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

    // if attached to a player detach
  if (game.GetPlayer().attach_id != 65535) {

    game.SendKey(VK_F7);

    g_RenderState.RenderDebugText("  DettachNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  g_RenderState.RenderDebugText("  DettachNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult SpectatorCheckNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();

  if (game.GetPlayer().ship == 8) {
    g_RenderState.RenderDebugText("  ShipCheckNode(fail): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  g_RenderState.RenderDebugText("  ShipCheckNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult SpectatorLockNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();

  if (game.GetPlayer().ship != 8) {
    game.SetShip(8);
    g_RenderState.RenderDebugText("  SpecLockNode(setting ship): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  g_RenderState.RenderDebugText("  ShipCheckNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult CastWeaponInfluenceNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& influence = ctx.bot->GetInfluenceMap();

  float decay_multiplier = 5.0f * (game.GetEnergy() / game.GetMaxEnergy());
  if (decay_multiplier < 1.0f) {
    decay_multiplier = 1.0f;
  }
  influence.Decay(game.GetPosition(), game.GetMaxSpeed(), ctx.dt, decay_multiplier);
  g_RenderState.RenderDebugText("InfluenceMapDecay: %llu", timer.GetElapsedTime());

  influence.CastWeapons(*ctx.bot);
  g_RenderState.RenderDebugText("InfluenceMapUpdate: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
};

behavior::ExecuteResult SortBaseTeams::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  const Player& bot = game.GetPlayer();

 // std::vector<Player> team_list;
 // std::vector<Player> enemy_list;
 // std::vector<Player> combined_list;
  bb.ClearTeamList();
  bb.ClearEnemyList();
  bb.ClearCombinedList();
  bb.ClearFreqList();
  bb.ClearTeamShipCounts();

  // a list of player counts for each frequency
  // where the index is the frequency, and the value is the player count
  //std::vector<uint16_t> fList(100, 0);

  //bool team_in_base = false;
  //bool enemy_in_base = false;
  //bool last_in_base = true;
  bb.SetTeamInBase(false);
  bb.SetEnemyInBase(false);
  bb.SetLastInBase(true);

 // uint16_t pFreq0 = bb.ValueOr<uint16_t>("PubTeam0", 00);
 // uint16_t pFreq1 = bb.ValueOr<uint16_t>("PubTeam1", 01);
  uint16_t pFreq0 = bb.GetPubTeam0();
  uint16_t pFreq1 = bb.GetPubTeam1();
  int base_team_count = 0;

  // Vector2f current_base = ctx.blackboard.GetBase[ctx.blackboard.GetRegionIndex()];

  // find team sizes and push team mates into a vector
  for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
    const Player& player = game.GetPlayers()[i];

    bool in_center = ctx.bot->GetRegions().IsConnected(player.position, MapCoord(512, 512));

    if (player.frequency < 100) {
      //fList[player.frequency]++;
      bb.IncrementFreqList(player.frequency);
    }

    if (player.frequency == pFreq0 || player.frequency == pFreq1) {
      bb.IncrementTeamShipCounts((Ship)player.ship);
      bb.PushCombinedList(&game.GetPlayers()[i]);
      base_team_count++;

      if (player.frequency == bot.frequency) {
        //team_list.push_back(game.GetPlayers()[i]);
        bb.PushTeamList(&game.GetPlayers()[i]);

        if (!in_center && IsValidPosition(player.position)) {
          //team_in_base = true;
          bb.SetTeamInBase(true);
        }
        if (!in_center && !player.dead && player.id != game.GetPlayer().id) {
          //last_in_base = false;
          bb.SetLastInBase(false);
        }
      } else {
        //enemy_list.push_back(game.GetPlayers()[i]);
        bb.PushEnemyList(&game.GetPlayers()[i]);

        if (!in_center && IsValidPosition(player.position)) {
          //enemy_in_base = true;
          bb.SetEnemyInBase(true);
        }
      }
    }
  }

  // last in base flag should not be set when bot is the only player on the team
  if (bb.GetTeamList().size() == 1) {
    //last_in_base = false;
    bb.SetLastInBase(false);
  }

  bb.SetBaseTeamsCount(base_team_count);

  //bb.Set<std::vector<Player>>("TeamList", team_list);
  //bb.Set<std::vector<Player>>("EnemyList", enemy_list);
 // bb.Set<std::vector<Player>>("CombinedList", combined_list);

  //bb.Set<std::vector<uint16_t>>("FreqList", fList);

 // bb.Set<bool>("TeamInBase", team_in_base);
  //bb.Set<bool>("EnemyInBase", enemy_in_base);
  //bb.Set<bool>("LastInBase", last_in_base);

  g_RenderState.RenderDebugText("  SortBaseTeams: %llu", timer.GetElapsedTime());

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult RespawnCheckNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& game = ctx.bot->GetGame();

  if (game.GetPlayer().dead) {
    g_RenderState.RenderDebugText("  RespawnCheckNode(fail): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  g_RenderState.RenderDebugText("  RespawnCheckNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult FindEnemyInCenterNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  behavior::ExecuteResult result = behavior::ExecuteResult::Failure;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  //if (!bb.ValueOr<bool>("InCenter", true)) {
  if (!bb.GetInCenter()) {
    g_RenderState.RenderDebugText("  FindEnemyInCenterNode(NotInCenter): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  const Player& bot = game.GetPlayer();
  float closest_cost = std::numeric_limits<float>::max();
  const Player* target = nullptr;

  for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
    const Player& player = game.GetPlayers()[i];

    if (!IsValidTarget(*ctx.bot, player, bb.GetCombatRole())) {
     // g_RenderState.RenderDebugText("  Not Valid %llu", timer.GetElapsedTime());
      continue;
    }

    float cost = CalculateCost(ctx, bot, player);

    if (cost < closest_cost) {
      closest_cost = cost;
      target = &game.GetPlayers()[i];
      result = behavior::ExecuteResult::Success;
    }
  }

  //const Player* current_target = bb.ValueOr<const Player*>("Target", nullptr);
  const Player* current_target = bb.GetTarget();

  if (current_target && IsValidTarget(*ctx.bot, *current_target, bb.GetCombatRole())) {
    // Calculate the cost to the current target so there's some stickiness
    // between close targets.
    const float kStickiness = 2.5f;
    float cost = CalculateCost(ctx, bot, *current_target);

    if (cost * kStickiness < closest_cost) {
      target = current_target;
    }
  }

  bb.Set<const Player*>("target", target);
  bb.SetTarget(target);
  if (result == behavior::ExecuteResult::Success) {
    g_RenderState.RenderDebugText("  FindEnemyInCenterNode(EnemyFound): %llu", timer.GetElapsedTime());
  } else {
    g_RenderState.RenderDebugText("  FindEnemyInCenterNode(NotFound): %llu", timer.GetElapsedTime());
  }
  return result;
}

float FindEnemyInCenterNode::CalculateCost(behavior::ExecuteContext& ctx, const Player& bot_player,
                                           const Player& target) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  float dist = bot_player.position.Distance(target.position);

  // How many seconds it takes to rotate 180 degrees
  float rotate_speed = game.GetRotation() * 0.5f;

  float move_cost = dist / game.GetMaxSpeed();

  Vector2f direction = Normalize(target.position - bot_player.position);
  float dot = std::abs(bot_player.GetHeading().Dot(direction) - 1.0f) / 2.0f;
  float rotate_cost = std::abs(dot) * rotate_speed;

  return move_cost + rotate_cost;
}

// get the closest player to the bot using path distance
behavior::ExecuteResult FindEnemyInBaseNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  behavior::ExecuteResult result = behavior::ExecuteResult::Failure;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  // bool in_center = bb.ValueOr<bool>("InCenter", false);
  bool in_center = bb.GetInCenter();
  bool last_in_base = bb.GetLastInBase();
  // bool anchoring = bb.ValueOr<bool>("IsAnchor", false);
  CombatRole combat_role = bb.GetCombatRole();
  const Path& base_path = ctx.bot->GetBasePath();

  if (base_path.empty()) {
    g_RenderState.RenderDebugText("  FindEnemyInBaseNode(path empty): %llu", timer.GetElapsedTime());
    return result;
  }
  if (in_center) {
    g_RenderState.RenderDebugText("  FindEnemyInBaseNode(bot in center): %llu", timer.GetElapsedTime());
    return result;
  }
  if (last_in_base && combat_role != CombatRole::Anchor) {  // anchors will follow anchor logic
    g_RenderState.RenderDebugText("  FindEnemyInBaseNode(last in base): %llu", timer.GetElapsedTime());
    return result;
  }

  // search creation is about 10-30 microseconds, search can be null
  auto search = path::PathNodeSearch::Create(*ctx.bot, base_path);
  std::size_t bot_node = search->FindNearestNodeBFS(game.GetPosition());

  const Player* target = nullptr;
  float shortest_distance = std::numeric_limits<float>::max();

  for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
    const Player& enemy = game.GetPlayers()[i];

    //if (enemy.frequency != bb.GetPubTeam0() && enemy.frequency != bb.GetPubTeam1()) continue;
    if (!IsValidTarget(*ctx.bot, enemy, bb.GetCombatRole())) continue;

    // players closest path node
    size_t enemy_node = search->FindNearestNodeBFS(enemy.position);

    // get distance between bot and the current enemy
    float enemy_distance = search->GetPathDistance(bot_node, enemy_node);

    if (enemy_distance < shortest_distance) {
      shortest_distance = enemy_distance;
      target = &game.GetPlayers()[i];
      result = behavior::ExecuteResult::Success;
    }
  }

  bb.SetTarget(target);
  //bb.Set<const Player*>("Target", target);
  if (result == behavior::ExecuteResult::Success) {
    std::string msg = "  FindEnemyInBaseNode(" + target->name + "): %llu";
    g_RenderState.RenderDebugText(msg.c_str(), timer.GetElapsedTime());
  } else {
    g_RenderState.RenderDebugText("  FindEnemyInBaseNode(enemy not found): %llu", timer.GetElapsedTime());
  }

  return result;
}

behavior::ExecuteResult PathToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  Vector2f bot = game.GetPosition();

  //const Player* enemy = bb.ValueOr<const Player*>("Target", nullptr);
  const Player* enemy = bb.GetTarget();

  if (!enemy) {
    g_RenderState.RenderDebugText("  PathToEnemyNode(fail): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  float radius = game.GetShipSettings().GetRadius();

  ctx.bot->GetPathfinder().CreatePath(*ctx.bot, bot, enemy->position, radius);

  g_RenderState.RenderDebugText("  PathToEnemyNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult PatrolNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  Vector2f from = game.GetPosition();
  float radius = game.GetShipSettings().GetRadius();

  //std::vector<Vector2f> nodes = bb.ValueOr<std::vector<Vector2f>>("PatrolNodes", std::vector<Vector2f>());
  const std::vector<MapCoord>& nodes = bb.GetPatrolNodes();
  //int index = ctx.blackboard.ValueOr<int>("PatrolIndex", 0);
  std::size_t index = bb.GetPatrolIndex();

  //if (bb.ValueOr<bool>("InCenter", true)) {
  if (bb.GetInCenter()) {
    Vector2f to = nodes.at(index);

    if (game.GetPosition().DistanceSq(to) < 5.0f * 5.0f) {
      index = (index + 1) % nodes.size();
      //ctx.blackboard.Set<int>("PatrolIndex", index);
      bb.SetPatrolIndex(index);
      to = nodes.at(index);
    }

    ctx.bot->GetPathfinder().CreatePath(*ctx.bot, from, to, radius);

    g_RenderState.RenderDebugText("  PatrolNode(success): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  g_RenderState.RenderDebugText("  PatrolNode(fail): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult RusherBasePathNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

//  bool in_center = bb.ValueOr<bool>("InCenter", true);
 // bool is_anchor = bb.ValueOr<bool>("IsAnchor", false);
 // bool last_in_base = bb.ValueOr<bool>("LastInBase", false);
 // const Player* enemy = bb.ValueOr<const Player*>("Target", nullptr);
  bool in_center = bb.GetInCenter();
  CombatRole role = bb.GetCombatRole();
  bool last_in_base = bb.GetLastInBase();
  const Player* enemy = bb.GetTarget();
  const Path& base_path = ctx.bot->GetBasePath();

  bool on_safe = game.GetMap().GetTileId(game.GetPosition()) == kSafeTileId;
  Vector2f team_safe = ctx.bot->GetTeamSafePosition(game.GetPlayer().frequency);

  if (in_center || role == CombatRole::Anchor || last_in_base || !enemy || base_path.empty()) {
    g_RenderState.RenderDebugText("  RusherPathNode(fail): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  Vector2f position = game.GetPosition();
  float radius = game.GetShipSettings().GetRadius();

  int desired_position = 1;

  //auto search = path::PathNodeSearch::Create(*ctx.bot, base_path);

  //size_t bot_node = search->FindNearestNodeBFS(position);
 // size_t enemy_node = search->FindNearestNodeBFS(enemy->position);
 // bool high_side = bot_node > enemy_node;

  //desired_position = search->FindForwardLOSNode(position, bot_node, radius, high_side);

  //ctx.bot->GetPathfinder().CreatePath(*ctx.bot, position, desired_position, radius);



  ctx.bot->GetPathfinder().CreatePath(*ctx.bot, position, enemy->position, radius);

  g_RenderState.RenderDebugText("  RusherBasePathNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult AnchorBasePathNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  auto search = path::PathNodeSearch::Create(*ctx.bot, ctx.bot->GetBasePath());
  //const Player* enemy = GetEnemy(ctx);
  const Player* enemy = bb.GetTarget();
  bool on_safe_tile = game.GetMap().GetTileId(game.GetPosition()) == kSafeTileId;

  //bool in_center = bb.ValueOr<bool>("InCenter", true);
  //bool is_anchor_ = bb.ValueOr<bool>("IsAnchor", false);
  //bool last_in_base = bb.ValueOr<bool>("LastInBase", false);
 // const Player* enemy = bb.ValueOr<const Player*>("Target", nullptr);
  bool in_center = bb.GetInCenter();
  CombatRole role = bb.GetCombatRole();
  bool last_in_base = bb.GetLastInBase();
  int manual_distance_adjustment = bb.ValueOr<int>("anchordistance", 0);

  if ( !enemy || in_center || (role != CombatRole::Anchor && !last_in_base)) {
    g_RenderState.RenderDebugText("  AnchorBasePathNode(fail): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }



  float enemy_bullet_speed = game.GetShipSettings(enemy->ship).GetBulletSpeed();
  float enemy_bomb_speed = game.GetShipSettings(enemy->ship).GetBombSpeed();
  float bullet_alive_time = game.GetSettings().GetBulletAliveTime();
  float bomb_alive_time = game.GetSettings().GetBombAliveTime();

  Vector2f position = game.GetPosition();
  float radius = game.GetShipSettings().GetRadius();
  float thrust = game.GetThrust();
  float enemy_radius = game.GetShipSettings(enemy->ship).GetRadius();

  
  
  size_t bot_node = search->FindNearestNodeBFS(game.GetPosition());
  size_t enemy_node = search->FindNearestNodeBFS(enemy->position);

  //team_safe_node_ = ctx.bot->GetTeamSafeIndex(game.GetPlayer().frequency);
 // enemy_safe_node_ = ctx.bot->GetTeamSafeIndex(enemy_->frequency);
  // this flag is important as it allows the bot to switch reference points even if the enemy target leaked past the team
  bool high_side = bot_node > enemy_node;

  // the enemy target is player with the highest net bullet travell in the direction of the bot
  // use the target found in find enemy node to get the front most node for reference
  // use the los_decrement flag to determine which direction in the base path the refernce nodes should be calculated 
 // enemy_fore_ = search_->FindForwardLOSNode(enemy_->position, enemy_node_, enemy_radius, !high_side_);
 // enemy_aft_ = search_->FindRearLOSNode(enemy_->position, enemy_node_, enemy_radius, !high_side_);
  
  Vector2f bot_fore = search->FindForwardLOSNode(position, bot_node, radius, high_side);
  Vector2f bot_aft = search->FindRearLOSNode(position, bot_node, radius, high_side);

  //RenderWorldLine(position_, position_, bot_fore_, RGB(255, 0, 0));

  float energy_pct = ((float)game.GetPlayer().energy / game.GetMaxEnergy());

  Vector2f desired_position = bot_fore;

  // to_anchor is the direction that the enemy needs to move in to reach the bot
 // enemy_to_bot_ = Normalize(enemy_fore_ - enemy_->position);
  Vector2f bot_to_enemy = Normalize(bot_fore - position);

  // use the dot product to determine how fast the enemy is moving towards the bot
  //enemy_speed_ = Normalize(enemy->velocity).Dot(enemy_to_bot_) * enemy->velocity.Length();
  //float enemy_speed = Normalize(enemy->velocity).Dot(enemy_to_bot_) * enemy->velocity.Length();

  // this is not the true max travel, instead its relative to the dot product
  //float enemy_bullet_travel_ = (max_enemy_speed_ + enemy_bullet_speed_) * alive_time_;
  // at its current speed, how long will it take to reach the bots position
  //float enemy_pathlength_to_bot = search_->GetPathDistance(enemy_node_, bot_node_);
  //max_net_enemy_bullet_travel_ = enemy_bullet_travel_ - enemy_pathlength_to_bot;
  //enemy_weapon_travel_ = (enemy_speed_ + enemy_bullet_speed_) * bullet_alive_time;
  //max_net_enemy_bullet_travel_ = bb.ValueOr<float>(BB::EnemyNetBulletTravel, 0.0f);
  //net_enemy_weapon_travel_ = bb.GetEnemyNetBulletTravel();
  //min_enemy_time_to_bot_ = enemy_pathlength_to_bot / enemy_speed_;

  //SetEnemy(ctx);
 // CalculateTeamThreat(ctx, enemy);

      // players forward most path node that is in line of sight
  Vector2f enemy_fore = search->FindForwardLOSNode(enemy->position, enemy_node, enemy_radius, high_side);
  // the direction from the player position to the player_fore
  Vector2f enemy_to_anchor = Normalize(enemy_fore - enemy->position);

  // RenderWorldLine(position_, player.position, player_fore, RGB(255, 0, 0));

  // the speed that the enemy is moving toward or away from its forward path node
  // this isnt perfect since players do a lot of wall bouncing
  //float enemy_speed = (Normalize(enemy->velocity).Dot(enemy_to_anchor)) * enemy->velocity.Length();
  float enemy_speed = enemy->velocity.Length();  // assume that any enemy velocity is flying towards the bot

  //if (enemy_speed < 0.0f) enemy_speed = 0.0f;  // dont chase enemies that seem to be fleeing

  // bug fixed here where i was using the player heading instead of normalized velocity
  float bot_speed = (Normalize(game.GetPlayer().velocity).Dot(bot_to_enemy)) * game.GetPlayer().velocity.Length();
  //if (bot_speed < 0.0f) bot_speed = 0.0f;

  // how fast the bot/enemy are moving towards/away from each other
  float initial_speed = enemy_speed + bot_speed;
  // the desired relative speed the bot wants to achieve
  float final_speed = 0.0f;
  // how long it takes the bot to reduce this speed to 0 given the ships thrust
  float time_to_zero = std::abs(initial_speed - final_speed) / thrust;
  // how many tiles it needs to travel to adjust to this speed
  float braking_distance = ((initial_speed + final_speed) / 2.0f) * time_to_zero;
  
  // range of 0 to 10
  float energy_modifier = 10.0f * (1.0f - energy_pct);

  float desired_distance = 45.0f + braking_distance + energy_modifier;
  //float desired_distance = enemy.weapon_travel + braking_distance + energy_modifier;

  // adjust desired range based on team threat balance
  // will range from 0.25 * desired to -0.25 * desired
  //float threat_adjustment = desired_distance * 0.5f * (0.5f - (team_threat_ / (team_threat_ + enemy_team_threat_)));
  //desired_distance += threat_adjustment;
  desired_distance += (float)manual_distance_adjustment;

  // get the distance to the enemy
  float pathlength = search->GetPathDistance(bot_node, enemy_node);
  
  if (pathlength < desired_distance || AvoidInfluence(ctx)) {
    desired_position = bot_aft;
    //bb.Set<bool>("SteerBackwards", true);
    bb.SetSteerBackwards(true);
  }

  //g_RenderState.RenderDebugText("  TEAM THREAT: %f", team_threat_);
  //g_RenderState.RenderDebugText("  ENEMY TEAM THREAT: %f", enemy_team_threat_);
  RenderWorldBox(game.GetPosition(), enemy->position, 0.875f);

  if (on_safe_tile && desired_position == bot_aft) {
    ctx.bot->GetKeys().Press(VK_CONTROL);
    ctx.bot->GetPathfinder().ClearPath();
  } else {
    ctx.bot->GetPathfinder().CreatePath(*ctx.bot, position, desired_position, radius);
  }

  g_RenderState.RenderDebugText("  AnchorBasePathNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}


const Player* AnchorBasePathNode::GetEnemy(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  const Player* result = nullptr;
  float shortest_distance = std::numeric_limits<float>::max();
  //float shortest_time_to_bot = std::numeric_limits<float>::max();
 
  auto search = path::PathNodeSearch::Create(*ctx.bot, ctx.bot->GetBasePath());
  std::size_t bot_node = search->FindNearestNodeBFS(game.GetPosition());

  

  for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
    const Player& enemy = game.GetPlayers()[i];

    if (enemy.frequency != bb.GetPubTeam0() && enemy.frequency != bb.GetPubTeam1()) continue;
    if (!IsValidTarget(*ctx.bot, enemy, bb.GetCombatRole())) continue;

    // players closest path node
    size_t enemy_node = search->FindNearestNodeBFS(enemy.position);
    bool high_side = bot_node > enemy_node;
    float enemy_radius = game.GetShipSettings(enemy.ship).GetRadius();
    float bullet_speed = game.GetShipSettings(enemy.ship).GetBulletSpeed();

    Vector2f enemy_fore = search->FindForwardLOSNode(enemy.position, enemy_node, enemy_radius, high_side);
    Vector2f enemy_to_anchor = Normalize(enemy_fore - enemy.position);
    float enemy_speed = (Normalize(enemy.velocity).Dot(enemy_to_anchor)) * enemy.velocity.Length();

    // get distance between bot and the current enemy
    float enemy_distance = search->GetPathDistance(bot_node, enemy_node);
    float time_to_bot = enemy_distance / (enemy_speed + bullet_speed);
    
    if (enemy_distance < shortest_distance) {
      shortest_distance = enemy_distance;
      result = &game.GetPlayers()[i];
    }
  }
  return result;
}

void AnchorBasePathNode::CalculateTeamThreat(behavior::ExecuteContext& ctx, const Player* enemy) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  float bullet_alive_time = game.GetSettings().GetBulletAliveTime();
  float bomb_alive_time = game.GetSettings().GetBombAliveTime();

  // calculate team threat based on the players energy, bullet travel, and distance to the enemy target
  for (std::size_t i = 0; i < bb.GetTeamList().size(); i++) {
    const Player* player = bb.GetTeamList()[i];

    if (player->dead || !ctx.bot->GetRegions().IsConnected(player->position, game.GetPosition())) continue;

    float player_bullet_speed = game.GetShipSettings(player->ship).GetBulletSpeed();
    float player_bomb_speed = game.GetShipSettings(player->ship).GetBombSpeed();
    float player_radius = game.GetShipSettings(player->ship).GetRadius();
  }
}

bool AnchorBasePathNode::AvoidInfluence(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  float radius = game.GetShipSettings().GetRadius();

  Vector2f pos = game.GetPlayer().position;
  float thrust = game.GetThrust();
  float speed = game.GetPlayer().velocity.Length();
  Vector2f direction = Normalize(game.GetPlayer().velocity);

  // how fast the bot/enemy are moving towards/away from each other
  float initial_speed = speed;
  // the desired relative speed the bot wants to achieve
  float final_speed = 0.0f;
  // how long it takes the bot to reduce this speed to 0 given the ships thrust
  float time_to_zero = std::abs(initial_speed - final_speed) / thrust;
  // how many tiles it needs to travel to adjust to this speed
  float braking_distance = ((initial_speed + final_speed) / 2.0f) * time_to_zero;

  for (float i = 0.0f; i <= braking_distance; i++) {
    Vector2f check = pos + direction * i;

    if (game.GetMap().IsSolid(check)) break;

    if (ctx.bot->GetInfluenceMap().GetValue(check) > 0.0f) {
      return true;
    }
  }



  #if 0
  // Check the corners of the ship to see if it's touching any influenced tiles
  Vector2f positions[] = {
      pos,
      pos + Vector2f(radius, radius),
      pos + Vector2f(-radius, -radius),
      pos + Vector2f(radius, -radius),
      pos + Vector2f(-radius, radius),
  };

  for (size_t i = 0; i < sizeof(positions) / sizeof(*positions); ++i) {
    Vector2f check = positions[i];

    if (ctx.bot->GetInfluenceMap().GetValue(check) > 0.0f) {
      Vector2f offsets[] = {
          Vector2f(0, -1), Vector2f(0, 1),  Vector2f(-1, 0), Vector2f(1, 0),
          Vector2f(1, 1),  Vector2f(1, -1), Vector2f(-1, 1), Vector2f(-1, -1),
      };

      size_t best_index = 0;
      float best_value = 1000000.0f;

      for (size_t i = 0; i < sizeof(offsets) / sizeof(*offsets); ++i) {
        float value = ctx.bot->GetInfluenceMap().GetValue(check + offsets[i]);
        float heading_value = std::abs(Normalize(offsets[i]).Dot(Normalize(game.GetPlayer().GetHeading())));

        value *= heading_value;

        if (value < best_value && game.GetMap().CanTraverse(pos, check + offsets[i], radius)) {
          best_value = value;
          best_index = i;
        }
      }

      // ctx.bot->GetSteering().Seek(check + offsets[best_index], 1000.0f);
      return true;
    }
  }
  #endif
  return false;
}


behavior::ExecuteResult FollowPathNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  float radius = game.GetShipSettings().GetRadius();
  std::vector<Vector2f> path = ctx.bot->GetPathfinder().GetPath();

  size_t path_size = path.size();

  if (path.empty()) {
    g_RenderState.RenderDebugText("  FollowPathNode(empty): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  Vector2f current = path.front();
  Vector2f from = game.GetPosition();

  // Fix an issue where the pathfinder generates nodes in the center of the tile
  // but the ship can't occupy the exact center. This is only an issue without path smoothing.
  if (!path.empty() && (u16)from.x == (u16)path.at(0).x && (u16)from.y == (u16)path.at(0).y) {
    path.erase(path.begin());

    if (!path.empty()) {
      current = path.front();
    }
  }

  // this code culls the front of the path as the ship moves along its nodes
  // it culls nodes using line of sight
  while (path.size() > 1) {
    // this check is good with wider than ship radius, keeps ship from grazing corners of walls
    // works very good in tubes because overwide radius check ensures the nodes wont get wiped until
    // the bot is on top of them
    // this check is directly tied to how the pathfinder decides to rebuild the path in the createpath function
    // value of 1.055 will clear line of sight in tubes
    // value of 1.1 wont clear line of sight in tubes
    bool hit = DiameterRayCastHit(*ctx.bot, game.GetPosition(), path[1], radius * 1.055f);

    if (!hit) {
      path.erase(path.begin());
      current = path.front();
    } else {
      break;
    }
  }

  if (path.size() == 1 && path.front().DistanceSq(game.GetPosition()) < 2.0f * 2.0f) {
    path.clear();
  }

  if (path.size() != path_size) {
    ctx.bot->GetPathfinder().SetPath(path); 
  }

  ctx.bot->Move(current, 0.0f);
  g_RenderState.RenderDebugText("  FollowPathNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult MineSweeperNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  for (Weapon* weapon : game.GetWeapons()) {
    const Player* weapon_player = game.GetPlayerById(weapon->GetPlayerId());

    if (weapon_player == nullptr) continue;
    if (weapon_player->frequency == game.GetPlayer().frequency) continue;
    if (weapon->IsMine()) {
      if (weapon->GetPosition().Distance(game.GetPosition()) < 8.0f && game.GetPlayer().repels > 0) {
        game.Repel(ctx.bot->GetKeys());

        g_RenderState.RenderDebugText("  MineSweeperNode(success): %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Success;
      }
    }
  }

  g_RenderState.RenderDebugText("  MineSweeperNode(fail): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult InLineOfSightNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  bool in_sight = false;

  //const Player* target = bb.ValueOr<const Player*>("Target", nullptr);
  const Player* target = bb.GetTarget();

  if (target) {
    // Pull out the radius check to the edge of the target's ship to be more accurate with raycast
    Vector2f target_front = target->position + Normalize(game.GetPosition() - target->position) *
                                                   game.GetShipSettings(target->ship).GetRadius();

    // This probably shouldn't be a radius ray cast because they are still in line of sight even if just one piece of
    // the ship is viewable.
    if (!RadiusRayCastHit(*ctx.bot, game.GetPosition(), target_front, game.GetShipSettings().GetRadius())) {
      g_RenderState.RenderDebugText("  InLineOfSightNode (success): %llu", timer.GetElapsedTime());
      //bb.Set<bool>("TargetInSight", true);
      bb.SetTargetInSight(true);
      return behavior::ExecuteResult::Success;
    }
  }

  //bb.Set<bool>("TargetInSight", false);
  bb.SetTargetInSight(false);
  g_RenderState.RenderDebugText("  InLineOfSightNode (fail): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult IsAnchorNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& bb = ctx.bot->GetBlackboard();

  //if (bb.ValueOr<bool>("IsAnchor", false) && !bb.ValueOr<bool>("InCenter", true)) {
  //if (bb.GetCombatRole() == CombatRole::Anchor && !bb.GetInCenter()) {
  if (bb.GetCombatRole() == CombatRole::Anchor) {
    g_RenderState.RenderDebugText("  IsAnchorNode(success): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  g_RenderState.RenderDebugText("  IsAnchorNode(fail): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult NotInCenterNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& bb = ctx.bot->GetBlackboard();

  bool in_center = ctx.bot->GetRegions().IsConnected(Vector2f(512, 512), ctx.bot->GetGame().GetPlayer().position);

 if (!in_center) {
    g_RenderState.RenderDebugText("  NotInCenterNode(success): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  g_RenderState.RenderDebugText("  NotInCenterNode(fail): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult InCenterNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& bb = ctx.bot->GetBlackboard();

  bool in_center = ctx.bot->GetRegions().IsConnected(Vector2f(512, 512), ctx.bot->GetGame().GetPlayer().position);

  if (in_center) {
    g_RenderState.RenderDebugText("  InCenterNode(success): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  g_RenderState.RenderDebugText("  InCenterNode(fail): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult BouncingShotNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  //const Player* target = bb.ValueOr<const Player*>("Target", nullptr);
  //bool in_sight = bb.ValueOr<bool>("TargetInSight", false);
  const Player* target = bb.GetTarget();
  bool in_sight = bb.GetTargetInSight();

  if (!target || in_sight) {
    g_RenderState.RenderDebugText("  BouncingShotNode(fail): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  float energy_pct = (float)game.GetPlayer().energy / game.GetMaxEnergy() * 100.0f;

  //if (bb.ValueOr<bool>("IsAnchor", false) && energy_pct < 50.0f) {
  if (bb.GetCombatRole() == CombatRole::Anchor && energy_pct < 50.0f) {
    g_RenderState.RenderDebugText("  BouncingShotNode(fail): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  float target_radius = game.GetShipSettings(target->ship).GetRadius();

  ShotResult bResult =
      ctx.bot->GetShooter().BouncingBombShot(*ctx.bot, target->position, target->velocity, target_radius);
  ShotResult gResult =
      ctx.bot->GetShooter().BouncingBulletShot(*ctx.bot, target->position, target->velocity, target_radius);

  float bomb_delay = game.GetShipSettings().GetBombFireDelay();
  //float bomb_timer = bb.ValueOr<float>("BombTimer", 0.0f);
  float bomb_cooldown = bb.GetBombCooldown();
  bomb_cooldown -= ctx.dt;
  //bb.Set<float>("BombTimer", bomb_timer);
  bb.SetBombCooldown(bomb_cooldown);

  if (game.GetMap().GetTileId(game.GetPosition()) != marvin::kSafeTileId) {
    if (bResult.hit && bomb_cooldown <= 0.0f) {
      ctx.bot->GetKeys().Press(VK_TAB);
      //bb.Set<float>("BombTimer", bomb_delay);
      bb.SetBombCooldown(bomb_delay);
    } else if (gResult.hit) {
      ctx.bot->GetKeys().Press(VK_CONTROL);
    }
  }

  g_RenderState.RenderDebugText("  BouncingShotNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult ShootEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  //const auto target_player = bb.ValueOr<const Player*>("Target", nullptr);
  const Player* target_player = bb.GetTarget();

  if (!target_player) {
    g_RenderState.RenderDebugText("  ShootEnemyNode(fail): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }
  const Player& target = *target_player;
  const Player& bot = game.GetPlayer();

  float target_radius = game.GetShipSettings(target.ship).GetRadius();
  float radius = game.GetShipSettings().GetRadius();
  Vector2f side = Perpendicular(bot.GetHeading());

  float proj_speed = (float)game.GetShipSettings(bot.ship).GetBulletSpeed();
  float alive_time = (float)game.GetSettings().GetBulletAliveTime();

  Vector2f solution = target.position;
  //bb.Set<Vector2f>("Solution", solution);
  bb.SetSolution(solution);

  float radius_multiplier = 1.4f;

  int weapon_key = VK_CONTROL;

  bool gun_hit = false;
  bool bomb_hit = false;

  std::vector<Vector2f> positions;

  if (game.GetShipSettings().HasDoubleBarrel()) {
    positions.push_back(bot.position + side * radius * 0.8f);
    positions.push_back(bot.position - side * radius * 0.8f);
  } else {
    positions.push_back(bot.position);
  }

  if (game.GetPlayer().multifire_status) {
    positions.push_back(bot.position);
    positions.push_back(bot.position);
  }
  // bomb element
  positions.push_back(bot.position);

  std::size_t iSize = positions.size();

  for (std::size_t i = 0; i < positions.size(); i++) {
    bool onBomb = i == (iSize - 1);

    // last element switch to bomb settings
    if (onBomb) {
      // safe bomb distance
      if (bot.position.Distance(target.position) <
          (((game.GetSettings().GetBombExplodePixels() * game.GetShipSettings().GetMaxBombs())) +
           target_radius)) {
        continue;
      }
      weapon_key = VK_TAB;
      proj_speed = game.GetShipSettings(bot.ship).GetBombSpeed();
      alive_time = game.GetSettings().GetBombAliveTime();
      radius_multiplier = 1.0f;
    }

    Vector2f weapon_velocity = bot.velocity + bot.GetHeading() * proj_speed;
    float adjusted_proj_speed = weapon_velocity.Length();

    ShotResult result = ctx.bot->GetShooter().CalculateShot(bot.position, target.position, bot.velocity,
                                                            target.velocity, proj_speed);
    solution = result.solution;

    if (result.hit) {
      if (!onBomb) {
        //bb.Set<Vector2f>("Solution", solution);
        bb.SetSolution(solution);
      }


      float bullet_travel = (weapon_velocity * alive_time).Length();

      RenderWorldLine(bot.position, bot.position, solution, RGB(100, 0, 0));
      RenderWorldLine(bot.position, bot.position, bot.position + bot.GetHeading() * (solution - bot.position).Length(),
                      RGB(100, 100, 0));

      RenderWorldLine(bot.position, bot.position, bot.position + Normalize(weapon_velocity) * bullet_travel,
                      onBomb ? RGB(0, 0, 100) : RGB(0, 100, 100));


      // Use target position in the distance calculation instead of solution so it will still aim at them while they are
      // moving away
      if (CanShoot(game, game.GetPosition(), target.position, weapon_velocity, alive_time)) {
        if (!onBomb && PreferGuns(ctx) && iSize == positions.size()) {
          positions.pop_back();
        }
        // if the target is cloaking and bot doesnt have xradar make the bot shoot wide
        if (!(game.GetPlayer().status & 4)) {
          if (target.status & 2) {
            radius_multiplier = 3.0f;
          }
        }

        if (bb.GetCombatDifficulty() == CombatDifficulty::Nerf) {
          radius_multiplier += 1.0f;
        }

        float nearby_radius = target_radius * radius_multiplier;

        Vector2f bBox_min = solution - Vector2f(nearby_radius, nearby_radius);
        Vector2f bBox_max = solution + Vector2f(nearby_radius, nearby_radius);
        Vector2f box_extent = bBox_max - bBox_min;
        float dist;
        Vector2f norm;
        Vector2f heading = bot.GetHeading();

        if (game.GetPlayer().multifire_status) {
          if (i == iSize - 2) {
            heading = game.GetPlayer().GetHeading(game.GetShipSettings().GetMultiFireAngle());
          } else if (i == iSize - 3) {
            heading = game.GetPlayer().GetHeading(-game.GetShipSettings().GetMultiFireAngle());
          }
        }


        RenderWorldBox(bot.position, bBox_min, bBox_min + box_extent, RGB(0, 255, 0));

        if (FloatingRayBoxIntersect(bot.position, bot.GetHeading(), solution, nearby_radius, &dist, &norm)) {
        //if (FloatingRayBoxIntersect(bot.position, Normalize(weapon_velocity), solution, nearby_radius, &dist, &norm)) {
          ctx.bot->GetKeys().Press(weapon_key);
          g_RenderState.RenderDebugText("  ShootEnemyNode (success): %llu", timer.GetElapsedTime());
          return behavior::ExecuteResult::Success;
        }
      }
    }
  }
  g_RenderState.RenderDebugText("  ShootEnemyNode (fail): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

bool ShootEnemyNode::PreferGuns(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();

  bool use_guns = false;

  float bomb_dps = game.GetShipSettings().GetBombFireDelay() * game.GetSettings().BombDamageLevel;

  float gun_damage = game.GetSettings().BulletDamageLevel + game.GetShipSettings().GetMaxGuns() * game.GetSettings().BulletDamageUpgrade;
  float gun_dps = game.GetShipSettings().GetBulletFireDelay() * gun_damage;

  if (game.GetShipSettings().HasDoubleBarrel() || bomb_dps < gun_dps) {
    use_guns = true;
  }
  return use_guns;
}

behavior::ExecuteResult MoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  //Vector2f position = bb.ValueOr<Vector2f>("Solution", Vector2f());
  Vector2f position = bb.GetSolution();

  bool in_safe = game.GetMap().GetTileId(game.GetPlayer().position) == marvin::kSafeTileId;

  float hover_distance = 0.0f;

  if (in_safe)
    hover_distance = 0.0f;
  else {
    hover_distance = 10.0f;
  }

  ctx.bot->Move(position, hover_distance);

  ctx.bot->GetSteering().Face(*ctx.bot, position);

  g_RenderState.RenderDebugText("  MoveToEnemyNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

}  // namespace bot
}  // namespace marvin
