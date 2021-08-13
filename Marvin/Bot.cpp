#include "Bot.h"

#include <chrono>
#include <cstring>
#include <limits>

#include "Debug.h"
#include "GameProxy.h"
#include "Map.h"
#include "RayCaster.h"
#include "RegionRegistry.h"
#include "Shooter.h"
#include "platform/ContinuumGameProxy.h"
#include "platform/Platform.h"
#include "zones/Devastation.h"
#include "zones/Hyperspace.h"

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

    auto root_sequence = std::make_unique<behavior::SequenceNode>(disconnect_.get(), respawn_check_.get(),
                                                                  commands_.get(), set_ship_.get(), set_freq_.get(),
                                                                  ship_check_.get(), action_selector.get());

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

std::unique_ptr<BehaviorBuilder> CreateBehaviorBuilder(Zone zone, std::string name) {
  std::unique_ptr<BehaviorBuilder> builder;

  switch (zone) {
    case Zone::Devastation: {
      if (name == "FrogBot") {
        builder = std::make_unique<DefaultBehaviorBuilder>();
      } else {
        builder = std::make_unique<deva::DevastationBehaviorBuilder>();
      }
    } break;
    case Zone::Hyperspace: {
      builder = std::make_unique<hs::HyperspaceBehaviorBuilder>();
    } break;
    default: {
      builder = std::make_unique<DefaultBehaviorBuilder>();
    } break;
  }

  return builder;
}

Bot::Bot(std::shared_ptr<marvin::GameProxy> game) : game_(std::move(game)), steering_(*game_, keys_), time_(*game_) {
  LoadBotConstuctor();
}

void Bot::LoadBotConstuctor() {
  auto processor = std::make_unique<path::NodeProcessor>(*game_);

  regions_ = RegionRegistry::Create(game_->GetMap());

  pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor), *regions_);
  pathfinder_->CreateMapWeights(game_->GetMap());

  SetZoneVariables();

  Zone zone = game_->GetZone();
  auto builder = CreateBehaviorBuilder(zone, game_->GetPlayer().name);

  // ctx_.blackboard.Set<int>("Ship", game_->GetPlayer().ship);
  ctx_.bot = this;

  this->behavior_ = builder->Build(*this);
}

void Bot::Update(float dt) {
  g_RenderState.debug_y = 30.0f;

  keys_.ReleaseAll();

  PerformanceTimer timer;

  if (!game_->Update(dt)) {
    LoadBotConstuctor();
    return;
  }

  g_RenderState.RenderDebugText("GameUpdate: %llu", timer.GetElapsedTime());

  steering_.Reset();
#if 0
        Vector2f sDirection = game_->GetPlayer().MultiFireDirection(game_->GetShipSettings().MultiFireAngle, true);
        CastResult line = RayCast(game_->GetMap(), game_->GetPosition(), sDirection, 1000.0f);
        RenderWorldLine(game_->GetPosition(), game_->GetPosition(), line.position, RGB(100, 0, 0));      
       // sDirection = sDirection - (line.normal * (2.0f * sDirection.Dot(line.normal)));      
       // CastResult nextline = RayCast(game_->GetMap(), line.position, sDirection, 1000.0f);
       // RenderWorldLine(game_->GetPosition(), line.position, nextline.position, RGB(100, 0, 0));
#endif
  // RenderPath(game_->GetPosition(), GetBasePath());
  // RenderText(std::to_string((game_->GetPlayer().multifire_capable)), GetWindowCenter() - Vector2f(0, 40),
  // TextColor::White, RenderText_Centered); RenderText(std::to_string((game_->GetPlayer().multifire_status)),
  // GetWindowCenter() - Vector2f(0, 60), TextColor::White, RenderText_Centered);
  ctx_.dt = dt;

  if (game_->GetZone() == Zone::Devastation) {
    if (game_->GetPlayer().name == "FrogBot") {
      if (game_->GetMapFile() != "#frog.lvl") {
        if (GetTime().TimedActionDelay("arenachange", 200)) {
          game_->SendChatMessage("?go #Frog");
        }
        return;
      }
    }

    influence_map_.Decay(dt);
    g_RenderState.RenderDebugText("InfluenceMapDecay: %llu", timer.GetElapsedTime());
    influence_map_.Update(*game_, ctx_.blackboard.ValueOr<std::vector<Player>>("EnemyList", std::vector<Player>()));
    g_RenderState.RenderDebugText("InfluenceMapUpdate: %llu", timer.GetElapsedTime());
  }

#if !DEBUG_NO_BEHAVIOR
  behavior_->Update(ctx_);

  g_RenderState.RenderDebugText("Behavior: %llu", timer.GetElapsedTime());
#endif

  //#if 0 // Test wall avoidance. This should be moved somewhere in the behavior tree
  std::vector<Vector2f> path = ctx_.blackboard.ValueOr<std::vector<Vector2f>>("Path", std::vector<Vector2f>());
  constexpr float kNearbyTurn = 20.0f;
  constexpr float kMaxAvoidDistance = 35.0f;

  if (!path.empty() && path[0].DistanceSq(game_->GetPosition()) < kNearbyTurn * kNearbyTurn) {
    steering_.AvoidWalls(kMaxAvoidDistance);
  }
  //#endif
  steering_.Steer(ctx_.blackboard.ValueOr<bool>("SteerBackwards", false));
  ctx_.blackboard.Set<bool>("SteerBackwards", false);

#if DEBUG_RENDER
  // RenderPath(game_->GetPosition(), base_paths_[ctx_.blackboard.GetRegionIndex()]);
#endif

  g_RenderState.RenderDebugText("Steering: %llu", timer.GetElapsedTime());
}

void Bot::Move(const Vector2f& target, float target_distance) {
  const Player& bot_player = game_->GetPlayer();
  float distance = bot_player.position.Distance(target);

  if (distance > target_distance) {
    steering_.Seek(target);
  }

  else if (distance <= target_distance) {
    Vector2f to_target = target - bot_player.position;

    steering_.Seek(target - Normalize(to_target) * target_distance);
  }
}

void Bot::CreateBasePaths(const std::vector<Vector2f>& start_vector, const std::vector<Vector2f>& end_vector,
                          float radius) {
  PerformanceTimer timer;

  for (std::size_t i = 0; i < start_vector.size(); i++) {
    Vector2f position_1 = start_vector[i];
    Vector2f position_2 = end_vector[i];

    float nRadius = 0.8f;  // still fits through 3 tile holes, the pathfinder cant do 4 tile holes, it jumps to 5
    float hRadius = 0.3f;  // fits through single tile holes

    std::vector<Vector2f> holes;
    std::vector<u8> previousId;
#if 0
            for (std::size_t i = 0; i < 2; i++) {
            //while (true) {
                bool holes_found = false;
                //make a path that can fit through single tile holes
                Path hole_path = GetPathfinder().FindPath(game_->GetMap(), std::vector<Vector2f>(), safe_1, safe_2, hRadius);
                hole_path = GetPathfinder().SmoothPath(hole_path, game_->GetMap(), hRadius);
                for (std::size_t i = 0; i < hole_path.size(); i++) {
                    Vector2f current = hole_path[i];
                    //find the areas where a normal ship cant fit and save them
                    if (!game_->GetMap().CanOccupy(current, nRadius)) {
                        holes.push_back(current);
                        previousId.push_back(game_->GetMap().GetTileId(current));
                        //plug the holes and calculate a new path
                        game_->SetTileId(current, 193);
                        holes_found = true;
                    }
                }
                if (holes_found == false) { break; }
            }
            //set Ids back to original value
            for (std::size_t i = 0; i < holes.size(); i++) {
                Vector2f current = holes[i];
                game_->SetTileId(current, previousId[i]);
            }

            base_holes_.push_back(holes);
#endif

    Path base_path = GetPathfinder().FindPath(game_->GetMap(), std::vector<Vector2f>(), position_1, position_2, radius);

    if (base_path.empty()) {
      base_paths_.push_back(base_path);
      continue;
    }

    base_path = GetPathfinder().SmoothPath(base_path, game_->GetMap(), radius);

    std::vector<Vector2f> reduced_path;

    Vector2f current = base_path[0];
    reduced_path.push_back(current);

    for (std::size_t i = 2; i < base_path.size(); i++) {
      if (RadiusRayCastHit(game_->GetMap(), current, base_path[i], radius)) {
        current = base_path[i];
        reduced_path.push_back(base_path[i - 1]);
      } else if (i % 3 == 0 || i == base_path.size() - 1) {
        current = base_path[i];
        reduced_path.push_back(base_path[i]);
      }
    }

    base_paths_.push_back(reduced_path);
  }

  g_RenderState.RenderDebugText("CreateBasePaths: %llu", timer.GetElapsedTime());
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

void Bot::SetZoneVariables() {
  uint16_t pubteam0 = 00;
  uint16_t pubteam1 = 01;

  Vector2f spawn(512, 512);

  uint16_t ship = game_->GetPlayer().ship;
  bool specced = game_->GetPlayer().ship == 8;

  if (game_->GetZone() == Zone::Devastation) {
    std::string name = Lowercase(game_->GetPlayer().name);
    if (name == "lilmarv" && specced) {
      ship = 1;
    }

    std::vector<Vector2f> nodes = {Vector2f(568, 568), Vector2f(454, 568), Vector2f(454, 454), Vector2f(568, 454),
                                   Vector2f(568, 568), Vector2f(454, 454), Vector2f(568, 454), Vector2f(454, 568),
                                   Vector2f(454, 454), Vector2f(568, 568), Vector2f(454, 568), Vector2f(568, 454)};

    ctx_.blackboard.Set<std::vector<Vector2f>>("PatrolNodes", nodes);
  }

  if (game_->GetZone() == Zone::ExtremeGames) {
    if (specced) {
      ship = 2;
    }
  }

  if (game_->GetZone() == Zone::Hyperspace) {
    pubteam0 = 90;
    pubteam1 = 91;

    ctx_.blackboard.Set<std::vector<Vector2f>>("PatrolNodes",
                                               std::vector<Vector2f>({Vector2f(585, 540), Vector2f(400, 570)}));
  }

  ctx_.blackboard.Set<uint16_t>("Freq", 999);
  ctx_.blackboard.Set<uint16_t>("PubTeam0", pubteam0);
  ctx_.blackboard.Set<uint16_t>("PubTeam1", pubteam1);
  ctx_.blackboard.Set<uint16_t>("Ship", ship);
  ctx_.blackboard.Set<Vector2f>("Spawn", spawn);
}

namespace bot {

behavior::ExecuteResult DisconnectNode::Execute(behavior::ExecuteContext& ctx) {
  // check chat for disconected message and terminate continuum
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();

  for (ChatMessage& chat : game.GetChat()) {
    if (chat.type == 0) {
      if (chat.message.compare(0, 9, "WARNING: ") == 0) {
        // exit(5);
      }

      if (game.GetZone() == Zone::ExtremeGames) {
        std::string name = game.GetPlayer().name;

        if (chat.message.compare(0, 4 + name.size(), "[ " + name + " ]") == 0) {
          //  exit(5);
        }
      }
    }
  }

  g_RenderState.RenderDebugText("  DisconnectNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult CommandNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  bool executed = false;

  for (ChatMessage& chat : game.GetChat()) {
    if (ctx.bot->GetCommandSystem().ProcessMessage(*ctx.bot, chat)) {
      executed = true;
    }
  }

  g_RenderState.RenderDebugText("  CommandNode: %llu", timer.GetElapsedTime());
  // Return failure on execute so the behavior will start over next tick with the commands processed completely.
  return executed ? behavior::ExecuteResult::Failure : behavior::ExecuteResult::Success;
}

behavior::ExecuteResult SetShipNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  uint16_t cShip = game.GetPlayer().ship;
  uint16_t dShip = bb.ValueOr<uint16_t>("Ship", 0);

  uint64_t ship_cooldown = 200;
  if (cShip == 8) {
    ship_cooldown = 1000;
  }

  if (cShip != dShip) {
    if (ctx.bot->GetTime().TimedActionDelay("shipchange", ship_cooldown)) {
      game.SetEnergy(100.0f);
      if (!game.SetShip(dShip)) {
        ctx.bot->GetTime().TimedActionDelay("shipchange", 0);
      }
    }

    g_RenderState.RenderDebugText("  SetShipNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  g_RenderState.RenderDebugText("  SetShipNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult SetFreqNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  const char* balancer_msg = "Changing to that team would disrupt the balance between it and your current team.";

  uint16_t freq = bb.ValueOr<uint16_t>("Freq", 999);

  if (freq != 999) {
    if (ctx.bot->GetTime().TimedActionDelay("setfreq", 200)) {
      game.SetEnergy(100.0f);
      game.SetFreq(freq);
    }

    for (ChatMessage& chat : game.GetChat()) {
      if (chat.message == balancer_msg && chat.type == 0) {
        game.SendChatMessage("The zone balancer has prevented me from joining that team.");
        bb.Set<uint16_t>("Freq", 999);
        break;
      }
    }

    if (freq == int(game.GetPlayer().frequency)) {
      bb.Set<uint16_t>("Freq", 999);
    }

    g_RenderState.RenderDebugText("  SetFreqNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }
  g_RenderState.RenderDebugText("  SetFreqNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult ShipCheckNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();

  if (game.GetPlayer().ship == 8) {
    g_RenderState.RenderDebugText("  ShipCheckNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  g_RenderState.RenderDebugText("  ShipCheckNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult SortBaseTeams::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  const Player& bot = game.GetPlayer();

  std::vector<Player> team_list;
  std::vector<Player> enemy_list;
  std::vector<Player> combined_list;

  std::vector<uint16_t> fList(100, 0);

  bool team_in_base = false;
  bool enemy_in_base = false;
  bool last_in_base = true;

  uint16_t pFreq0 = bb.ValueOr<uint16_t>("PubTeam0", 00);
  uint16_t pFreq1 = bb.ValueOr<uint16_t>("PubTeam1", 01);

  // Vector2f current_base = ctx.blackboard.GetBase[ctx.blackboard.GetRegionIndex()];

  // find team sizes and push team mates into a vector
  for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
    const Player& player = game.GetPlayers()[i];

    bool in_center = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, (MapCoord)Vector2f(512, 512));

    if (player.frequency < 100) fList[player.frequency]++;

    if ((player.frequency == pFreq0 || player.frequency == pFreq1) && (player.id != bot.id)) {
      combined_list.push_back(game.GetPlayers()[i]);

      if (player.frequency == bot.frequency) {
        team_list.push_back(game.GetPlayers()[i]);

        if (!in_center && IsValidPosition(player.position)) {
          team_in_base = true;
        }
        if (!in_center && player.active) {
          last_in_base = false;
        }
      } else {
        enemy_list.push_back(game.GetPlayers()[i]);

        if (!in_center && IsValidPosition(player.position)) {
          enemy_in_base = true;
        }
      }
    }
  }
  if (team_list.size() == 0) {
    last_in_base = false;
  }

  bb.Set<std::vector<Player>>("TeamList", team_list);
  bb.Set<std::vector<Player>>("EnemyList", enemy_list);
  bb.Set<std::vector<Player>>("CombinedList", combined_list);

  bb.Set<std::vector<uint16_t>>("FreqList", fList);

  bb.Set<bool>("TeamInBase", team_in_base);
  bb.Set<bool>("EnemyInBase", enemy_in_base);
  bb.Set<bool>("LastInBase", last_in_base);

  g_RenderState.RenderDebugText("  SortBaseTeams: %llu", timer.GetElapsedTime());

  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult RespawnCheckNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();

  if (!game.GetPlayer().active) {
    g_RenderState.RenderDebugText("  RespawnCheckNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  g_RenderState.RenderDebugText("  RespawnCheckNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult FindEnemyInCenterNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  behavior::ExecuteResult result = behavior::ExecuteResult::Failure;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  if (!bb.ValueOr<bool>("InCenter", true)) {
    g_RenderState.RenderDebugText("  FindEnemyInCenterNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  const Player& bot = game.GetPlayer();

  float closest_cost = std::numeric_limits<float>::max();

  const Player* target = nullptr;

  for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
    const Player& player = game.GetPlayers()[i];

    if (!IsValidTarget(*ctx.bot, player)) {
      continue;
    }

    float cost = CalculateCost(ctx, bot, player);

    if (cost < closest_cost) {
      closest_cost = cost;
      target = &game.GetPlayers()[i];
      result = behavior::ExecuteResult::Success;
    }
  }

  const Player* current_target = bb.ValueOr<const Player*>("Target", nullptr);

  if (current_target && IsValidTarget(*ctx.bot, *current_target)) {
    // Calculate the cost to the current target so there's some stickiness
    // between close targets.
    const float kStickiness = 2.5f;
    float cost = CalculateCost(ctx, bot, *current_target);

    if (cost * kStickiness < closest_cost) {
      target = current_target;
    }
  }

  bb.Set<const Player*>("Target", target);
  g_RenderState.RenderDebugText("  FindEnemyInCenterNode: %llu", timer.GetElapsedTime());
  return result;
}

float FindEnemyInCenterNode::CalculateCost(behavior::ExecuteContext& ctx, const Player& bot_player,
                                           const Player& target) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  float dist = bot_player.position.Distance(target.position);

  // How many seconds it takes to rotate 180 degrees
  float rotate_speed = game.GetRotation();

  float move_cost = dist / game.GetMaxSpeed();

  Vector2f direction = Normalize(target.position - bot_player.position);
  float dot = std::abs(bot_player.GetHeading().Dot(direction) - 1.0f) / 2.0f;
  float rotate_cost = std::abs(dot) * rotate_speed;

  return move_cost + rotate_cost;
}

behavior::ExecuteResult FindEnemyInBaseNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  behavior::ExecuteResult result = behavior::ExecuteResult::Failure;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  Path base_path = ctx.bot->GetBasePath();

  int bot_node = (int)FindPathIndex(base_path, game.GetPosition());

  int closest_distance = std::numeric_limits<int>::max();

  const Player* target = nullptr;

  for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
    const Player& player = game.GetPlayers()[i];

    if (!IsValidTarget(*ctx.bot, player)) {
      continue;
    }

    int player_node = (int)FindPathIndex(base_path, player.position);

    int distance = std::abs(bot_node - player_node);

    if (distance < closest_distance) {
      closest_distance = distance;
      target = &game.GetPlayers()[i];
      result = behavior::ExecuteResult::Success;
    }
  }

  bb.Set<const Player*>("Target", target);
  g_RenderState.RenderDebugText("  FindEnemyInBaseNode: %llu", timer.GetElapsedTime());
  return result;
}

behavior::ExecuteResult PathToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  Vector2f bot = game.GetPosition();

  const Player* enemy = bb.ValueOr<const Player*>("Target", nullptr);

  if (!enemy) {
    g_RenderState.RenderDebugText("  PathToEnemyNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  Path path = bb.ValueOr<Path>("Path", Path());
  float radius = game.GetShipSettings().GetRadius();

  path = ctx.bot->GetPathfinder().CreatePath(path, bot, enemy->position, radius);

  bb.Set<Path>("Path", path);
  g_RenderState.RenderDebugText("  PathToEnemyNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult PatrolNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  Vector2f from = game.GetPosition();
  Path path = bb.ValueOr<Path>("Path", Path());
  float radius = game.GetShipSettings().GetRadius();

  std::vector<Vector2f> nodes = bb.ValueOr<std::vector<Vector2f>>("PatrolNodes", std::vector<Vector2f>());
  int index = ctx.blackboard.ValueOr<int>("PatrolIndex", 0);

  if (bb.ValueOr<bool>("InCenter", true)) {
    Vector2f to = nodes.at(index);

    if (game.GetPosition().DistanceSq(to) < 5.0f * 5.0f) {
      index = (index + 1) % nodes.size();
      ctx.blackboard.Set<int>("PatrolIndex", index);
      to = nodes.at(index);
    }

    path = ctx.bot->GetPathfinder().CreatePath(path, from, to, radius);
    bb.Set<Path>("Path", path);

    g_RenderState.RenderDebugText("  PatrolNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  g_RenderState.RenderDebugText("  PatrolNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult TvsTBasePathNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  bool in_center = bb.ValueOr<bool>("InCenter", true);
  bool is_anchor = bb.ValueOr<bool>("IsAnchor", false);
  bool last_in_base = bb.ValueOr<bool>("LastInBase", false);

  if (in_center) {
    g_RenderState.RenderDebugText("  TvsTBasePathNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  const Player* enemy = bb.ValueOr<const Player*>("Target", nullptr);

  if (!enemy) {
    g_RenderState.RenderDebugText("  TvsTBasePathNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  Vector2f position = game.GetPosition();
  float radius = game.GetShipSettings().GetRadius();

  Path base_path = ctx.bot->GetBasePath();
  Path path = bb.ValueOr<Path>("Path", Path());

  Vector2f desired_position;

  std::size_t bot_node = FindPathIndex(base_path, position);
  std::size_t enemy_node = FindPathIndex(base_path, enemy->position);

  if (RadiusRayCastHit(game.GetMap(), enemy->position, base_path[enemy_node],
                       game.GetSettings().ShipSettings[enemy->ship].GetRadius())) {
  
  desired_position = enemy->position;
  }
  else if (bot_node > enemy_node) {
    desired_position =
        LastLOSNode(game.GetMap(), bot_node, true, base_path, game.GetSettings().ShipSettings[enemy->ship].GetRadius());
  } else {
    desired_position = LastLOSNode(game.GetMap(), bot_node, false, base_path,
                                   game.GetSettings().ShipSettings[enemy->ship].GetRadius());
  }


  if (is_anchor || last_in_base) {
    float energy_pct = ((float)game.GetPlayer().energy / game.GetMaxEnergy());
    float enemy_speed = 1.0f;
    float pathlength = 0.0f;

    Vector2f ref;
    if (bot_node > enemy_node) {
      ref = LastLOSNode(game.GetMap(), enemy_node, false, base_path,
                        game.GetSettings().ShipSettings[enemy->ship].GetRadius());
    } else {
      ref = LastLOSNode(game.GetMap(), enemy_node, true, base_path,
                        game.GetSettings().ShipSettings[enemy->ship].GetRadius());
    }

    Vector2f to_bot = ref - enemy->position;
    enemy_speed = enemy->velocity * Normalize(to_bot);

    float desired = (30.0f + (enemy_speed + (6.0f / (energy_pct + 0.0001f))));

    pathlength = PathLength(ctx.bot->GetBasePath(), position, enemy->position);

    if (pathlength < desired || bb.ValueOr<bool>("TargetInSight", false) || AvoidInfluence(ctx)) {
      if (bot_node > enemy_node) {
        desired_position = LastLOSNode(game.GetMap(), bot_node, false, base_path,
                                       game.GetSettings().ShipSettings[enemy->ship].GetRadius());
      } else {
        desired_position = LastLOSNode(game.GetMap(), bot_node, true, base_path,
                                       game.GetSettings().ShipSettings[enemy->ship].GetRadius());
      }

      // path = ctx.bot->GetPathfinder().CreatePath(path, position, bb.ValueOr<Vector2f>("TeamSafe", Vector2f()),
      // radius);
      bb.Set<bool>("SteerBackwards", true);
    }
  }

  path = ctx.bot->GetPathfinder().CreatePath(path, position, desired_position, radius);
  bb.Set<Path>("Path", path);

  g_RenderState.RenderDebugText("  TvsTBasePathNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

bool TvsTBasePathNode::AvoidInfluence(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;
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

        if (value < best_value && game.GetMap().CanOccupy(check + offsets[i], radius)) {
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
  auto& bb = ctx.blackboard;

  Path path = bb.ValueOr<Path>("Path", Path());

  size_t path_size = path.size();

  if (path.empty()) {
    g_RenderState.RenderDebugText("  FollowPathNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
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

  while (path.size() > 1 && CanMoveBetween(game, game.GetPosition(), path.at(1))) {
    path.erase(path.begin());
    current = path.front();
  }

  if (path.size() == 1 && path.front().DistanceSq(game.GetPosition()) < 2.0f * 2.0f) {
    path.clear();
  }

  if (path.size() != path_size) {
    bb.Set<Path>("Path", path);
  }

  ctx.bot->Move(current, 0.0f);
  g_RenderState.RenderDebugText("  FollowPathNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

bool FollowPathNode::CanMoveBetween(GameProxy& game, Vector2f from, Vector2f to) {
  Vector2f trajectory = to - from;
  Vector2f direction = Normalize(trajectory);
  Vector2f side = Perpendicular(direction);

  float distance = from.Distance(to);
  float radius = game.GetShipSettings().GetRadius();

  CastResult center = RayCast(game.GetMap(), from, direction, distance);
  CastResult side1 = RayCast(game.GetMap(), from + side * radius, direction, distance);
  CastResult side2 = RayCast(game.GetMap(), from - side * radius, direction, distance);

  return !center.hit && !side1.hit && !side2.hit;
}

/*monkey> no i dont have a list of the types
monkey> i think it's some kind of combined thing with weapon flags in it
monkey> im pretty sure type & 0x01 is whether or not it's bouncing
monkey> and the 0x8000 is the alternative bit for things like mines and multifire
monkey> i was using type & 0x8F00 to see if it was a mine and it worked decently well
monkey> 0x0F00 worked ok for seeing if its a bomb
monkey> and 0x00F0 for bullet, but i don't think it's exact*/

/*
0x0000 = nothing
0x1000 & 0x0F00 & 0x1100 & 0x1800 & 0x1F00 & 0x0001 = mine or bomb
0x0800 = emp mine or bomb
0x8000 & 0x8100 = mine or multifire
0x8001 = multi mine bomb
0xF000 & 0x8800 & 0x8F00 & 0xF100 & 0xF800 & 0xFF00 = mine multi bomb
0x000F & 0x00F0 = any weapon

stopped at 0x0F01
*/

behavior::ExecuteResult MineSweeperNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  for (Weapon* weapon : game.GetWeapons()) {
    const Player* weapon_player = game.GetPlayerById(weapon->GetPlayerId());

    if (weapon_player == nullptr) continue;
    if (weapon_player->frequency == game.GetPlayer().frequency) continue;
    if (weapon->IsMine()) {
      if (weapon->GetPosition().Distance(game.GetPosition()) < 8.0f && game.GetPlayer().repels > 0) {
        game.Repel(ctx.bot->GetKeys());

        g_RenderState.RenderDebugText("  MineSweeperNode: %llu", timer.GetElapsedTime());
        return behavior::ExecuteResult::Success;
      }
    }
  }

  g_RenderState.RenderDebugText("  MineSweeperNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult InLineOfSightNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  const Player* target = bb.ValueOr<const Player*>("Target", nullptr);

  if (target) {
    if (!RadiusRayCastHit(game.GetMap(), game.GetPosition(), target->position, game.GetShipSettings().GetRadius())) {
      g_RenderState.RenderDebugText("  InLineOfSightNode: %llu", timer.GetElapsedTime());
      return behavior::ExecuteResult::Success;
    }
  }

  g_RenderState.RenderDebugText("  InLineOfSightNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult IsAnchorNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;
  auto& bb = ctx.blackboard;

  if (bb.ValueOr<bool>("IsAnchor", false) && !bb.ValueOr<bool>("InCenter", true)) {
    g_RenderState.RenderDebugText("  IsAnchorNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Success;
  }

  g_RenderState.RenderDebugText("  IsAnchorNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

behavior::ExecuteResult BouncingShotNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  const Player* target = bb.ValueOr<const Player*>("Target", nullptr);
  if (!target) {
    g_RenderState.RenderDebugText("  BouncingShotNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  float energy_pct = (float)game.GetPlayer().energy / game.GetMaxEnergy() * 100.0f;

  if (bb.ValueOr<bool>("IsAnchor", false) && energy_pct < 50.0f) {
    g_RenderState.RenderDebugText("  BouncingShotNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }

  float target_radius = game.GetSettings().ShipSettings[target->ship].GetRadius();
  bool shoot = false;
  bool bomb_hit = false;
  Vector2f wall_pos;

  if (BounceShot(game, target->position, target->velocity, target_radius, game.GetPlayer().GetHeading(), &bomb_hit,
                 &wall_pos)) {
    if (game.GetMap().GetTileId(game.GetPosition()) != marvin::kSafeTileId) {
      if (bomb_hit) {
        ctx.bot->GetKeys().Press(VK_TAB);
      } else {
        ctx.bot->GetKeys().Press(VK_CONTROL);
      }
    }
  }

  g_RenderState.RenderDebugText("  BouncingShotNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

behavior::ExecuteResult ShootEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  const auto target_player = bb.ValueOr<const Player*>("Target", nullptr);
  if (!target_player) {
    g_RenderState.RenderDebugText("  ShootEnemyNode: %llu", timer.GetElapsedTime());
    return behavior::ExecuteResult::Failure;
  }
  const Player& target = *target_player;
  const Player& bot = game.GetPlayer();

  float target_radius = game.GetSettings().ShipSettings[target.ship].GetRadius();
  float radius = game.GetShipSettings().GetRadius();
  Vector2f side = Perpendicular(bot.GetHeading());

  float proj_speed = game.GetSettings().ShipSettings[bot.ship].BulletSpeed / 10.0f / 16.0f;
  float alive_time = ((float)game.GetSettings().BulletAliveTime / 100.0f);

  // float gSpeed = game.GetSettings().ShipSettings[bot.ship].BulletSpeed / 10.0f / 16.0f;
  // float bSpeed = game.GetSettings().ShipSettings[bot.ship].BombSpeed / 10.0f / 16.0f;

  Vector2f solution = target.position;
  bb.Set<Vector2f>("Solution", solution);

  float radius_multiplier = 1.2f;

  int weapon_key = VK_CONTROL;

  bool gun_hit = false;
  bool bomb_hit = false;

  std::vector<Vector2f> positions;

  if (game.GetShipSettings().DoubleBarrel == 1) {
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
          ((((float)game.GetSettings().BombExplodePixels * (float)(game.GetShipSettings().MaxBombs & 3)) / 16.0f) +
           target_radius)) {
        continue;
      }
      weapon_key = VK_TAB;
      proj_speed = game.GetSettings().ShipSettings[bot.ship].BombSpeed / 10.0f / 16.0f;
      alive_time = ((float)game.GetSettings().BombAliveTime / 100.0f);
      radius_multiplier = 1.0f;
    }

    if (CalculateShot(bot.position, target.position, bot.velocity, target.velocity, proj_speed, &solution)) {
      if (!onBomb) {
        bb.Set<Vector2f>("Solution", solution);
      }

      if (CanShoot(game, game.GetPosition(), solution, proj_speed, alive_time)) {
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
        Vector2f box_extent(nearby_radius * 1.2f, nearby_radius * 1.2f);
        float dist;
        Vector2f norm;
        Vector2f heading = bot.GetHeading();

        if (game.GetPlayer().multifire_status) {
          if (i == iSize - 2) {
            heading = game.GetPlayer().MultiFireDirection(game.GetShipSettings().MultiFireAngle, true);
          } else if (i == iSize - 3) {
            heading = game.GetPlayer().MultiFireDirection(game.GetShipSettings().MultiFireAngle, false);
          }
        }

        if (RayBoxIntersect(bot.position, heading, bBox_min, box_extent, &dist, &norm)) {
          ctx.bot->GetKeys().Press(weapon_key);
          g_RenderState.RenderDebugText("  ShootEnemyNode: %llu", timer.GetElapsedTime());
          return behavior::ExecuteResult::Success;
        }
      }
    }
  }
  g_RenderState.RenderDebugText("  ShootEnemyNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Failure;
}

bool ShootEnemyNode::PreferGuns(behavior::ExecuteContext& ctx) {
  auto& game = ctx.bot->GetGame();

  bool use_guns = false;

  float bomb_dps = ((float)game.GetShipSettings().BombFireDelay / 100.0f) * (float)game.GetSettings().BombDamageLevel;

  float gun_damage = ((float)game.GetSettings().BulletDamageLevel +
                      ((float)(game.GetShipSettings().MaxGuns & 3) * ((float)game.GetSettings().BulletDamageUpgrade)));
  float gun_dps = ((float)game.GetShipSettings().BulletFireDelay / 100.0f) * gun_damage;

  if (game.GetShipSettings().DoubleBarrel == 1 || bomb_dps < gun_dps) {
    use_guns = true;
  }
  return use_guns;
}

behavior::ExecuteResult MoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
  PerformanceTimer timer;

  auto& game = ctx.bot->GetGame();
  auto& bb = ctx.blackboard;

  Vector2f position = bb.ValueOr<Vector2f>("Solution", Vector2f());

  bool in_safe = game.GetMap().GetTileId(game.GetPlayer().position) == marvin::kSafeTileId;

  float hover_distance = 0.0f;

  if (in_safe)
    hover_distance = 0.0f;
  else {
    hover_distance = 10.0f;
  }

  ctx.bot->Move(position, hover_distance);

  ctx.bot->GetSteering().Face(position);

  g_RenderState.RenderDebugText("  MoveToEnemyNode: %llu", timer.GetElapsedTime());
  return behavior::ExecuteResult::Success;
}

}  // namespace bot
}  // namespace marvin
