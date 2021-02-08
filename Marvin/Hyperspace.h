#pragma once

#include <fstream>
#include <memory>

#include "KeyController.h"
#include "Common.h"
#include "Steering.h"
#include "behavior/BehaviorEngine.h"
#include "path/Pathfinder.h"
#include "platform/ContinuumGameProxy.h"

namespace marvin {
   
    struct Flaggers {
        const Player* enemy_anchor; const Player* team_anchor;
        int flaggers, team_lancs, team_spiders;

    };

    class Hyperspace {
    public:
        Hyperspace(std::unique_ptr<GameProxy> game);

        void Update(float dt);

        KeyController& GetKeys() { return keys_; }
        GameProxy& GetGame() { return *game_; }

        void Move(const Vector2f& target, float target_distance);

        path::Pathfinder& GetPathfinder() { return *pathfinder_; }

        const RegionRegistry& GetRegions() const { return *regions_; }

        SteeringBehavior& GetSteering() { return steering_; }
        std::vector<uint16_t> GetFreqList() { return freq_list; }


        void AddBehaviorNode(std::unique_ptr<behavior::BehaviorNode> node) {
            behavior_nodes_.push_back(std::move(node));
        }

        void SetBehavior(behavior::BehaviorNode* behavior) {
            behavior_ = std::make_unique<behavior::BehaviorEngine>(behavior);
        }

        bool InCenter() { return in_center; }

        int BaseSelector();

        Flaggers FindFlaggers();

        bool CheckStatus();

        std::vector<Vector2f> GetBasePaths(std::size_t base_number) { return hs_base_paths_[base_number]; }

        bool InHSBase(Vector2f position);

        std::vector<bool> InHSBaseNumber(Vector2f position);

    private:
        void Steer();

        void CreateHSBasePaths();
        void CheckHSBaseRegions(Vector2f position);

        void CheckCenterRegion();

        bool hs_in_1, hs_in_2, hs_in_3, hs_in_4, hs_in_5, hs_in_6, hs_in_7, hs_in_8;
        std::vector<std::vector<Vector2f>> hs_base_paths_;
        uint64_t last_base_change_;

        std::vector<uint16_t> freq_list;

        int ship_;
        uint64_t last_ship_change_;

        bool in_center;

        std::unique_ptr<path::Pathfinder> pathfinder_;
        std::unique_ptr<RegionRegistry> regions_;
        std::unique_ptr<GameProxy> game_;
        std::unique_ptr<behavior::BehaviorEngine> behavior_;
        std::vector<std::unique_ptr<behavior::BehaviorNode>> behavior_nodes_;
        behavior::ExecuteContext ctx_;

        Common common_;
        KeyController keys_;
        SteeringBehavior steering_;


    };


        namespace hs {

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










