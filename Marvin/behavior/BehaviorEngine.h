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

  template <typename T, typename... Args>
  inline T* MakeNode(Args&&... args) {
    static_assert(std::is_base_of_v<BehaviorNode, T>,
      "T must derive from BehaviorNode");

    auto node = std::make_unique<T>(std::forward<Args>(args)...);
    T* raw = node.get();                 // non-owning pointer
    behavior_nodes_.push_back(std::move(node));
    return raw;
  }

  template <typename T, typename... Args>
  inline T* MakeRoot(Args&&... args) {
    T* root = MakeNode<T>(std::forward<Args>(args)...);
    behavior_tree_ = root;
    return root;
  }

  inline void PushNode(std::unique_ptr<BehaviorNode> node) { behavior_nodes_.push_back(std::move(node)); }
  inline void PushRoot(std::unique_ptr<BehaviorNode> node) {
    SetBehaviorTree(node.get());
    PushNode(std::move(node));
  }

  void Update(ExecuteContext& ctx);

 private:
   // this is the raw pointer tree that is called to be executed
   // calling get() on a unique_ptr gives this
  BehaviorNode* behavior_tree_;
  // this container keeps the pointer tree alive and is never accessed
  // these are unique_ptr which stays alive as long as the owner is alive
  std::vector<std::unique_ptr<behavior::BehaviorNode>> behavior_nodes_;
};

}  // namespace behavior
}  // namespace marvin
