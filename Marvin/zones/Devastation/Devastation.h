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

class DevaRunBDNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  void PrintCurrentScore(behavior::ExecuteContext& ctx);
  void PrintFinalScore(behavior::ExecuteContext& ctx);
  void ClearScore(behavior::ExecuteContext& ctx);
  void WarpAllToCenter(behavior::ExecuteContext& ctx);
  void WarpAllToBase(behavior::ExecuteContext& ctx);
};

class DevaFreqMan : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
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
