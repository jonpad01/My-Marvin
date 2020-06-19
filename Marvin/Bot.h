#pragma once

#include <fstream>
#include <memory>

#include "KeyController.h"
#include "RayCaster.h"
#include "RegionRegistry.h"
#include "Steering.h"
#include "behavior/BehaviorEngine.h"
#include "path/Pathfinder.h"




namespace marvin {

    using Path = std::vector<Vector2f>;

extern std::ofstream debug_log;
struct Area {
    bool in_base;
    bool in_tunnel;
    bool in_center;
    bool in_1;
    bool in_2;
    bool in_3;
    bool in_4;
    bool in_5;
    bool in_6;
    bool in_7;
    bool in_8;
    bool connected;
    bool in_diff;
    std::vector<bool> in;
    bool in_deva_center;
};
struct Players {
    Player enemy;
    Player team;
    int flaggers;
    int team_lancs;
    int base_duelers;
    int freq_0s;
    int freq_1s;
    std::vector< Player > duel_team;
    bool in_base;
};

class GameProxy;
struct Player;

class Bot {
public:
    Bot(std::unique_ptr<GameProxy> game);
    void Hyperspace();
    void Devastation();

  void Update(float dt);

  KeyController& GetKeys() { return keys_; }
  GameProxy& GetGame() { return *game_; }

  void Move(const Vector2f& target, float target_distance);
  path::Pathfinder& GetPathfinder() { return *pathfinder_; }

  const RegionRegistry& GetRegions() const { return *regions_; }

  SteeringBehavior& GetSteering() { return steering_; }

  uint64_t GetTime() const;

  void AddBehaviorNode(std::unique_ptr<behavior::BehaviorNode> node) {
      behavior_nodes_.push_back(std::move(node));
  }

  void SetBehavior(behavior::BehaviorNode* behavior) {
      behavior_ = std::make_unique<behavior::BehaviorEngine>(behavior);
  }


  Area InArea(Vector2f position);

  int BaseSelector();

  Players FindFlaggers();
  Players FindDuelers();
  bool CheckStatus();
  int FindOpenFreq();

 private:
  void Steer();

  std::unique_ptr<GameProxy> game_;
  std::unique_ptr<path::Pathfinder> pathfinder_;
  std::unique_ptr<behavior::BehaviorEngine> behavior_;
  std::unique_ptr<RegionRegistry> regions_;
  behavior::ExecuteContext behavior_ctx_;
  int ship_;
  int page_count;
  int repel_trigger;
  uint64_t last_attach;
  uint64_t last_page;
  uint64_t last_base_change_;
  uint64_t last_ship_change_;
  uint16_t last_target;
  SteeringBehavior steering_;

  std::vector<std::unique_ptr<behavior::BehaviorNode>> behavior_nodes_;

  // TODO: Action-key map would be more versatile
  KeyController keys_;
};

bool InRect(marvin::Vector2f pos, marvin::Vector2f min_rect,
    marvin::Vector2f max_rect);

bool IsValidPosition(marvin::Vector2f position);

bool CanShoot(const marvin::Map& map, const marvin::Player& bot_player,
    const marvin::Player& target);

marvin::Vector2f CalculateShot(const marvin::Vector2f& pShooter,
    const marvin::Vector2f& pTarget,
    const marvin::Vector2f& vShooter,
    const marvin::Vector2f& vTarget,
    float sProjectile);

class PathingNode : public behavior::BehaviorNode {
public:

    virtual behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx) override = 0;

    float PathLength(behavior::ExecuteContext& ctx, Vector2f from, Vector2f to);
        
protected:
    Path CreatePath(behavior::ExecuteContext& ctx, const std::string& pathname, Vector2f from, Vector2f to, float radius);
};

}  // namespace marvin
