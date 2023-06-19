#pragma once

#include <fstream>
#include <memory>

#include "../Bot.h"
#include "../Common.h"
#include "../KeyController.h"
#include "../Steering.h"
#include "../Time.h"
#include "../behavior/BehaviorEngine.h"
#include "../path/Pathfinder.h"
#include "../platform/ContinuumGameProxy.h"

namespace marvin {
namespace eg {

class ExtremeGamesBehaviorBuilder : public BehaviorBuilder {
 public:
  void CreateBehavior(Bot& bot);
};

class FreqWarpAttachNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  bool CheckStatus(behavior::ExecuteContext& ctx);
};




class AimWithGunNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class AimWithBombNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class ShootGunNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class ShootBombNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class MoveToEnemyNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  bool IsAimingAt(GameProxy& game, const Player& shooter, const Player& target, Vector2f* dodge);
};
}  // namespace eg
}  // namespace marvin