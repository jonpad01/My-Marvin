#pragma once

#include <fstream>
#include <memory>

#include "../../Bot.h"
#include "../../Common.h"
#include "../../KeyController.h"
#include "../../Steering.h"
#include "../../Time.h"
#include "../../behavior/BehaviorEngine.h"
#include "../../path/Pathfinder.h"
#include "../../platform/ContinuumGameProxy.h"

namespace marvin {

using Path = std::vector<Vector2f>;

namespace deva {

class DevastationBehaviorBuilder : public BehaviorBuilder {
 public:
  void CreateBehavior(Bot& bot);
};

class DevaDebugNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

// base 8 was reversed in the ini file (vector 7)
class DevaSetRegionNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class DevaFreqMan : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  bool ShouldJoinTeam0(behavior::ExecuteContext& ctx);
  bool ShouldJoinTeam1(behavior::ExecuteContext& ctx);
  bool ShouldLeaveDuelTeam(behavior::ExecuteContext& ctx);
  bool ShouldLeaveCrowdedFreq(behavior::ExecuteContext& ctx);

  std::vector<uint16_t> GetDuelTeamJoiners(behavior::ExecuteContext& ctx);
  std::vector<uint16_t> GetTeamMates(behavior::ExecuteContext& ctx);
  std::vector<uint16_t> GetDuelTeamMates(behavior::ExecuteContext& ctx);

  uint16_t GetLowestID(behavior::ExecuteContext& ctx, std::vector<uint16_t> IDs);
  uint16_t ChooseDueler(behavior::ExecuteContext& ctx, std::vector<uint16_t> IDs);
  uint16_t GetLastPlayerInBase(behavior::ExecuteContext& ctx);

};

class DevaWarpNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class DevaAttachNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  void SetAttachTarget(behavior::ExecuteContext& ctx);
};

class DevaToggleStatusNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class DevaPatrolBaseNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class DevaRepelEnemyNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class DevaBurstEnemyNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class DevaMoveToEnemyNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  bool IsAimingAt(GameProxy& game, const Player& shooter, const Player& target, Vector2f* dodge);
};

}  // namespace deva
}  // namespace marvin
