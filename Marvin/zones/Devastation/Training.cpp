#include "Training.h"

#include <chrono>
#include <cstring>
#include <limits>

#include "../../Bot.h"
#include "../../Debug.h"
#include "../../GameProxy.h"
#include "../../Map.h"
#include "../../RayCaster.h"
#include "../../RegionRegistry.h"
#include "../../Shooter.h"
#include "../../platform/ContinuumGameProxy.h"
#include "../../platform/Platform.h"

namespace marvin {
namespace training {


  void TrainingBehaviorBuilder::CreateBehavior(Bot& bot) {

    auto template_node = std::make_unique<TemplateNode>();

    auto move_to_enemy = std::make_unique<bot::MoveToEnemyNode>();

    auto move_method_selector = std::make_unique<behavior::SelectorNode>(move_to_enemy.get());
    auto los_weapon_selector = std::make_unique<behavior::SelectorNode>(shoot_enemy_.get());
    auto parallel_shoot_enemy =
        std::make_unique<behavior::ParallelNode>(los_weapon_selector.get(), move_method_selector.get());

    auto path_to_enemy_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy_.get(), follow_path_.get());

    auto los_shoot_conditional =
        std::make_unique<behavior::SequenceNode>(target_in_los_.get(), parallel_shoot_enemy.get());

    auto path_or_shoot_selector =
        std::make_unique<behavior::SelectorNode>(los_shoot_conditional.get(), path_to_enemy_sequence.get());

    auto find_enemy_in_center_sequence =
        std::make_unique<behavior::SequenceNode>(find_enemy_in_center_.get(), path_or_shoot_selector.get());

    auto action_selector = std::make_unique<behavior::SelectorNode>(find_enemy_in_center_sequence.get());

    auto root_sequence =
        std::make_unique<behavior::SequenceNode>(template_node, respawn_check_.get(), commands_.get(), set_ship_.get(),
                                                 set_freq_.get(), ship_check_.get(), action_selector.get());

    engine_->PushRoot(std::move(root_sequence));

    engine_->PushNode(std::move(template_node));
    engine_->PushNode(std::move(move_to_enemy));
    engine_->PushNode(std::move(move_method_selector));
    engine_->PushNode(std::move(los_weapon_selector));
    engine_->PushNode(std::move(parallel_shoot_enemy));
    engine_->PushNode(std::move(path_to_enemy_sequence));
    engine_->PushNode(std::move(los_shoot_conditional));
    engine_->PushNode(std::move(path_or_shoot_selector));
    engine_->PushNode(std::move(find_enemy_in_center_sequence));
    engine_->PushNode(std::move(action_selector));
  }

  behavior::ExecuteResult TemplateNode::Execute(behavior::ExecuteContext& ctx) {

      if (ctx.bot->GetTime().TimedActionDelay("templatenode", 1000)) {
      ctx.bot->GetGame().SendChatMessage("Template node is running");
      }

    return behavior::ExecuteResult::Success;
    return behavior::ExecuteResult::Failure;
  }


}  // namespace training
}  // namespace marvin
