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

class BaseDuelBehaviorBuilder : public BehaviorBuilder {
 public:
  void CreateBehavior(Bot& bot);
};

class StartBDNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  void ClearScore(behavior::ExecuteContext& ctx);
};

class PauseBDNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class EndBDNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

class RunBDNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  void PrintCurrentScore(behavior::ExecuteContext& ctx);
  void PrintFinalScore(behavior::ExecuteContext& ctx);
  void ClearScore(behavior::ExecuteContext& ctx);
  void WarpAllToCenter(behavior::ExecuteContext& ctx);
  void WarpAllToBase(behavior::ExecuteContext& ctx);
};

}  // namespace training
}  // namespace marvin
