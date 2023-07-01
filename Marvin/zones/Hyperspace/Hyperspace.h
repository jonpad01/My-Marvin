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
  bool has_summoner;
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
  const Player* SelectAnchor(const AnchorSet& anchors, Bot& bot);
  const Player* FindAnchorInBase(const std::vector<const Player*>& anchors, Bot& bot);
  const Player* FindAnchorInAnyBase(const std::vector<const Player*>& anchors, Bot& bot);
  const Player* FindAnchorInTunnel(const std::vector<const Player*>& anchors, Bot& bot);
  const Player* FindAnchorInCenter(const std::vector<const Player*>& anchors, Bot& bot);
};

class HSSetRegionNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSDropFlagsNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSFlaggerBasePatrolNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSMoveToBaseNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSGatherFlagsNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

 private:
  bool InFlagCollectingShip(uint64_t ship);
  const Flag* SelectFlag(behavior::ExecuteContext& ctx);
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

}  // namespace hs
}  // namespace marvin
