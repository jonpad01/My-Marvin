#pragma once

#include "Bot.h"
#include "Debug.h"

namespace marvin {
    namespace gs {

        class FreqWarpAttachNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        private:
            bool CheckStatus(behavior::ExecuteContext& ctx);
        };

        class FindEnemyNode : public PathingNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

        private:
            float CalculateCost(GameProxy& game, const Player& bot_player, const Player& target);
            bool IsValidTarget(behavior::ExecuteContext& ctx, const Player& target);

            Vector2f view_min_;
            Vector2f view_max_;
        };


        class PathToEnemyNode : public PathingNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };


        class LookingAtEnemyNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        private:
            bool CanShoot(const marvin::Map& map, const marvin::Player& bot_player, const marvin::Player& target);
        };


        class PatrolNode : public PathingNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };


        class FollowPathNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

        private:
            bool CanMoveBetween(GameProxy& game, Vector2f from, Vector2f to);
        };


        class InLineOfSightNode : public behavior::BehaviorNode {
        public:
            using VectorSelector = std::function<const Vector2f * (marvin::behavior::ExecuteContext&)>;
            InLineOfSightNode(VectorSelector selector) : selector_(selector) {}

            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        private:
            VectorSelector selector_;
        };


        class ShootEnemyNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };



        class MoveToEnemyNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

        private:
            bool IsAimingAt(GameProxy& game, const Player& shooter, const Player& target, Vector2f* dodge);
        };
    } // namespace gs
}  // namespace marvin
