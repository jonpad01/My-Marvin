#pragma once

#include <vector>

#include "BehaviorTree.h"

namespace marvin {
namespace behavior {

class BehaviorEngine {
 public:
  BehaviorEngine(BehaviorNode* behavior_tree);
  BehaviorEngine() : behavior_tree_(nullptr) {}

  inline void SetBehaviorTree(BehaviorNode* root) { behavior_tree_ = root; }
  inline void PushNode(std::unique_ptr<BehaviorNode> node) { behavior_nodes_.push_back(std::move(node)); }
  inline void PushRoot(std::unique_ptr<BehaviorNode> node) {
    SetBehaviorTree(node.get());
    PushNode(std::move(node));
  }

  void Update(ExecuteContext& ctx);

 private:
  BehaviorNode* behavior_tree_;
  std::vector<std::unique_ptr<behavior::BehaviorNode>> behavior_nodes_;
};

}  // namespace behavior
}  // namespace marvin
