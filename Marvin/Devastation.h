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
    //namespace deva {

        using Path = std::vector<Vector2f>;

        class GameProxy;
        struct Player;

    class Devastation {

    public:
        Devastation(std::shared_ptr<GameProxy> game, std::shared_ptr<GameProxy> game2, std::shared_ptr<GameProxy> game3);

        void Update(float dt);

        KeyController& GetKeys() { return keys_; }
        GameProxy& GetGame() { return *game_; }

        void Move(const Vector2f& target, float target_distance);
        path::Pathfinder& GetPathfinder() { return *pathfinder_; }

        const RegionRegistry& GetRegions() const { return *regions_; }

        SteeringBehavior& GetSteering() { return steering_; }

        Path CreatePath(behavior::ExecuteContext& ctx, const std::string& pathname, Vector2f from, Vector2f to, float radius);

        void AddBehaviorNode(std::unique_ptr<behavior::BehaviorNode> node) {
            behavior_nodes_.push_back(std::move(node));
        }

        void SetBehavior(behavior::BehaviorNode* behavior) {
            behavior_ = std::make_unique<behavior::BehaviorEngine>(behavior);
        }

       
        std::vector<uint16_t> GetFreqList() { return freq_list; }
        std::vector<Player>& GetDevaTeam() { return team; }
        std::vector<Player>& GetDevaEnemyTeam() { return enemy_team; }
        std::vector<Player>& GetDevaDuelers() { return duelers; }
        bool LastBotStanding() { return last_bot_standing; }
        bool InCenter() { return in_center_; }
        Vector2f GetTeamSafe() { return team_safe; }
        Vector2f GetEnemySafe() { return enemy_safe; }
        std::vector<Vector2f> GetBasePath() { return base_paths[base_index_]; }



        bool CanShootGun(const Map& map, const Player& bot_player, const Player& target);
        bool CanShootBomb(const Map& map, const Player& bot_player, const Player& target);

        void LookForWallShot(Vector2f target_pos, Vector2f target_vel, float proj_speed, int alive_time, uint8_t bounces);
        bool CalculateWallShot(Vector2f target_pos, Vector2f target_vel, Vector2f direction, float proj_speed, int alive_time, uint8_t bounces);
        Vector2f GetWallShot() { return wall_shot; }
        bool HasWallShot() { return has_wall_shot; }


    private:
        void Steer();



        void CreateBasePaths();
        void SortPlayers();
        void SetRegion();

        int ship_;
        uint64_t last_ship_change_;

        bool in_center_;
        std::size_t base_index_;
        Vector2f current_base_;

        std::vector<uint16_t> freq_list;
        Vector2f wall_shot;
        bool has_wall_shot;
        Vector2f enemy_node;
        Vector2f bot_node;
     
        Vector2f team_safe;
        Vector2f enemy_safe;
        bool last_bot_standing;
   
        //these must be cleared before use as they are never deconstructed
        std::vector<Player> team;
        std::vector<Player> enemy_team;
        std::vector<Player> duelers;
        //these are calculated once and saved
        std::vector<std::vector<Vector2f>> base_paths;


        std::unique_ptr<path::Pathfinder> pathfinder_;
        std::unique_ptr<RegionRegistry> regions_;
        std::shared_ptr<GameProxy> game_;
        //std::unique_ptr<Common> common_;
        std::unique_ptr<behavior::BehaviorEngine> behavior_;
        std::vector<std::unique_ptr<behavior::BehaviorNode>> behavior_nodes_;
        behavior::ExecuteContext ctx_;   

        Common common_;
        KeyController keys_;
        SteeringBehavior steering_;
        
    };


    namespace deva {
        //base 8 was reversed in the ini file (vector 7)
        const std::vector<Vector2f> team0_safes { Vector2f(32, 56), Vector2f(185, 58), Vector2f(247, 40), Vector2f(383, 64), Vector2f(579, 62), Vector2f(667, 56), Vector2f(743, 97), Vector2f(963, 51),
                                                  Vector2f(32, 111), Vector2f(222, 82), Vector2f(385, 148), Vector2f(499, 137), Vector2f(624, 185), Vector2f(771, 189), Vector2f(877, 110), Vector2f(3, 154),
                                                  Vector2f(242, 195), Vector2f(326, 171), Vector2f(96, 258), Vector2f(182, 303), Vector2f(297, 287), Vector2f(491, 296), Vector2f(555, 324), Vector2f(764, 259),
                                                  Vector2f(711, 274), Vector2f(904, 303), Vector2f(170, 302), Vector2f(147, 379), Vector2f(280, 310), Vector2f(425, 326), Vector2f(569, 397), Vector2f(816, 326),
                                                  Vector2f(911, 347), Vector2f(97, 425), Vector2f(191, 471), Vector2f(395, 445), Vector2f(688, 392), Vector2f(696, 545), Vector2f(854, 447), Vector2f(16, 558),
                                                  Vector2f(245, 610), Vector2f(270, 595), Vector2f(5, 641), Vector2f(381, 610), Vector2f(422, 634), Vector2f(565, 584), Vector2f(726, 598), Vector2f(859, 639),
                                                  Vector2f(3, 687), Vector2f(135, 710), Vector2f(287, 715), Vector2f(414, 687), Vector2f(612, 713), Vector2f(726, 742), Vector2f(1015, 663), Vector2f(50, 806),
                                                  Vector2f(160, 841), Vector2f(310, 846), Vector2f(497, 834), Vector2f(585, 861), Vector2f(737, 755), Vector2f(800, 806), Vector2f(913, 854), Vector2f(91, 986),
                                                  Vector2f(175, 901), Vector2f(375, 887), Vector2f(499, 909), Vector2f(574, 846), Vector2f(699, 975), Vector2f(864, 928), Vector2f(221, 1015), Vector2f(489, 1008),
                                                  Vector2f(769, 983), Vector2f(948, 961), Vector2f(947, 511) };

        const std::vector<Vector2f> team1_safes { Vector2f(102, 56), Vector2f(189, 58), Vector2f(373, 40), Vector2f(533, 64), Vector2f(606, 57), Vector2f(773, 56), Vector2f(928, 46), Vector2f(963, 47),
                                                  Vector2f(140, 88), Vector2f(280, 82), Vector2f(395, 148), Vector2f(547, 146), Vector2f(709, 117), Vector2f(787, 189), Vector2f(993, 110), Vector2f(181, 199),
                                                  Vector2f(258, 195), Vector2f(500, 171), Vector2f(100, 258), Vector2f(280, 248), Vector2f(446, 223), Vector2f(524, 215), Vector2f(672, 251), Vector2f(794, 259),
                                                  Vector2f(767, 274), Vector2f(972, 215), Vector2f(170, 336), Vector2f(170, 414), Vector2f(314, 391), Vector2f(455, 326), Vector2f(623, 397), Vector2f(859, 409),
                                                  Vector2f(1013, 416), Vector2f(97, 502), Vector2f(228, 489), Vector2f(365, 445), Vector2f(770, 392), Vector2f(786, 545), Vector2f(903, 552), Vector2f(122, 558),
                                                  Vector2f(245, 650), Vector2f(398, 595), Vector2f(130, 640), Vector2f(381, 694), Vector2f(422, 640), Vector2f(626, 635), Vector2f(808, 598), Vector2f(1004, 583),
                                                  Vector2f(138, 766), Vector2f(241, 795), Vector2f(346, 821), Vector2f(525, 687), Vector2f(622, 703), Vector2f(862, 742), Vector2f(907, 769), Vector2f(98, 806),
                                                  Vector2f(295, 864), Vector2f(453, 849), Vector2f(504, 834), Vector2f(682, 792), Vector2f(746, 810), Vector2f(800, 812), Vector2f(1004, 874), Vector2f(57, 909),
                                                  Vector2f(323, 901), Vector2f(375, 996), Vector2f(503, 909), Vector2f(694, 846), Vector2f(720, 895), Vector2f(881, 945), Vector2f(272, 1015), Vector2f(617, 929),
                                                  Vector2f(775, 983), Vector2f(948, 983), Vector2f(1012, 494) };
                                                  
        

       

        

        class FreqWarpAttachNode : public behavior::BehaviorNode {
        public:
            behavior::ExecuteResult Execute(behavior::ExecuteContext& ctx);
        private:
            bool CheckStatus(behavior::ExecuteContext& ctx);
            void SetAttachTarget(behavior::ExecuteContext& ctx);
            bool TeamInBase(behavior::ExecuteContext& ctx);
            bool EnemyInBase(behavior::ExecuteContext& ctx);

            const Player* lag_attach_target;
            const Player* attach_target;
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


        class LookingAtEnemyNode : public behavior::BehaviorNode {
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
    } // namespace deva
}  // namespace marvin
