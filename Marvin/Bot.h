#pragma once

#include <fstream>
#include <memory>

#include "CommandSystem.h"
#include "InfluenceMap.h"
#include "KeyController.h"
#include "RayCaster.h"
#include "RegionRegistry.h"
#include "Steering.h"
#include "Time.h"
#include "behavior/BehaviorEngine.h"
#include "common.h"
#include "path/Pathfinder.h"
#include "platform/ContinuumGameProxy.h"

namespace marvin {

using Path = std::vector<Vector2f>;

extern std::ofstream debug_log;

class GameProxy;
struct Player;

class Bot {
 public:
  Bot(std::shared_ptr<GameProxy> game);

  void LoadBotConstuctor();

  void Update(float dt);

  KeyController& GetKeys() { return keys_; }
  GameProxy& GetGame() { return *game_; }
  Time& GetTime() { return time_; }

  void Move(const Vector2f& target, float target_distance);

  path::Pathfinder& GetPathfinder() { return *pathfinder_; }
  const RegionRegistry& GetRegions() const { return *regions_; }
  SteeringBehavior& GetSteering() { return steering_; }
  InfluenceMap& GetInfluenceMap() { return influence_map_; }
  CommandSystem& GetCommandSystem() { return command_system_; }

  const std::vector<Vector2f>& GetBasePath() {
    return base_paths_[ctx_.blackboard.ValueOr<std::size_t>("BaseIndex", 0)];
  }

  bool MaxEnergyCheck();

  void CreateBasePaths(const std::vector<Vector2f>& start_vector, const std::vector<Vector2f>& end_vector,
                       float radius);

 private:
  void SetZoneVariables();

  std::vector<std::vector<Vector2f>> base_paths_;
  std::vector<std::vector<Vector2f>> base_holes_;

  std::shared_ptr<GameProxy> game_;
  std::unique_ptr<path::Pathfinder> pathfinder_;
  std::unique_ptr<RegionRegistry> regions_;
  behavior::ExecuteContext ctx_;
  SteeringBehavior steering_;
  InfluenceMap influence_map_;
  CommandSystem command_system_;

  std::unique_ptr<behavior::BehaviorEngine> behavior_;

  // TODO: Action-key map would be more versatile
  KeyController keys_;
  Time time_;
};

namespace bot {

class DisconnectNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class CommandNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class SetShipNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class SetFreqNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class ShipCheckNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class RespawnCheckNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class SortBaseTeams : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class FindEnemyInCenterNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  float CalculateCost(behavior::ExecuteContext& ctx, const Player& bot_player, const Player& target);
};

class FindEnemyInBaseNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class PathToEnemyNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class PatrolNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class TvsTBasePathNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  bool AvoidInfluence(behavior::ExecuteContext& ctx);
};

class FollowPathNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  bool CanMoveBetween(GameProxy& game, Vector2f from, Vector2f to);
};

class MineSweeperNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class InLineOfSightNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class BouncingShotNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class IsAnchorNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class ShootEnemyNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  bool PreferGuns(behavior::ExecuteContext& ctx);
};

class MoveToEnemyNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  // bool IsAimingAt(GameProxy& game, const Player& shooter, const Player& target, Vector2f* dodge);
};

}  // namespace bot

class BehaviorBuilder {
 public:
  std::unique_ptr<behavior::BehaviorEngine> Build(Bot& bot) {
    BuildCommonNodes();
    CreateBehavior(bot);
    PushCommonNodes();

    return std::move(engine_);
  }

 protected:
  virtual void CreateBehavior(Bot& bot) = 0;

  void BuildCommonNodes() {
    engine_ = std::make_unique<behavior::BehaviorEngine>();

    follow_path_ = std::make_unique<bot::FollowPathNode>();
    shoot_enemy_ = std::make_unique<bot::ShootEnemyNode>();
    mine_sweeper_ = std::make_unique<bot::MineSweeperNode>();
    target_in_los_ = std::make_unique<bot::InLineOfSightNode>();
    path_to_enemy_ = std::make_unique<bot::PathToEnemyNode>();
    find_enemy_in_center_ = std::make_unique<bot::FindEnemyInCenterNode>();
    set_freq_ = std::make_unique<bot::SetFreqNode>();
    set_ship_ = std::make_unique<bot::SetShipNode>();
    commands_ = std::make_unique<bot::CommandNode>();
    ship_check_ = std::make_unique<bot::ShipCheckNode>();
    respawn_check_ = std::make_unique<bot::RespawnCheckNode>();
    disconnect_ = std::make_unique<bot::DisconnectNode>();
  }

  void PushCommonNodes() {
    engine_->PushNode(std::move(follow_path_));
    engine_->PushNode(std::move(shoot_enemy_));
    engine_->PushNode(std::move(mine_sweeper_));
    engine_->PushNode(std::move(target_in_los_));
    engine_->PushNode(std::move(path_to_enemy_));
    engine_->PushNode(std::move(find_enemy_in_center_));
    engine_->PushNode(std::move(set_freq_));
    engine_->PushNode(std::move(set_ship_));
    engine_->PushNode(std::move(commands_));
    engine_->PushNode(std::move(ship_check_));
    engine_->PushNode(std::move(respawn_check_));
    engine_->PushNode(std::move(disconnect_));
  }

  std::unique_ptr<behavior::BehaviorEngine> engine_;

  std::unique_ptr<bot::FollowPathNode> follow_path_;
  std::unique_ptr<bot::ShootEnemyNode> shoot_enemy_;
  std::unique_ptr<bot::MineSweeperNode> mine_sweeper_;
  std::unique_ptr<bot::InLineOfSightNode> target_in_los_;
  std::unique_ptr<bot::PathToEnemyNode> path_to_enemy_;
  std::unique_ptr<bot::FindEnemyInCenterNode> find_enemy_in_center_;
  std::unique_ptr<bot::SetFreqNode> set_freq_;
  std::unique_ptr<bot::SetShipNode> set_ship_;
  std::unique_ptr<bot::CommandNode> commands_;
  std::unique_ptr<bot::ShipCheckNode> ship_check_;
  std::unique_ptr<bot::RespawnCheckNode> respawn_check_;
  std::unique_ptr<bot::DisconnectNode> disconnect_;
};

}  // namespace marvin
