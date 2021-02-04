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



namespace marvin {

    using Path = std::vector<Vector2f>;

extern std::ofstream debug_log;
#if 1
struct Flaggers {
    const Player* enemy_anchor; const Player* team_anchor;
    int flaggers, team_lancs, team_spiders;
    
};
#endif
class GameProxy;
struct Player;

class Bot {
public:
 
    Bot(std::shared_ptr<GameProxy> game, std::shared_ptr<marvin::GameProxy> game2);

    void Hyperspace();
    void ExtremeGames();
    void GalaxySports();
    void HockeyZone();
    void PowerBall();

  void Update(float dt);

  KeyController& GetKeys() { return keys_; }
  GameProxy& GetGame() { return *game_; }

  void Move(const Vector2f& target, float target_distance);
  path::Pathfinder& GetPathfinder() { return *pathfinder_; }

  const RegionRegistry& GetRegions() const { return *regions_; }

  SteeringBehavior& GetSteering() { return steering_; }

  uint64_t GetTime() const;
  Path CreatePath(behavior::ExecuteContext& ctx, const std::string& pathname, Vector2f from, Vector2f to, float radius);

  void AddBehaviorNode(std::unique_ptr<behavior::BehaviorNode> node) {
      behavior_nodes_.push_back(std::move(node));
  }

  void SetBehavior(behavior::BehaviorNode* behavior) {
      behavior_ = std::make_unique<behavior::BehaviorEngine>(behavior);
  }

  

  int BaseSelector();

  Flaggers FindFlaggers();
  
  bool CheckStatus();
  int FindOpenFreq();

  
  //HS specific members
  Path GetBasePaths(std::size_t base_number) { return hs_base_paths_[base_number]; }
  bool InHSBase(Vector2f position);
  std::vector<bool> InHSBaseNumber(Vector2f position);

  //Devastation specific members
  std::vector<uint16_t> GetFreqList() { return freq_list; }

  bool InCenter() { return in_center; }


  //PowerBall members
  Vector2f GetPowerBallGoal() { return powerball_goal_; }
  Vector2f GetPowerBallGoalPath() { return powerball_goal_path_; }


  float PathLength(Vector2f point_1, Vector2f point_2);

  bool CanShootGun(const Map& map, const Player& bot_player, const Player& target);
  bool CanShootBomb(const Map& map, const Player& bot_player, const Player& target);

  void LookForWallShot(Vector2f target_pos, Vector2f target_vel, float proj_speed, int alive_time, uint8_t bounces);
  bool CalculateWallShot(Vector2f target_pos, Vector2f target_vel, Vector2f desired_direction, float proj_speed, int alive_time, uint8_t bounces);
  Vector2f GetWallShot() { return wall_shot; }
  bool HasWallShot() { return has_wall_shot; }

 private:
  void Steer();

  void CreateHSBasePaths();
  void CheckHSBaseRegions(Vector2f position);

  void CheckCenterRegion();
  
  std::size_t FindClosestPathNode(Vector2f position);


  void FindPowerBallGoal();

  Vector2f powerball_goal_;
  Vector2f powerball_goal_path_;

  bool in_center;
  std::vector<uint16_t> freq_list;
  Vector2f wall_shot;
  bool has_wall_shot;

  Vector2f enemy_node;
  Vector2f bot_node;
  std::size_t index;
  Vector2f team_safe;
  Vector2f enemy_safe;

  bool hs_in_1, hs_in_2, hs_in_3, hs_in_4, hs_in_5, hs_in_6, hs_in_7, hs_in_8;
  std::vector<Path> hs_base_paths_;

  std::shared_ptr<GameProxy> game_;
  std::unique_ptr<path::Pathfinder> pathfinder_;
  
  std::unique_ptr<RegionRegistry> regions_;

  behavior::ExecuteContext behavior_ctx_;
 
  int ship_;
  uint64_t last_ship_change_;
  uint64_t last_base_change_;
  SteeringBehavior steering_;
  std::string powerball_arena_;

  std::unique_ptr<behavior::BehaviorEngine> behavior_;
  std::vector<std::unique_ptr<behavior::BehaviorNode>> behavior_nodes_;

  // TODO: Action-key map would be more versatile
  KeyController keys_;
};

bool InRect(marvin::Vector2f pos, marvin::Vector2f min_rect,
    marvin::Vector2f max_rect);

bool IsValidPosition(marvin::Vector2f position);



marvin::Vector2f CalculateShot(const marvin::Vector2f& pShooter,
    const marvin::Vector2f& pTarget,
    const marvin::Vector2f& vShooter,
    const marvin::Vector2f& vTarget,
    float sProjectile);

}  // namespace marvin