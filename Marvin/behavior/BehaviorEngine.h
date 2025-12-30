#pragma once

#include <vector>
#include <unordered_map>

#include "BehaviorTree.h"
#include "..//Debug.h"

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
  inline T* MakeRoot(uint8_t key, Args&&... args) {
    T* root = MakeNode<T>(std::forward<Args>(args)...);

    behavior_trees_[key] = root;
    SetBehaviorTree(root);
    key_ = key;
    return root;
  }

  inline void PushNode(std::unique_ptr<BehaviorNode> node) { behavior_nodes_.push_back(std::move(node)); }
  inline void PushRoot(std::unique_ptr<BehaviorNode> node) {
    SetBehaviorTree(node.get());
    PushNode(std::move(node));
  }

  void Update(ExecuteContext& ctx);

  void SetActiveTree(uint8_t key) {
    if (key_ == key) return;
    
    auto it = behavior_trees_.find(key);

    if (it != behavior_trees_.end()) {
      behavior_tree_ = it->second;
      key_ = key;
    }
  }

 private:
   // this is the raw pointer tree that is called to be executed
   // calling get() on a unique_ptr gives this
  BehaviorNode* behavior_tree_;
  std::unordered_map<uint8_t, BehaviorNode*> behavior_trees_;
  uint8_t key_ = 99;
  //std::vector<BehaviorNode*> behavior_trees__;
  // this container keeps the pointer tree alive and is never accessed
  // these are unique_ptr which stays alive as long as the owner is alive
  std::vector<std::unique_ptr<behavior::BehaviorNode>> behavior_nodes_;
};

}  // namespace behavior
}  // namespace marvin
