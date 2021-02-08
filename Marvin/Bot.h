#pragma once

#include <fstream>
#include <memory>

#include "KeyController.h"
#include "RayCaster.h"
#include "RegionRegistry.h"
#include "Steering.h"
#include "behavior/BehaviorEngine.h"
#include "path/Pathfinder.h"
#include "platform/ContinuumGameProxy.h"
#include "common.h"



namespace marvin {

    using Path = std::vector<Vector2f>;

extern std::ofstream debug_log;

class GameProxy;
struct Player;

class Bot {
public:
 
    Bot(std::unique_ptr<GameProxy> game);

  void Update(float dt);

  KeyController& GetKeys() { return keys_; }
  GameProxy& GetGame() { return *game_; }

  void Move(const Vector2f& target, float target_distance);

  path::Pathfinder& GetPathfinder() { return *pathfinder_; }
  const RegionRegistry& GetRegions() const { return *regions_; }
  SteeringBehavior& GetSteering() { return steering_; }

  void AddBehaviorNode(std::unique_ptr<behavior::BehaviorNode> node) {
      behavior_nodes_.push_back(std::move(node));
  }

  void SetBehavior(behavior::BehaviorNode* behavior) {
      behavior_ = std::make_unique<behavior::BehaviorEngine>(behavior);
  }

 private:
  void Steer();

  std::unique_ptr<GameProxy> game_;
  std::unique_ptr<path::Pathfinder> pathfinder_;
  
  std::unique_ptr<RegionRegistry> regions_;

  behavior::ExecuteContext ctx_;
 
  int ship_;
  uint64_t last_ship_change_;
 
  SteeringBehavior steering_;

  std::unique_ptr<behavior::BehaviorEngine> behavior_;
  std::vector<std::unique_ptr<behavior::BehaviorNode>> behavior_nodes_;

  // TODO: Action-key map would be more versatile
  KeyController keys_;
  Common common_;
};


}  // namespace marvin