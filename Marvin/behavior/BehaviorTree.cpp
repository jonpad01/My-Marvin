#include "BehaviorTree.h"

namespace marvin {
namespace behavior {

SequenceNode::SequenceNode(std::vector<BehaviorNode*> children)
    : children_(std::move(children)), running_node_index_(-1) {}

ExecuteResult SequenceNode::Execute(ExecuteContext& ctx) {
  std::size_t index = 0;

  if (running_node_index_ < children_.size()) {
    index = running_node_index_;
  }

  for (; index < children_.size(); ++index) {
    auto node = children_[index];
    ExecuteResult result = node->Execute(ctx);

    if (result == ExecuteResult::Failure || result == ExecuteResult::Stop) {
      return result;
    } else if (result == ExecuteResult::Running) {
      this->running_node_index_ = index;
      return result;
    }
  }

  return ExecuteResult::Success;
}

// allways run all nodes, return success if any succeed, stop overrides success
ParallelAnyNode::ParallelAnyNode(std::vector<BehaviorNode*> children) : children_(std::move(children)) {}

ExecuteResult ParallelAnyNode::Execute(ExecuteContext& ctx) {
  ExecuteResult result = ExecuteResult::Failure;
  bool stopped = false;

  for (auto& child : children_) {
    ExecuteResult child_result = child->Execute(ctx);

    if (child_result == ExecuteResult::Stop) {
      stopped = true;
    }

    if (child_result == ExecuteResult::Success) {
      result = child_result;
    }
  }

  if (stopped) return ExecuteResult::Stop;
  return result;
}

SelectorNode::SelectorNode(std::vector<BehaviorNode*> children) : children_(std::move(children)) {}

ExecuteResult SelectorNode::Execute(ExecuteContext& ctx) {
  ExecuteResult result = ExecuteResult::Failure;

  for (auto& child : children_) {
    ExecuteResult child_result = child->Execute(ctx);

    if (child_result == ExecuteResult::Running || child_result == ExecuteResult::Success || child_result == ExecuteResult::Stop) {
      return child_result;
    }
  }

  return result;
}

SuccessNode::SuccessNode(BehaviorNode* child) : child_(child) {}

ExecuteResult SuccessNode::Execute(ExecuteContext& ctx) {
  child_->Execute(ctx);
  return ExecuteResult::Success;
}

InvertNode::InvertNode(BehaviorNode* child) : child_(child) {}

ExecuteResult InvertNode::Execute(ExecuteContext& ctx) {
  ExecuteResult child_result = child_->Execute(ctx);

  if (child_result == ExecuteResult::Success) {
    return ExecuteResult::Failure;
  } else if (child_result == ExecuteResult::Failure) {
    return ExecuteResult::Success;
  }

  return child_result;
}
}  // namespace behavior
}  // namespace marvin
