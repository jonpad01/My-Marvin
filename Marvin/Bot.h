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
    bool in_base, in_tunnel, in_center, in_gs_spawn, in_gs_free_mode, in_gs_warp;
    bool in_1, in_2, in_3, in_4, in_5, in_6, in_7, in_8;
    bool connected, in_diff;
    std::vector<bool> in;
};
struct Flaggers {
    Player enemy_anchor, team_anchor;
    int flaggers, team_lancs, team_spiders;
    
};
struct Duelers {
    int freq_0s, freq_1s;
    std::vector< Player > team;
    bool team_in_base, enemy_in_base;
};

class GameProxy;
struct Player;

class Bot {
public:
    Bot(std::unique_ptr<GameProxy> game);
    void Hyperspace();
    void Devastation();
    void ExtremeGames();
    void GalaxySports();

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

  Flaggers FindFlaggers();
  Duelers FindDuelers();
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
  uint64_t last_ship_change_;
  uint64_t last_base_change_;
  SteeringBehavior steering_;

  std::vector<std::unique_ptr<behavior::BehaviorNode>> behavior_nodes_;

  // TODO: Action-key map would be more versatile
  KeyController keys_;
};

bool WallShot(behavior::ExecuteContext& ctx, const marvin::Player& bot_player);

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
