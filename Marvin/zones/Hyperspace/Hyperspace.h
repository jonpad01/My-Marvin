#pragma once

#include <fstream>
#include <memory>

#include "..//..//Bot.h"
#include "..//..//behavior/BehaviorEngine.h"

namespace marvin {

namespace hs {

struct AnchorResult {
  AnchorSet anchors;
  bool found;
};

class HyperspaceBehaviorBuilder : public BehaviorBuilder {
 public:
  void CreateBehavior(Bot& bot);
};

class HSPlayerSortNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);


 private:

  AnchorResult GetAnchors(Bot& bot);
  AnchorPlayer SelectAnchor(const AnchorSet& anchors, Bot& bot);
  AnchorPlayer FindAnchorInBase(const std::vector<AnchorPlayer>& anchors, Bot& bot);
  AnchorPlayer FindAnchorInAnyBase(const std::vector<AnchorPlayer>& anchors, Bot& bot);
  AnchorPlayer FindAnchorInTunnel(const std::vector<AnchorPlayer>& anchors, Bot& bot);
  AnchorPlayer FindAnchorInCenter(const std::vector<AnchorPlayer>& anchors, Bot& bot);
};

class HSSetRegionNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSFlaggerPatrolNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSSetDefensePositionNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSFreqManagerNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSFlaggingCheckNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSShipManagerNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  uint16_t SelectShip(uint16_t current);
};

class HSWarpToCenterNode : public behavior::BehaviorNode {
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
