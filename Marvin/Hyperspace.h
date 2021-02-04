#pragma once

#include "Bot.h"
#include "Debug.h"

namespace marvin {
    namespace hs {


        class Hyperspace {



        };




        class FreqWarpAttachNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };

        class FindEnemyNode : public behavior::BehaviorNode {//: public PathingNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

        private:
            float CalculateCost(GameProxy& game, const Player& bot_player, const Player& target);

            bool IsValidTarget(behavior::ExecuteContext& ctx, const Player& target);

            Vector2f view_min_;
            Vector2f view_max_;
        };


        class PathToEnemyNode : public behavior::BehaviorNode {//: public PathingNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        private:
            void FindClosestNodes(behavior::ExecuteContext& ctx);
            bool IsDefendingAnchor(behavior::ExecuteContext& ctx);
            float DistanceToEnemy(behavior::ExecuteContext& ctx);

            Vector2f enemy_;
            Flaggers flaggers_;
            std::vector<Vector2f> base_path_;
            std::size_t enemy_node;
            std::size_t enemy_anchor_node;
            std::size_t bot_node;
        };


        class PatrolNode : public behavior::BehaviorNode {//: public PathingNode {
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
            using VectorSelector =
                std::function<const Vector2f * (marvin::behavior::ExecuteContext&)>;
            InLineOfSightNode(VectorSelector selector) : selector_(selector) {}

            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

        private:
            VectorSelector selector_;
        };




        class LookingAtEnemyNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };


        class ShootEnemyNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };


        class BundleShots : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

        private:
            bool ShouldActivate(behavior::ExecuteContext& ctx, const Player& target);

            uint64_t start_time_;
            bool running_;
        };

        class MoveToEnemyNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

        private:
            bool IsAimingAt(GameProxy& game, const Player& shooter, const Player& target, Vector2f* dodge);
        };




    } // namesspace hs
}  // namespace marvin










