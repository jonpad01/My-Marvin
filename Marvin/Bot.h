#pragma once

#include <fstream>
#include <memory>

#include "commands/CommandSystem.h"
#include "InfluenceMap.h"
#include "KeyController.h"
#include "RayCaster.h"
#include "RegionRegistry.h"
#include "Steering.h"
#include "Time.h"
#include "behavior/BehaviorEngine.h"
#include "platform/ContinuumGameProxy.h"
#include "Shooter.h"
#include "BasePaths.h"
#include "TeamGoals.h"
#include "blackboard/Blackboard.h"


namespace marvin {
class GameProxy;
struct Player;

const std::vector<std::string> kBotNames = {"LilMarv", "MadMarv", "MarvMaster", "Baked Cake", "Marv1",
                                            "Marv2",   "Marv3",   "Marv4",      "Marv5",      "Marv6",
                                            "Marv7",   "Marv8",   "FrogBot",    "X-Marv1",    "X-Marv2", 
                                            "X-Marv3", "X-Marv4", "X-Marv5",    "X-Notify2"};


class Bot {
 public:
  Bot(std::shared_ptr<GameProxy> game);

  void Load();
  void Update(bool reload, float dt);

  KeyController& GetKeys() { return keys_; }
  GameProxy& GetGame() { return *game_; }
  Time& GetTime() { return time_; }
  Blackboard& GetBlackboard() { return *blackboard_; }
  behavior::ExecuteContext& GetExecuteContext() { return ctx_; }
  path::Pathfinder& GetPathfinder() { return *pathfinder_; }
  RegionRegistry& GetRegions() { return *regions_; }
  Shooter& GetShooter() { return shooter_; }
  SteeringBehavior& GetSteering() { return steering_; }
  InfluenceMap& GetInfluenceMap() { return *influence_map_; }
  CommandSystem& GetCommandSystem() { return *command_system_; }
  TeamGoalCreator& GetTeamGoals() { return *goals_; }
  BasePaths& GetBasePaths() { return *base_paths_; }
  uint64_t GetLastLoadTimeStamp() { return last_load_timestamp_; }
  float GetUpdateInterval() { return update_interval_; }
  void SetUpdateInterval(float value) { update_interval_ = value; }

  const std::vector<Vector2f>& GetBasePath() {
    //return base_paths_->GetBasePath(ctx_.blackboard.ValueOr<std::size_t>(BB::BaseIndex, 0));
    return base_paths_->GetBasePath();
  }

  std::size_t GetTeamSafeIndex(uint16_t freq) {
   // uint16_t low_index_team = ctx_.blackboard.ValueOr<uint16_t>(BB::PubTeam0, 999);
   // uint16_t high_index_team = ctx_.blackboard.ValueOr<uint16_t>(BB::PubTeam1, 999);
    uint16_t low_index_team = blackboard_->GetPubTeam0();
    uint16_t high_index_team = blackboard_->GetPubTeam1();

    if (freq == low_index_team) {
      return 0;
    } else if (freq == high_index_team) {
      //return base_paths_->GetBasePath(ctx_.blackboard.ValueOr<std::size_t>(BB::BaseIndex, 0)).size() - 1;
      return base_paths_->GetBasePath().size() - 1;
    }
    return 0;
  }

  const Vector2f& GetTeamSafePosition(uint16_t freq) {
   // return base_paths_->GetBasePath(ctx_.blackboard.ValueOr<std::size_t>(BB::BaseIndex, 0))[GetTeamSafeIndex(freq)];
    return base_paths_->GetBasePath()[GetTeamSafeIndex(freq)];
  }

  void Move(const Vector2f& target, float target_distance);

  bool MaxEnergyCheck();
  void FindPowerBallGoal();

 private:

  void SelectBehaviorTree();

  float radius_;
  float update_interval_ = 60;

  uint64_t last_load_timestamp_ = 0;
  int load_index = 1;
  u16 xMin = 0;
  u16 xMax = 32;

  Vector2f powerball_goal_;
  Vector2f powerball_goal_path_;
  std::string powerball_arena_;

  std::shared_ptr<GameProxy> game_;
  std::unique_ptr<path::Pathfinder> pathfinder_;
  std::unique_ptr<RegionRegistry> regions_;
  std::unique_ptr<Blackboard> blackboard_;
  behavior::ExecuteContext ctx_;
  SteeringBehavior steering_;
  std::unique_ptr<InfluenceMap> influence_map_;
  std::unique_ptr<CommandSystem> command_system_;
  Shooter shooter_;

  //std::unique_ptr<deva::BaseDuelWarpCoords> warps_;
  std::unique_ptr<TeamGoalCreator> goals_;
  std::unique_ptr<behavior::BehaviorEngine> behavior_;
  std::unique_ptr<BasePaths> base_paths_;

  // TODO: Action-key map would be more versatile
  KeyController keys_;
  Time time_;
};

namespace bot {

class CommandNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class DettachNode : public behavior::BehaviorNode {
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

class SpectatorCheckNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class SpectatorLockNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class SetArenaNode : public behavior::BehaviorNode {
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

class CastWeaponInfluenceNode : public behavior::BehaviorNode {
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

class RusherBasePathNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class AnchorBasePathNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:

  bool AvoidInfluence(behavior::ExecuteContext& ctx);
  const Player* GetEnemy(behavior::ExecuteContext& ctx);
  void CalculateTeamThreat(behavior::ExecuteContext& ctx, const Player* enemy);
};

class FailureNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class SuccessNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class FollowPathNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
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

class NotInCenterNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class InCenterNode : public behavior::BehaviorNode {
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
    dettach_ = std::make_unique<bot::DettachNode>();
    shoot_enemy_ = std::make_unique<bot::ShootEnemyNode>();
    mine_sweeper_ = std::make_unique<bot::MineSweeperNode>();
    target_in_los_ = std::make_unique<bot::InLineOfSightNode>();
    path_to_enemy_ = std::make_unique<bot::PathToEnemyNode>();
    find_enemy_in_center_ = std::make_unique<bot::FindEnemyInCenterNode>();
   // set_freq_ = std::make_unique<bot::SetFreqNode>();
    //set_ship_ = std::make_unique<bot::SetShipNode>();
    //set_arena_ = std::make_unique<bot::SetArenaNode>();
    commands_ = std::make_unique<bot::CommandNode>();
    spectator_check_ = std::make_unique<bot::SpectatorCheckNode>();
    respawn_check_ = std::make_unique<bot::RespawnCheckNode>();
  }

  void PushCommonNodes() {
    engine_->PushNode(std::move(follow_path_));
    engine_->PushNode(std::move(dettach_));
    engine_->PushNode(std::move(shoot_enemy_));
    engine_->PushNode(std::move(mine_sweeper_));
    engine_->PushNode(std::move(target_in_los_));
    engine_->PushNode(std::move(path_to_enemy_));
    engine_->PushNode(std::move(find_enemy_in_center_));
   // engine_->PushNode(std::move(set_freq_));
   // engine_->PushNode(std::move(set_ship_));
    //engine_->PushNode(std::move(set_arena_));
    engine_->PushNode(std::move(commands_));
    engine_->PushNode(std::move(spectator_check_));
    engine_->PushNode(std::move(respawn_check_));
  }

  std::unique_ptr<behavior::BehaviorEngine> engine_;

  std::unique_ptr<bot::FollowPathNode> follow_path_;
  std::unique_ptr<bot::DettachNode> dettach_;
  std::unique_ptr<bot::ShootEnemyNode> shoot_enemy_;
  std::unique_ptr<bot::MineSweeperNode> mine_sweeper_;
  std::unique_ptr<bot::InLineOfSightNode> target_in_los_;
  std::unique_ptr<bot::PathToEnemyNode> path_to_enemy_;
  std::unique_ptr<bot::FindEnemyInCenterNode> find_enemy_in_center_;
  //std::unique_ptr<bot::SetFreqNode> set_freq_;
  //std::unique_ptr<bot::SetShipNode> set_ship_;
 // std::unique_ptr<bot::SetArenaNode> set_arena_;
  std::unique_ptr<bot::CommandNode> commands_;
  std::unique_ptr<bot::SpectatorCheckNode> spectator_check_;
  std::unique_ptr<bot::RespawnCheckNode> respawn_check_;
};

}  // namespace marvin
