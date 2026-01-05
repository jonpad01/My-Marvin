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

class HSJackpotQueryNode : public behavior::BehaviorNode {
public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSSetCombatRoleNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSCenterSafePathNode : public behavior::BehaviorNode {
public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSBuySellNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSPrintShipStatusNode : public behavior::BehaviorNode {
 public:
  behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
};

class HSMoveToDepotNode : public behavior::BehaviorNode {
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
private:
  uint64_t GetJoinDelay(behavior::ExecuteContext& ctx);
  uint64_t GetLeaveDelay(behavior::ExecuteContext& ctx);
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

class HSTreeSelector : public BehaviorTreeSelector {
public:
  uint8_t GetBehaviorTree(Bot& bot);
};


class HyperspaceBehaviorBuilder : public BehaviorBuilder {
public:
  void CreateBehavior(Bot& bot);
private:

  void BuildCenterGunnerTree();
  void BuildCenterBomberTree();
  void CenterSaferTree();
  void SpectatorTree();
  void DeadPlayerTree();

  bot::GetOutOfSpecNode* get_out_of_spec;
  HSFreqManagerNode* HS_freqman;
  HSJackpotQueryNode* jackpot_query;
  bot::BouncingShotNode* bouncing_shot;
  bot::PatrolNode* patrol_center;
  bot::MoveToEnemyNode* move_to_enemy;
  HSCenterSafePathNode* center_safe_path;
};

}  // namespace hs
}  // namespace marvin
