#include "Bot.h"

#include <chrono>
#include <cstring>
#include <limits>

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


Bot::Bot(std::shared_ptr<marvin::GameProxy> game) : game_(std::move(game)), time_(*game_) {
  srand((unsigned int)(time_.GetTime() + game_->GetPlayer().id));
  base_paths_ = nullptr;
  goals_ = nullptr;
  Load();
}

void Bot::LoadForRadius() {
  auto processor = std::make_unique<path::NodeProcessor>(*game_);
  regions_ = std::make_unique<RegionRegistry>(game_->GetMap());
  regions_->CreateAll(game_->GetMap(), radius_);
  pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor), *regions_);
  pathfinder_->CreateMapWeights(game_->GetMap(), radius_);
  pathfinder_->SetPathableNodes(game_->GetMap(), radius_);

  // if base paths isnt null then its safe to rebuild
  if (base_paths_) {
    base_paths_ = std::make_unique<BasePaths>(goals_->GetGoals(), radius_, *pathfinder_, game_->GetMap());
  }
}

void Bot::Load() {
  PerformanceTimer timer;
  log.Write("LOADING BOT", timer.GetElapsedTime());
  
  std::unique_ptr<BehaviorBuilder> builder;
  Zone zone = game_->GetZone();
  std::string map = game_->GetMapFile();
  log.Write("Map " + game_->GetMapFile() + " found", timer.GetElapsedTime());
  radius_ = game_->GetRadius();

  blackboard_ = std::make_unique<Blackboard>();
  command_system_ = std::make_unique<CommandSystem>(zone);
  auto processor = std::make_unique<path::NodeProcessor>(*game_);
  log.Write("Processor created", timer.GetElapsedTime());
  influence_map_ = std::make_unique<InfluenceMap>();
  log.Write("Influence Map created", timer.GetElapsedTime());
  regions_ = std::make_unique<RegionRegistry>(game_->GetMap());
  regions_->CreateAll(game_->GetMap(), radius_);
  log.Write("Regions created", timer.GetElapsedTime());
  pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor), *regions_);
  log.Write("Pathfinder created", timer.GetElapsedTime());
  pathfinder_->CreateMapWeights(game_->GetMap(), radius_);
  log.Write("Map Weights created", timer.GetElapsedTime());
  pathfinder_->SetPathableNodes(game_->GetMap(), radius_);
  log.Write("Pathable Nodes created", timer.GetElapsedTime());

  
  switch (zone) {
    case Zone::Devastation: {
      goals_ = std::make_unique<deva::BaseDuelWarpCoords>(game_->GetMapFile());
      if (map == "bdelite.lvl") {
        log.Write("Building Training behavior tree.");
        builder = std::make_unique<training::TrainingBehaviorBuilder>();
      } else {
        if (goals_->HasCoords()) {
          log.Write("Building Devastation behavior tree.");
          base_paths_ = std::make_unique<BasePaths>(goals_->GetGoals(), radius_, *pathfinder_, game_->GetMap());
          builder = std::make_unique<deva::DevastationBehaviorBuilder>();
        } else {
          game_->SendChatMessage(
              "Warning: I don't have base duel coords for this arena, using default behavior tree.");
          log.Write("Building Default behavior tree.");
          builder = std::make_unique<DefaultBehaviorBuilder>();
        }
      }
    } break;
    case Zone::ExtremeGames: {
      // builder = std::make_unique<eg::ExtremeGamesBehaviorBuilder>();
    } break;
    case Zone::GalaxySports: {
      // builder = std::make_unique<gs::GalaxySportsBehaviorBuilder>();
    } break;
    case Zone::Hockey: {
      // builder = std::make_unique<hz::HockeyBehaviorBuilder>();
    } break;
    case Zone::Hyperspace: {
      goals_ = std::make_unique<hs::HSFlagRooms>();
      base_paths_ = std::make_unique<BasePaths>(goals_->GetGoals(), radius_, *pathfinder_, game_->GetMap());
      builder = std::make_unique<hs::HyperspaceBehaviorBuilder>();
      log.Write("Building Hyperspace behavior tree.");
    } break;
    case Zone::PowerBall: {
      // builder = std::make_unique<pb::PowerBallBehaviorBuilder>();
    } break;
    default: {
      builder = std::make_unique<DefaultBehaviorBuilder>();
    } break;
  }

  ctx_.bot = this;

  this->behavior_ = builder->Build(*this);
  log.Write("Behavior Built", timer.GetElapsedTime());
  log.Write("BOT LOADED - TOTAL TIME", timer.TimeSinceConstruction());
}

void Bot::Update(float dt) {
  PerformanceTimer timer;
  g_RenderState.debug_y = 30.0f;



  keys_.ReleaseAll();

  float radius = game_->GetRadius();
  int ship = game_->GetPlayer().ship;
  
  UpdateState state = game_->Update(dt);
  g_RenderState.RenderDebugText("GameUpdate: %llu", timer.GetElapsedTime());

  if (state == UpdateState::Wait) {
    return;
  }
  
  if (state == UpdateState::Reload) {
    log.Write("GAME REQUESTED RELOAD");
    Load();
    return;
  }

  if (radius != radius_) {
    log.Write("SHIP RADIUS CHANGED");
    radius_ = radius;
    LoadForRadius();
    return;
  }

  steering_.Reset();
  ctx_.dt = dt;

  if (game_->GetZone() == Zone::Devastation) {
    if (game_->GetPlayer().name == "FrogBot") {
      if (game_->GetMapFile() != "bdelite.lvl") {
        if (GetTime().TimedActionDelay("arenachange", 200)) {
          game_->SendChatMessage("?go BDElite");
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

  #if DEBUG_RENDER_REGION_REGISTRY
    regions_->DebugUpdate(game_->GetPosition());
    g_RenderState.RenderDebugText("RegionDebugUpdate: %llu", timer.GetElapsedTime());
#endif

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

void Bot::Move(const Vector2f& target, float target_distance) {
  const Player& bot_player = game_->GetPlayer();
  float distance = bot_player.position.Distance(target);

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
  if (!game_->GetPlayer().active) {
    result = false;
  }
  return result;
}

namespace bot {

behavior::ExecuteResult CommandNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  bool executed = false;

  for (ChatMessage& chat : game.GetChat()) {
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
        game.SetEnergy(100.0f);
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

  if (bb.GetCommandRequest() == CommandRequestType::ShipChange) {
    if (cShip != dShip) {
      if (ctx.bot->GetTime().RepeatedActionDelay("shipchange", ship_cooldown)) {
        game.SetEnergy(100.0f);
        if (!game.SetShip(dShip)) {
          ctx.bot->GetTime().RepeatedActionDelay("shipchange", 0);
        }
        return behavior::ExecuteResult::Failure;
        g_RenderState.RenderDebugText("  SetShipNode(fail): %llu", timer.GetElapsedTime());
      }
    } else {
      bb.SetCommandRequest(CommandRequestType::None);
    }
  }

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
        game.SetEnergy(100.0f);
        game.SetFreq(freq);
      }

      for (ChatMessage& chat : game.GetChat()) {
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
        if (!in_center && player.active) {
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

    // ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPosition(), MapCoord(839, 223), 0.95);
  // ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPosition(), MapCoord(522, 381), 0.95);
 //  ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPosition(), MapCoord(399, 543), 0.95);
  ctx.bot->GetPathfinder().CreatePath(*ctx.bot, game.GetPosition(), MapCoord(962, 63), 0.95);
  return behavior::ExecuteResult::Failure;

  if (!game.GetPlayer().active) {
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

  //bb.Set<const Player*>("Target", target);
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

  auto search = path::PathNodeSearch::Create(*ctx.bot, base_path);
  int bot_node = (int)search->FindNearestNodeBFS(game.GetPosition());

  float max_net_player_bullet_travel = std::numeric_limits<float>::lowest();
  const Player* target = nullptr;

  for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
    const Player& player = game.GetPlayers()[i];

    if (!IsValidTarget(*ctx.bot, player, combat_role)) {
      continue;
    }

    // TODO: look at target bomb speed and bouncing status
    float alive_time = game.GetSettings().GetBulletAliveTime();
    float player_bullet_speed = game.GetShipSettings(player.ship).GetBulletSpeed();

    int player_node = (int)search->FindNearestNodeBFS(player.position);
    bool high_side = bot_node < player_node;

    Vector2f player_fore = search->FindForwardLOSNode(player.position, player_node, 0.8f, high_side);
    //RenderWorldBox(game.GetPosition(), player_fore, 0.5f);
    Vector2f player_to_bot = Normalize(player_fore - player.position);
    //RenderDirection(game.GetPosition(), player.position, player_to_bot, player_fore.Distance(player.position));

    // if the player can see the bot then use the bot as the direction to point towards
    if (!DiameterRayCastHit(*ctx.bot, player.position, game.GetPosition(), 0.8f)) {
      player_to_bot = Normalize(game.GetPosition() - player.position);
    }

    // multiply the players speed by a ratio of how "true" its flying towards the bot
    float player_speed = Normalize(player.velocity).Dot(player_to_bot) * player.velocity.Length();
    // if the target is dead, set its speed to 0
    if (!player.active) {
      player_speed = 0.0f;
    }
    // if the player is flying at full speed in the direction of the player_fore position, then it will reach its maximum bullet travel
    float player_bullet_travel = (player_speed + player_bullet_speed) * alive_time;
    // at its current speed, how long will it take to reach the bots position
    float player_pathlength_to_bot = search->GetPathDistance(player_node, bot_node);
    float net_player_bullet_travel = player_bullet_travel - player_pathlength_to_bot;

    if (net_player_bullet_travel > max_net_player_bullet_travel) {
      max_net_player_bullet_travel = net_player_bullet_travel;
      target = &game.GetPlayers()[i];
      result = behavior::ExecuteResult::Success;
    }
  }

  //bb.Set<float>(BB::EnemyNetBulletTravel, max_net_player_bullet_travel);
  bb.SetEnemyNetBulletTravel(max_net_player_bullet_travel);
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

  if (in_center || role == CombatRole::Anchor || last_in_base || !enemy || base_path.empty()) {
    g_RenderState.RenderDebugText("  RusherPathNode(fail): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  Vector2f position = game.GetPosition();
  float radius = game.GetShipSettings().GetRadius();

  Vector2f desired_position;

  auto search = path::PathNodeSearch::Create(*ctx.bot, base_path);

  size_t bot_node = search->FindNearestNodeBFS(position);
  size_t enemy_node = search->FindNearestNodeBFS(enemy->position);
  bool high_side = bot_node > enemy_node;

  desired_position = search->FindForwardLOSNode(position, bot_node, radius, high_side);

  ctx.bot->GetPathfinder().CreatePath(*ctx.bot, position, desired_position, radius);

  g_RenderState.RenderDebugText("  RusherBasePathNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult AnchorBasePathNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  //bool in_center = bb.ValueOr<bool>("InCenter", true);
  //bool is_anchor_ = bb.ValueOr<bool>("IsAnchor", false);
  //bool last_in_base = bb.ValueOr<bool>("LastInBase", false);
 // const Player* enemy = bb.ValueOr<const Player*>("Target", nullptr);
  bool in_center = bb.GetInCenter();
  role_ = bb.GetCombatRole();
  bool last_in_base = bb.GetLastInBase();
  const Player* enemy = bb.GetTarget();

  if (!enemy || in_center || (role_ != CombatRole::Anchor && !last_in_base)) {
    g_RenderState.RenderDebugText("  AnchorBasePathNode(fail): %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  enemy_bullet_speed_ = game.GetShipSettings(enemy->ship).GetBulletSpeed();
  alive_time_ = game.GetSettings().GetBulletAliveTime();
  position_ = game.GetPosition();
  radius_ = game.GetShipSettings().GetRadius();
  float thrust = game.GetThrust();
  enemy_radius_ = game.GetShipSettings(enemy->ship).GetRadius();

  base_path_ = ctx.bot->GetBasePath();
  search_ = path::PathNodeSearch::Create(*ctx.bot, base_path_);
  bot_node_ = search_->FindNearestNodeBFS(position_);
  enemy_node_ = search_->FindNearestNodeBFS(enemy->position);
  team_safe_node_ = ctx.bot->GetTeamSafeIndex(game.GetPlayer().frequency);
  enemy_safe_node_ = ctx.bot->GetTeamSafeIndex(enemy->frequency);
  // this flag is important as it allows the bot to switch reference points even if the enemy target leaked past the team
  high_side_ = bot_node_ > enemy_node_;

  // the enemy target is player with the highest net bullet travell in the direction of the bot
  // use the target found in find enemy node to get the front most node for reference
  // use the los_decrement flag to determine which direction in the base path the refernce nodes should be calculated 
  enemy_fore_ = search_->FindForwardLOSNode(enemy->position, enemy_node_, enemy_radius_, !high_side_);
  enemy_aft_ = search_->FindRearLOSNode(enemy->position, enemy_node_, enemy_radius_, !high_side_);
  bot_fore_ = search_->FindForwardLOSNode(position_, bot_node_, radius_, high_side_);
  bot_aft_ = search_->FindRearLOSNode(position_, bot_node_, radius_, high_side_);

  //RenderWorldLine(position_, position_, bot_fore_, RGB(255, 0, 0));

  enemy_team_threat_ = 0.0f;
  team_threat_ = 0.0f;
  float energy_pct = ((float)game.GetPlayer().energy / game.GetMaxEnergy());

  desired_position_ = bot_fore_;

  // to_anchor is the direction that the enemy needs to move in to reach the bot
  enemy_to_bot_ = Normalize(enemy_fore_ - enemy->position);
  bot_to_enemy_ = Normalize(bot_fore_ - position_);

  // use the dot product to determine how fast the enemy is moving towards the bot
  max_enemy_speed_ = Normalize(enemy->velocity).Dot(enemy_to_bot_) * enemy->velocity.Length();
  float enemy_speed = Normalize(enemy->velocity).Dot(enemy_to_bot_) * enemy->velocity.Length();
  // if the target is dead, set its speed to 0
  if (!enemy->active) {
    max_enemy_speed_ = 0.0f;
  }
  // this is not the true max travel, instead its relative to the dot product
  //float enemy_bullet_travel_ = (max_enemy_speed_ + enemy_bullet_speed_) * alive_time_;
  // at its current speed, how long will it take to reach the bots position
  float enemy_pathlength_to_bot = search_->GetPathDistance(enemy_node_, bot_node_);
  //max_net_enemy_bullet_travel_ = enemy_bullet_travel_ - enemy_pathlength_to_bot;
  max_enemy_bullet_travel_ = (max_enemy_speed_ + enemy_bullet_speed_) * alive_time_;
  //max_net_enemy_bullet_travel_ = bb.ValueOr<float>(BB::EnemyNetBulletTravel, 0.0f);
  max_net_enemy_bullet_travel_ = bb.GetEnemyNetBulletTravel();
  min_enemy_time_to_bot_ = enemy_pathlength_to_bot / max_enemy_speed_;

  CalculateEnemyThreat(ctx, enemy);
  CalculateTeamThreat(ctx, enemy);

  // assume player is always facing its direction of travel and ignore its heading
  //Vector2f enemy_bullet_velocity = Normalize(enemy->velocity) * enemy_bullet_speed_ + enemy->velocity;
  // float max_bullet_travel = enemy_bullet_velocity.Length() * alive_time;

  //g_RenderState.RenderDebugText("  MAXENEMYSPEED: %f", max_enemy_speed);
  //g_RenderState.RenderDebugText("  MAXTRAVEL: %f", max_bullet_travel);

  // bug fixed here where i was using the player heading instead of normalized velocity
  float bot_speed = (Normalize(game.GetPlayer().velocity).Dot(bot_to_enemy_)) * game.GetPlayer().velocity.Length();

  // how fast the bot/enemy are moving towards/away from each other
  float initial_speed = max_enemy_speed_ + bot_speed;
  // the desired relative speed the bot wants to achieve
  float final_speed = 0.0f;
  // how long it takes the bot to reduce this speed to 0 given the ships thrust
  float time_to_zero = std::abs(initial_speed - final_speed) / thrust;
  // how many tiles it needs to travel to adjust to this speed
  float braking_distance = ((initial_speed + final_speed) / 2.0f) * time_to_zero;

  // this reflects the likelyhood that a player could hit the bot at the bullets travel length
  float bullet_travel_multiplier = 1.0f;
  
  // range of 0 to 10
  float energy_modifier = 10.0f * (1.0f - energy_pct);

  //max_bullet_travel *= bullet_travel_multiplier;

  float desired_distance = max_enemy_bullet_travel_ + braking_distance + energy_modifier;

  // adjust desired range based on team threat balance
  // will range from 0.25 * desired to -0.25 * desired
  float threat_adjustment = desired_distance * 0.5f * (0.5f - (team_threat_ / (team_threat_ + enemy_team_threat_)));
  desired_distance += threat_adjustment;

  // get the distance to the enemy
  float pathlength = search_->GetPathDistance(bot_node_, enemy_node_);
  
  if (pathlength < desired_distance || AvoidInfluence(ctx)) {
    desired_position_ = bot_aft_;
    //bb.Set<bool>("SteerBackwards", true);
    bb.SetSteerBackwards(true);
  }

  g_RenderState.RenderDebugText("  TEAM THREAT: %f", team_threat_);
  g_RenderState.RenderDebugText("  ENEMY TEAM THREAT: %f", enemy_team_threat_);
  RenderWorldBox(game.GetPosition(), enemy->position, 0.875f);

  ctx.bot->GetPathfinder().CreatePath(*ctx.bot, position_, desired_position_, radius_);

  g_RenderState.RenderDebugText("  AnchorBasePathNode(success): %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

// note: the enemy argument is always the closest enemy to the bot in a base
void AnchorBasePathNode::CalculateEnemyThreat(behavior::ExecuteContext& ctx, const Player* enemy) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  for (std::size_t i = 0; i < bb.GetEnemyList().size(); i++) {
    const Player* player = bb.GetEnemyList()[i];

    if (!IsValidTarget(*ctx.bot, *player, role_)) continue;

    float player_bullet_speed = game.GetShipSettings(player->ship).GetBulletSpeed();
    float player_radius = game.GetShipSettings(player->ship).GetRadius();

    // players closest path node
    size_t player_node = search_->FindNearestNodeBFS(player->position);
    bool high_side = bot_node_ < player_node;

    /* 
    If the player is not on the same side of the base as the enemy target, then one or the other has leaked past the anchor bot.

    Ignore players not on the same side because they wont be accurate calculations.  

    When the bot is facing opposition from both sides these calculations will only be relevant to players on the same side as 
    the current target so it won't watch whats happening behind it, but the find enemy in base node will allow it to switch targets 
    if it attepmts to back pedal into the enemies behind it. 
    */

    if (high_side == high_side_) {
      continue;
    }
 
    // players forward most path node that is in line of sight
    Vector2f player_fore =
        search_->FindForwardLOSNode(player->position, player_node, player_radius, high_side);
    // the direction from the player position to the player_fore
    Vector2f player_to_anchor = Normalize(player_fore - player->position);

    //RenderWorldLine(position_, player.position, player_fore, RGB(255, 0, 0));

    // the speed that the enemy is moving toward or away from its forward path node
    float player_speed = (Normalize(player->velocity).Dot(player_to_anchor)) * player->velocity.Length();
    // if the player is dead set it's speed to 0
    if (!player->active) {
      player_speed = 0.0f;
    }

    // get distance between bot and the current enemy
    float player_pathlength_to_bot = search_->GetPathDistance(bot_node_, player_node);
    float player_pathlength_to_enemy = search_->GetPathDistance(enemy_node_, player_node);
    // using the current enemy speed, determine its estimated time to reach the bot
    float player_time_to_bot = player_pathlength_to_bot / player_speed;
    // get the current enemy bullet travel
    float player_bullet_travel = (player_speed + player_bullet_speed) * alive_time_;
    // how far the current enemy bullets will travel past the target enemy
    float net_player_bullet_travel = player_bullet_travel - player_pathlength_to_bot;

    if (player->active) {
      float threat_distance = player_bullet_travel - player_pathlength_to_enemy;
      if (threat_distance > 0.0f) {
        enemy_team_threat_ += player->energy * (threat_distance / player_bullet_travel);
      }
    }

    // get the bullet travel distance 
    if (net_player_bullet_travel > max_net_enemy_bullet_travel_) {
      max_enemy_bullet_travel_ = player_bullet_travel;
      max_net_enemy_bullet_travel_ = net_player_bullet_travel;
    }
    if (player_time_to_bot < min_enemy_time_to_bot_) {
      min_enemy_time_to_bot_ = player_time_to_bot;
      max_enemy_speed_ = player_speed;
    }
  }
}

void AnchorBasePathNode::CalculateTeamThreat(behavior::ExecuteContext& ctx, const Player* enemy) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();

  // calculate team threat based on the players energy, bullet travel, and distance to the enemy target
  for (std::size_t i = 0; i < bb.GetTeamList().size(); i++) {
    const Player* player = bb.GetTeamList()[i];

    if (!player->active || !ctx.bot->GetRegions().IsConnected(player->position, position_)) continue;

    float player_bullet_speed = game.GetShipSettings(player->ship).GetBulletSpeed();
    float player_radius = game.GetShipSettings(player->ship).GetRadius();

    size_t player_node = search_->FindNearestNodeBFS(player->position);
    // use the enemy target for high side comparison
    bool high_side = player_node > enemy_node_;

    // if the team player leaked past the enemy target this will switch the fore back towards the enemy target
    // so that the team threat is established as team players rushing towards the enemy target
    Vector2f player_fore =
        search_->FindForwardLOSNode(player->position, player_node, player_radius, high_side);
    // the direction from the player position to the player_fore
    Vector2f player_to_enemy = Normalize(player_fore - player->position);

    // the speed that the enemy is moving toward or away from its forward path node
    float player_speed = (Normalize(player->velocity).Dot(player_to_enemy)) * player->velocity.Length();

    float player_pathlength_to_enemy = search_->GetPathDistance(enemy_node_, player_node);
    float player_bullet_travel = (player_speed + player_bullet_speed) * alive_time_;

    // a range of 0 to player energy in a linear scale
    float threat_distance = player_bullet_travel - player_pathlength_to_enemy;
    if (threat_distance > 0.0f) {
      team_threat_ += player->energy * (threat_distance / player_bullet_travel);
    }
  }
}

bool AnchorBasePathNode::AvoidInfluence(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.bot->GetBlackboard();
  float radius = game.GetShipSettings().GetRadius();

  Vector2f pos = game.GetPlayer().position;
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
    bool hit = DiameterRayCastHit(*ctx.bot, game.GetPosition(), path[0], radius * 1.055f);

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

        float nearby_radius = target_radius * radius_multiplier;

        Vector2f bBox_min = solution - Vector2f(nearby_radius, nearby_radius);
        Vector2f bBox_max = solution + Vector2f(nearby_radius, nearby_radius);
        Vector2f box_extent = bBox_max - bBox_min;
        float dist;
        Vector2f norm;
        Vector2f heading = bot.GetHeading();

        if (game.GetPlayer().multifire_status) {
          if (i == iSize - 2) {
            heading = game.GetPlayer().MultiFireDirection(game.GetShipSettings().GetMultiFireAngle(), true);
          } else if (i == iSize - 3) {
            heading = game.GetPlayer().MultiFireDirection(game.GetShipSettings().GetMultiFireAngle(), false);
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
