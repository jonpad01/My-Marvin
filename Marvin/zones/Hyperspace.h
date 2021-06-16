#pragma once

#include <fstream>
#include <memory>

#include "../behavior/BehaviorEngine.h"

namespace marvin {

class Bot;

namespace hs {

void Initialize(Bot& bot);

class HSFlaggerSort : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSSetRegionNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSDefensePositionNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSFreqMan : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSShipMan : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSWarpToCenter : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSAttachNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSToggleNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSPatrolBaseNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

}  // namespace hs
}  // namespace marvin
