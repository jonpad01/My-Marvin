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

namespace training {

class TrainingBehaviorBuilder : public BehaviorBuilder {
 public:
  void CreateBehavior(Bot& bot);
};

class TemplateNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
};

}  // namespace training
}  // namespace marvin
