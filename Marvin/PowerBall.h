#pragma once

#include <fstream>
#include <memory>

#include "KeyController.h"
#include "Common.h"
#include "Steering.h"
#include "behavior/BehaviorEngine.h"
#include "path/Pathfinder.h"
#include "platform/ContinuumGameProxy.h"
#include "Time.h"

namespace marvin {
        

    class PowerBall {
    public:
        PowerBall(std::unique_ptr<GameProxy> game);

        void Update(float dt);

        KeyController& GetKeys() { return keys_; }
        GameProxy& GetGame() { return *game_; }
        Time& GetTime() { return time_; }

        void Move(const Vector2f& target, float target_distance);
        path::Pathfinder& GetPathfinder() { return *pathfinder_; }

        const RegionRegistry& GetRegions() const { return *regions_; }

        SteeringBehavior& GetSteering() { return steering_; }


        void AddBehaviorNode(std::unique_ptr<behavior::BehaviorNode> node) {
            behavior_nodes_.push_back(std::move(node));
        }

        void SetBehavior(behavior::BehaviorNode* behavior) {
            behavior_ = std::make_unique<behavior::BehaviorEngine>(behavior);
        }

   
        Vector2f GetPowerBallGoal() { return powerball_goal_; }
        Vector2f GetPowerBallGoalPath() { return powerball_goal_path_; }



    private:
        void Steer();

        void FindPowerBallGoal();

        Vector2f powerball_goal_;
        Vector2f powerball_goal_path_;

        std::string powerball_arena_;

        int ship_;
        uint64_t last_ship_change_;

        bool in_center_;

        std::unique_ptr<path::Pathfinder> pathfinder_;
        std::unique_ptr<RegionRegistry> regions_;
        std::unique_ptr<GameProxy> game_;
        std::unique_ptr<behavior::BehaviorEngine> behavior_;
        std::vector<std::unique_ptr<behavior::BehaviorNode>> behavior_nodes_;
        behavior::ExecuteContext ctx_;

        KeyController keys_;
        SteeringBehavior steering_;
        Time time_;

    };



        namespace pb {

            class BallSelectorNode : public behavior::BehaviorNode {
            public:
                behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
            private:
       
            };

        class FreqWarpAttachNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        private:
            bool CheckStatus(behavior::ExecuteContext& ctx);
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
 
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        private:

        };


        class AimWithGunNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };


        class AimWithBombNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };


        class ShootGunNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };



        class ShootBombNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        };



        class MoveToEnemyNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);

        private:
            bool IsAimingAt(GameProxy& game, const Player& shooter, const Player& target, Vector2f* dodge);
        };
    } // namespace pb
}  // namespace marvin