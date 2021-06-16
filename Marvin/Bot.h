#pragma once

#include <fstream>
#include <memory>

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

  void AddBehaviorNode(std::unique_ptr<behavior::BehaviorNode> node) { behavior_nodes_.push_back(std::move(node)); }
  void SetBehavior(behavior::BehaviorNode* behavior) {
    behavior_ = std::make_unique<behavior::BehaviorEngine>(behavior);
  }

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

  std::unique_ptr<behavior::SelectorNode> move_method_selector_;
  std::unique_ptr<behavior::SelectorNode> los_weapon_selector_;
  std::unique_ptr<behavior::ParallelNode> parallel_shoot_enemy_;
  std::unique_ptr<behavior::ParallelNode> anchor_los_parallel_;
  std::unique_ptr<behavior::SequenceNode> anchor_los_sequence_;
  std::unique_ptr<behavior::SelectorNode> anchor_los_selector_;
  std::unique_ptr<behavior::SequenceNode> los_shoot_conditional_;
  std::unique_ptr<behavior::ParallelNode> bounce_path_parallel_;
  std::unique_ptr<behavior::SequenceNode> path_to_enemy_sequence_;
  std::unique_ptr<behavior::SequenceNode> TvsT_base_path_sequence_;
  std::unique_ptr<behavior::SelectorNode> enemy_path_logic_selector_;
  std::unique_ptr<behavior::SelectorNode> path_or_shoot_selector_;
  std::unique_ptr<behavior::SequenceNode> find_enemy_in_base_sequence_;
  std::unique_ptr<behavior::SequenceNode> find_enemy_in_center_sequence_;
  std::unique_ptr<behavior::SelectorNode> find_enemy_selector_;
  std::unique_ptr<behavior::SequenceNode> patrol_base_sequence_;
  std::unique_ptr<behavior::SequenceNode> patrol_center_sequence_;
  std::unique_ptr<behavior::SelectorNode> patrol_selector_;
  std::unique_ptr<behavior::SelectorNode> action_selector_;
  std::unique_ptr<behavior::SequenceNode> root_sequence_;

  std::unique_ptr<behavior::BehaviorEngine> behavior_;
  std::vector<std::unique_ptr<behavior::BehaviorNode>> behavior_nodes_;

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
}  // namespace marvin
