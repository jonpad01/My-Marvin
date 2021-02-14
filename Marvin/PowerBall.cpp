#include "PowerBall.h"

#include <chrono>
#include <cstring>
#include <limits>

#include "Debug.h"
#include "GameProxy.h"
#include "Map.h"
#include "RayCaster.h"
#include "RegionRegistry.h"
#include "platform/Platform.h"
#include "platform/ContinuumGameProxy.h"
#include "Shooter.h"

namespace marvin {

    PowerBall::PowerBall(std::unique_ptr<marvin::GameProxy> game) : game_(std::move(game)), keys_(time_.GetTime()), steering_(*game_, keys_) {

        auto processor = std::make_unique<path::NodeProcessor>(*game_);

        last_ship_change_ = 0;
        ship_ = game_->GetPlayer().ship;


        if (ship_ > 7) {
            ship_ = 1;
        }

        pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor));
        regions_ = RegionRegistry::Create(game_->GetMap());
        pathfinder_->CreateMapWeights(game_->GetMap());

        FindPowerBallGoal();
        powerball_arena_ = game_->GetMapFile();

        auto freq_warp_attach = std::make_unique<pb::FreqWarpAttachNode>();
        auto find_enemy = std::make_unique<pb::FindEnemyNode>();
        auto aim_with_gun = std::make_unique<pb::AimWithGunNode>();
        auto aim_with_bomb = std::make_unique<pb::AimWithBombNode>();
        auto target_in_los = std::make_unique<pb::InLineOfSightNode>(
            [](marvin::behavior::ExecuteContext& ctx) {
                const Vector2f* result = nullptr;

                const Player* target_player =
                    ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

                if (target_player) {
                    result = &target_player->position;
                }
                return result;
            });

        auto shoot_gun = std::make_unique<pb::ShootGunNode>();
        auto shoot_bomb = std::make_unique<pb::ShootBombNode>();
        auto path_to_enemy = std::make_unique<pb::PathToEnemyNode>();
        auto move_to_enemy = std::make_unique<pb::MoveToEnemyNode>();
        auto follow_path = std::make_unique<pb::FollowPathNode>();
        auto patrol = std::make_unique<pb::PatrolNode>();


        auto move_method_selector = std::make_unique<behavior::SelectorNode>(move_to_enemy.get());
        auto gun_sequence = std::make_unique<behavior::SequenceNode>(aim_with_gun.get(), shoot_gun.get());
        auto bomb_sequence = std::make_unique<behavior::SequenceNode>(aim_with_bomb.get(), shoot_bomb.get());
        auto bomb_gun_sequence = std::make_unique<behavior::SelectorNode>(gun_sequence.get(), bomb_sequence.get());
        auto parallel_shoot_enemy = std::make_unique<behavior::ParallelNode>(bomb_gun_sequence.get(), move_method_selector.get());
        auto los_shoot_conditional = std::make_unique<behavior::SequenceNode>(target_in_los.get(), parallel_shoot_enemy.get());
        auto enemy_path_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy.get(), follow_path.get());
        auto patrol_path_sequence = std::make_unique<behavior::SequenceNode>(patrol.get(), follow_path.get());
        auto path_or_shoot_selector = std::make_unique<behavior::SelectorNode>(los_shoot_conditional.get(), enemy_path_sequence.get());
        auto handle_enemy = std::make_unique<behavior::SequenceNode>(find_enemy.get(), path_or_shoot_selector.get());
        auto root_selector = std::make_unique<behavior::SelectorNode>(freq_warp_attach.get(), handle_enemy.get(), patrol_path_sequence.get());

        behavior_nodes_.push_back(std::move(freq_warp_attach));
        behavior_nodes_.push_back(std::move(find_enemy));
        behavior_nodes_.push_back(std::move(aim_with_gun));
        behavior_nodes_.push_back(std::move(aim_with_bomb));
        behavior_nodes_.push_back(std::move(target_in_los));
        behavior_nodes_.push_back(std::move(shoot_gun));
        behavior_nodes_.push_back(std::move(shoot_bomb));
        behavior_nodes_.push_back(std::move(path_to_enemy));
        behavior_nodes_.push_back(std::move(move_to_enemy));
        behavior_nodes_.push_back(std::move(follow_path));
        behavior_nodes_.push_back(std::move(patrol));

        behavior_nodes_.push_back(std::move(move_method_selector));
        behavior_nodes_.push_back(std::move(bomb_gun_sequence));
        behavior_nodes_.push_back(std::move(bomb_sequence));
        behavior_nodes_.push_back(std::move(gun_sequence));
        behavior_nodes_.push_back(std::move(parallel_shoot_enemy));
        behavior_nodes_.push_back(std::move(los_shoot_conditional));
        behavior_nodes_.push_back(std::move(enemy_path_sequence));
        behavior_nodes_.push_back(std::move(patrol_path_sequence));
        behavior_nodes_.push_back(std::move(path_or_shoot_selector));
        behavior_nodes_.push_back(std::move(handle_enemy));
        behavior_nodes_.push_back(std::move(root_selector));

        behavior_ = std::make_unique<behavior::BehaviorEngine>(behavior_nodes_.back().get());
        ctx_.pb = this;
    }


    void PowerBall::Update(float dt) {
        keys_.ReleaseAll();
        game_->Update(dt);

        uint64_t timestamp = time_.GetTime();
        uint64_t ship_cooldown = 10000;

        //check chat for disconected message or spec message in eg and terminate continuum
        std::string name = game_->GetName();
        std::string disconnected = "WARNING: ";


        Chat chat = game_->GetChat();

        if (chat.type == 0) {
            if (chat.message.compare(0, 9, disconnected) == 0) {
                exit(5);
            }
        }

        //then check if specced for lag
        if (game_->GetPlayer().ship > 7) {
            uint64_t timestamp = time_.GetTime();
            if (timestamp - last_ship_change_ > ship_cooldown) {
                if (game_->SetShip(ship_)) {
                    last_ship_change_ = timestamp;
                }
            }
            return;
        }



      
            //look at alive time to see if bot has changed arena, then reset pointers for the map
            if (powerball_arena_ != game_->GetMapFile()) {

                powerball_arena_ = game_->GetMapFile();

                auto processor = std::make_unique<path::NodeProcessor>(*game_);
                pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor));

                ctx_.blackboard.Set("path", std::vector<Vector2f>());
                regions_ = RegionRegistry::Create(game_->GetMap());
                pathfinder_->CreateMapWeights(game_->GetMap());

                //set new goals
                FindPowerBallGoal();
            }
            if (!regions_->IsConnected((MapCoord)game_->GetPosition(), (MapCoord)powerball_goal_)) {
                FindPowerBallGoal();
            }
        

        steering_.Reset();


        ctx_.dt = dt;

        behavior_->Update(ctx_);

#if DEBUG_RENDER
        // RenderPath(GetGame(), behavior_ctx_.blackboard);
#endif

        Steer();

    }


    void PowerBall::Steer() {
        Vector2f center = GetWindowCenter();
        float debug_y = 100.0f;
        Vector2f force = steering_.GetSteering();
        float rotation = steering_.GetRotation();
        RenderText("force" + std::to_string(force.Length()), center - Vector2f(0, debug_y), RGB(100, 100, 100), RenderText_Centered);
        Vector2f heading = game_->GetPlayer().GetHeading();
        // Start out by trying to move in the direction that the bot is facing.
        Vector2f steering_direction = heading;

        bool has_force = force.LengthSq() > 0.0f;

        // If the steering system calculated any movement force, then set the movement
        // direction to it.
        if (has_force) {
            steering_direction = marvin::Normalize(force);
        }

        // Rotate toward the movement direction.
        Vector2f rotate_target = steering_direction;

        // If the steering system calculated any rotation then rotate from the heading
        // to desired orientation.
        if (rotation != 0.0f) {
            rotate_target = Rotate(heading, -rotation);
        }

        if (!has_force) {
            steering_direction = rotate_target;
        }
        Vector2f perp = marvin::Perpendicular(heading);
        bool behind = force.Dot(heading) < 0;
        // This is whether or not the steering direction is pointing to the left of
        // the ship.
        bool leftside = steering_direction.Dot(perp) < 0;

        // Cap the steering direction so it's pointing toward the rotate target.
        if (steering_direction.Dot(rotate_target) < 0.75) {
            // RenderText("adjusting", center - Vector2f(0, debug_y), RGB(100, 100, 100), RenderText_Centered);
            // debug_y -= 20;
            float rotation = 0.1f;
            int sign = leftside ? 1 : -1;
            if (behind) sign *= -1;

            // Pick the side of the rotate target that is closest to the force
            // direction.
            steering_direction = Rotate(rotate_target, rotation * sign);

            leftside = steering_direction.Dot(perp) < 0;
        }

        bool clockwise = !leftside;

        if (has_force) {

            //allow bot to use afterburners, force returns high numbers in the hypertunnel so cap it
            bool strong_force = force.Length() > 18.0f && force.Length() < 1000.0f;
            //dont press shift if the bot is trying to shoot
            bool shooting = keys_.IsPressed(VK_CONTROL) || keys_.IsPressed(VK_TAB);

            if (behind) { keys_.Press(VK_DOWN); }
            else { keys_.Press(VK_UP); }

            if (strong_force && !shooting) {
                keys_.Press(VK_SHIFT);
            }

        }


        if (heading.Dot(steering_direction) < 0.996f) {  //1.0f

            if (clockwise) {
                keys_.Press(VK_RIGHT);
            }
            else {
                keys_.Press(VK_LEFT);
            }


            //ensure that only one arrow key is pressed at any given time
            //keys_.Set(VK_RIGHT, clockwise, GetTime());
            //keys_.Set(VK_LEFT, !clockwise, GetTime());
        }

#ifdef DEBUG_RENDER

        if (has_force) {
            Vector2f force_direction = Normalize(force);
            float force_percent = force.Length() / (GetGame().GetShipSettings().MaximumSpeed / 16.0f / 16.0f);
            RenderLine(center, center + (force_direction * 100 * force_percent), RGB(255, 255, 0)); //yellow
        }

        RenderLine(center, center + (heading * 100), RGB(255, 0, 0)); //red
        RenderLine(center, center + (perp * 100), RGB(100, 0, 100));  //purple
        RenderLine(center, center + (rotate_target * 85), RGB(0, 0, 255)); //blue
        RenderLine(center, center + (steering_direction * 75), RGB(0, 255, 0)); //green
        /*
        if (rotation == 0.0f) {
            RenderText("no rotation", center - Vector2f(0, debug_y), RGB(100, 100, 100),
                RenderText_Centered);
            debug_y -= 20;
        }
        else {
            std::string text = "rotation: " + std::to_string(rotation);
            RenderText(text.c_str(), center - Vector2f(0, debug_y), RGB(100, 100, 100),
                RenderText_Centered);
            debug_y -= 20;
        }

        if (behind) {
            RenderText("behind", center - Vector2f(0, debug_y), RGB(100, 100, 100),
                RenderText_Centered);
            debug_y -= 20;
        }

        if (leftside) {
            RenderText("leftside", center - Vector2f(0, debug_y), RGB(100, 100, 100),
                RenderText_Centered);
            debug_y -= 20;
        }

        if (rotation != 0.0f) {
            RenderText("face-locked", center - Vector2f(0, debug_y), RGB(100, 100, 100),
                RenderText_Centered);
            debug_y -= 20;
        }
        */

#endif
    }

    void PowerBall::Move(const Vector2f& target, float target_distance) {
        const Player& bot_player = game_->GetPlayer();
        float distance = bot_player.position.Distance(target);
        Vector2f heading = game_->GetPlayer().GetHeading();

        Vector2f target_position = ctx_.blackboard.ValueOr("target_position", Vector2f());
        float dot = heading.Dot(Normalize(target_position - game_->GetPosition()));



        //seek makes the bot hunt the target hard
        //arrive makes the bot slow down as it gets closer
        //this works with steering.Face in MoveToEnemyNode
        //there is a small gap where the bot wont do anything at all
        if (distance > target_distance) {

            steering_.Seek(target);
        }

        else if (distance <= target_distance) {

            Vector2f to_target = target - bot_player.position;

            steering_.Seek(target - Normalize(to_target) * target_distance);
        }
    }


    void PowerBall::FindPowerBallGoal() {

        powerball_goal_;
        powerball_goal_path_;

        float closest_distance = std::numeric_limits<float>::max();
        int alive_time = game_->GetSettings().BombAliveTime;

        //player is in original
        if (game_->GetMapFile() == "powerlg.lvl") {
            if (game_->GetPlayer().frequency == 00) {
                powerball_goal_ = Vector2f(945, 512);

                std::vector<Vector2f> goal1_paths{ Vector2f(945, 495), Vector2f(926, 512), Vector2f(962, 511), Vector2f(944, 528) };

                for (std::size_t i = 0; i < goal1_paths.size(); i++) {
                    float distance = goal1_paths[i].Distance(game_->GetPosition());
                    if (distance < closest_distance) {
                        powerball_goal_path_ = goal1_paths[i];
                        closest_distance = distance;
                    }
                }
            }
            else {
                powerball_goal_ = Vector2f(72, 510);

                std::vector<Vector2f> goal0_paths{ Vector2f(89, 510), Vector2f(72, 526), Vector2f(55, 509), Vector2f(73, 492) };

                for (std::size_t i = 0; i < goal0_paths.size(); i++) {
                    float distance = goal0_paths[i].Distance(game_->GetPosition());
                    if (distance < closest_distance) {
                        powerball_goal_path_ = goal0_paths[i];
                        closest_distance = distance;
                    }
                }
            }
        }
        //player is in pub
        else if (game_->GetMapFile() == "combo_pub.lvl") {

            std::vector<Vector2f> arenas{ Vector2f(512, 133), Vector2f(512, 224), Vector2f(512, 504), Vector2f(512, 775) };
            std::vector<Vector2f> goal_0{ Vector2f(484, 137), Vector2f(398, 263), Vector2f(326, 512), Vector2f(435, 780) };
            std::vector<Vector2f> goal_1{ Vector2f(540, 137), Vector2f(625, 263), Vector2f(697, 512), Vector2f(587, 780) };

            for (std::size_t i = 0; i < arenas.size(); i++) {

                if (GetRegions().IsConnected((MapCoord)game_->GetPosition(), arenas[i])) {
                    if (game_->GetPlayer().frequency == 00) {
                        powerball_goal_ = goal_1[i];
                        powerball_goal_path_ = powerball_goal_;
                    }
                    else {
                        powerball_goal_ = goal_0[i];
                        powerball_goal_path_ = powerball_goal_;
                    }
                }
            }
        }
    }



    namespace pb {

        behavior::ExecuteResult FreqWarpAttachNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.pb->GetGame();
            uint64_t time = ctx.pb->GetTime().GetTime();

            // probably 775 (7.75 seconds)
            uint16_t hold_time = game.GetSettings().ShipSettings[game.GetPlayer().ship].SoccerBallThrowTimer * 10;

            uint64_t ball_cooldown = ctx.blackboard.ValueOr<uint64_t>("BallCoolDown", 0);
            bool set_timer = ctx.blackboard.ValueOr<bool>("SetTimer", true);

            if (time >= ball_cooldown) {               
                set_timer = true;
                ctx.blackboard.Set("SetTimer", true);
            }

            int64_t timer = hold_time;

            const BallData& ball = game.GetBallData();

            //make this node pass even if the target is a nullptr
            if (ball.held && ball.last_holder_id == game.GetPlayer().id) {
                if (set_timer) {
                    ctx.blackboard.Set("BallCoolDown", time + hold_time);
                    ctx.blackboard.Set("SetTimer", false);
                }
                else {
                    timer = ball_cooldown - time;
                }
            }

            ctx.blackboard.Set("BallTimer", timer);
            //RenderText(std::to_string(timer), GetWindowCenter() - Vector2f(100, 100), RGB(100, 100, 100), RenderText_Centered);

            return behavior::ExecuteResult::Failure;
        }

        behavior::ExecuteResult FindEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            behavior::ExecuteResult result = behavior::ExecuteResult::Failure;
            float closest_cost = std::numeric_limits<float>::max();
            float cost = 0;
            auto& game = ctx.pb->GetGame();
            const Player* target = nullptr;
            const Player& bot = ctx.pb->GetGame().GetPlayer();

           const BallData& ball = game.GetBallData();   
  

            //make this node pass even if the target is a nullptr
            if (ball.held && ball.last_holder_id == game.GetPlayer().id) {
                result = behavior::ExecuteResult::Success;
            }
            //if able to grab the ball patrol to ball
            if (!ball.held) {
                if (ball.inactive_timer > 1200 || ball.last_holder_id != game.GetPlayer().id) {
                    return behavior::ExecuteResult::Failure;
                }
            }
            
            Vector2f position = game.GetPosition();

            //this loop checks every player and finds the closest one based on a cost formula
            //if this does not succeed then there is no target, it will go into patrol mode
            for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
                const Player& player = game.GetPlayers()[i];

                if (!IsValidTarget(ctx, player)) continue;

                if (ball.held && ball.last_holder_id == player.id) {
                    target = &game.GetPlayers()[i];
                    ctx.blackboard.Set("target_player", target);
                    return behavior::ExecuteResult::Success;
                }
                else {

                    auto to_target = player.position - position;
                    CastResult in_sight = RayCast(game.GetMap(), game.GetPosition(), Normalize(to_target), to_target.Length());

                    //make players in line of sight high priority

                    if (!in_sight.hit) {
                        cost = CalculateCost(game, bot, player) / 100;
                    }
                    else {
                        cost = CalculateCost(game, bot, player);
                    }
                    if (cost < closest_cost) {
                        closest_cost = cost;
                        target = &game.GetPlayers()[i];
                        result = behavior::ExecuteResult::Success;
                    }
                }
            }
            //on the first loop, there is nothing in current_target
            const Player* current_target = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
            //const marvin::Player& check_current_target = *current_target;

            //on the following loops, this compares what it found to the current_target
            //and checks if the current_target is still valid
            if (current_target && IsValidTarget(ctx, *current_target)) {
                // Calculate the cost to the current target so there's some stickiness
                // between close targets.
                const float kStickiness = 2.5f;
                float cost = CalculateCost(game, bot, *current_target);

                if (cost * kStickiness < closest_cost) {
                    target = current_target;
                }
            }

            //and this sets the current_target for the next loop
            ctx.blackboard.Set("target_player", target);

            return result;
        }

        float FindEnemyNode::CalculateCost(GameProxy& game, const Player& bot_player, const Player& target) {
            float dist = bot_player.position.Distance(target.position);
            // How many seconds it takes to rotate 180 degrees
            float rotate_speed = game.GetShipSettings().InitialRotation / 200.0f; //MaximumRotation
            float move_cost =
                dist / (game.GetShipSettings().InitialSpeed / 10.0f / 16.0f); //MaximumSpeed
            Vector2f direction = Normalize(target.position - bot_player.position);
            float dot = std::abs(bot_player.GetHeading().Dot(direction) - 1.0f) / 2.0f;
            float rotate_cost = std::abs(dot) * rotate_speed;

            return move_cost + rotate_cost;
        }

        bool FindEnemyNode::IsValidTarget(behavior::ExecuteContext& ctx, const Player& target) {
            const auto& game = ctx.pb->GetGame();
            const Player& bot_player = game.GetPlayer();
            const BallData& ball = game.GetBallData();

            if (!target.active) return false;
            if (target.id == game.GetPlayer().id) return false;
            if (target.ship > 7) return false;

            if (ball.held && ball.last_holder_id == game.GetPlayer().id) {
                if (target.frequency != game.GetPlayer().frequency) return false;
            }
            else {
                if (target.frequency == game.GetPlayer().frequency) return false;
            }

            if (target.name[0] == '<') return false;

            if (game.GetMap().GetTileId(target.position) == marvin::kSafeTileId) {
                return false;
            }

            if (!IsValidPosition(target.position)) {
                return false;
            }

            MapCoord bot_coord(bot_player.position);
            MapCoord target_coord(target.position);

            if (!ctx.pb->GetRegions().IsConnected(bot_coord, target_coord)) {
                return false;
            }
            return true;
        }




        behavior::ExecuteResult PathToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.pb->GetGame();
            std::vector<Vector2f> path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());
            Vector2f bot = game.GetPosition();

            const BallData& ball = game.GetBallData();

            if (ball.held && ball.last_holder_id == game.GetPlayer().id) {

                path = ctx.pb->GetPathfinder().CreatePath(path, bot, ctx.pb->GetPowerBallGoalPath(), game.GetShipSettings().GetRadius());

                ctx.blackboard.Set("path", path);
                return behavior::ExecuteResult::Success;
            }


            const Player* enemy = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            if (enemy == nullptr) {
                return behavior::ExecuteResult::Failure;
            }

            path = ctx.pb->GetPathfinder().CreatePath(path, bot, enemy->position, game.GetShipSettings().GetRadius());

            ctx.blackboard.Set("path", path);
            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult PatrolNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.pb->GetGame();
            Vector2f from = game.GetPosition();
            std::vector<Vector2f> path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());
     
            BallData ball = game.GetBallData();
            
            path = ctx.pb->GetPathfinder().CreatePath(path, from, ball.position, game.GetShipSettings().GetRadius());

            ctx.blackboard.Set("path", path);

            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult FollowPathNode::Execute(behavior::ExecuteContext& ctx) {
            auto path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());

            size_t path_size = path.size();
            auto& game = ctx.pb->GetGame();


            if (path.empty()) return behavior::ExecuteResult::Failure;

            Vector2f current = path.front();
            Vector2f from = game.GetPosition();

            // Fix an issue where the pathfinder generates nodes in the center of the tile
            // but the ship can't occupy the exact center. This is only an issue without path smoothing.
            if (!path.empty() && (u16)from.x == (u16)path.at(0).x && (u16)from.y == (u16)path.at(0).y) {
                path.erase(path.begin());

                if (!path.empty()) {
                    current = path.front();
                }
            }

            while (path.size() > 1 && CanMoveBetween(game, game.GetPosition(), path.at(1))) {
                path.erase(path.begin());
                current = path.front();
            }

            if (path.size() == 1 &&
                path.front().DistanceSq(game.GetPosition()) < 2.0f * 2.0f) {
                path.clear();
            }

            if (path.size() != path_size) {
                ctx.blackboard.Set("path", path);
            }

            ctx.pb->Move(current, 0.0f);

            return behavior::ExecuteResult::Success;
        }

        bool FollowPathNode::CanMoveBetween(GameProxy& game, Vector2f from, Vector2f to) {
            Vector2f trajectory = to - from;
            Vector2f direction = Normalize(trajectory);
            Vector2f side = Perpendicular(direction);

            float distance = from.Distance(to);
            float radius = game.GetShipSettings().GetRadius();

            CastResult center = RayCast(game.GetMap(), from, direction, distance);
            CastResult side1 =
                RayCast(game.GetMap(), from + side * radius, direction, distance);
            CastResult side2 =
                RayCast(game.GetMap(), from - side * radius, direction, distance);

            return !center.hit && !side1.hit && !side2.hit;
        }




        behavior::ExecuteResult InLineOfSightNode::Execute(behavior::ExecuteContext& ctx) {
            behavior::ExecuteResult result = behavior::ExecuteResult::Failure;
            auto target = selector_(ctx);

            auto& game = ctx.pb->GetGame();

            float ball_speed = game.GetSettings().ShipSettings[game.GetPlayer().ship].SoccerBallSpeed / 10.0f / 16.0f;

            const BallData& ball = game.GetBallData();

            int64_t ball_timer = ctx.blackboard.ValueOr<int64_t>("BallTimer", 7750);

          //  if (ctx.com->ShootWall(ctx.pb->GetPowerBallGoal(), Vector2f(0, 0), ball_speed, 400, 10) || ball_time < 200) {
           //     return behavior::ExecuteResult::Success;
          //  }


            if (!target) return behavior::ExecuteResult::Failure;

            auto to_target = *target - game.GetPosition();
            CastResult ray_cast = RayCast(game.GetMap(), game.GetPosition(), Normalize(to_target), to_target.Length());

            if (!ray_cast.hit) {
                result = behavior::ExecuteResult::Success;
            }
            return result;
        }




        behavior::ExecuteResult AimWithGunNode::Execute(behavior::ExecuteContext& ctx) {

            int64_t ball_timer = ctx.blackboard.ValueOr<int64_t>("BallTimer", 7750);

           // if (ctx.pb->HasWallShot() || ball_timer < 200) {
           //     return behavior::ExecuteResult::Success;
          //  }

            const auto target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            if (target_player == nullptr) return behavior::ExecuteResult::Failure;

            const Player& target = *target_player;
            auto& game = ctx.pb->GetGame();
            const Player& bot_player = game.GetPlayer();

            if (!CanShootGun(game, game.GetMap(), game.GetPosition(), target.position)) {
                return behavior::ExecuteResult::Failure;
            }

            float proj_speed = game.GetSettings().ShipSettings[bot_player.ship].BulletSpeed / 10.0f / 16.0f;

            Vector2f target_pos = target.position;

          //  Vector2f seek_position = CalculateShot(game.GetPosition(), target_pos, bot_player.velocity, target.velocity, proj_speed);

            Vector2f projectile_trajectory = (bot_player.GetHeading() * proj_speed) + bot_player.velocity;

            Vector2f projectile_direction = Normalize(projectile_trajectory);

            float target_radius = game.GetSettings().ShipSettings[target.ship].GetRadius();

            float aggression = ctx.blackboard.ValueOr("aggression", 0.0f);
            float radius_multiplier = 1.0f;

            //if the target is cloaking and bot doesnt have xradar make the bot shoot wide
            if (!(game.GetPlayer().status & 4)) {
                if (target.status & 2) {
                    radius_multiplier = 3.0f;
                }
            }

            float nearby_radius = target_radius * radius_multiplier;

            Vector2f box_min = target.position - Vector2f(nearby_radius, nearby_radius);
            Vector2f box_extent(nearby_radius * 1.2f, nearby_radius * 1.2f);
            float dist;
            Vector2f norm;

            bool hit = RayBoxIntersect(bot_player.position, projectile_direction,
                box_min, box_extent, &dist, &norm);

            if (!hit) {
         //       box_min = seek_position - Vector2f(nearby_radius, nearby_radius);
                hit = RayBoxIntersect(bot_player.position, bot_player.GetHeading(),
                    box_min, box_extent, &dist, &norm);
            }


        //    if (seek_position.DistanceSq(target_player->position) < 15 * 15) {
       //         ctx.blackboard.Set("target_position", seek_position);
       //     }
      //      else {
       //         ctx.blackboard.Set("target_position", target.position);
      //      }


            if (hit) {
                return behavior::ExecuteResult::Success;
            }

            return behavior::ExecuteResult::Failure;
        }


        behavior::ExecuteResult AimWithBombNode::Execute(behavior::ExecuteContext& ctx) {
            const auto target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            if (target_player == nullptr) return behavior::ExecuteResult::Failure;

            const Player& target = *target_player;
            auto& game = ctx.pb->GetGame();
            const Player& bot_player = game.GetPlayer();

            if (!CanShootBomb(game, game.GetMap(), game.GetPosition(), target.position)) {
                return behavior::ExecuteResult::Failure;
            }

            float proj_speed = game.GetSettings().ShipSettings[bot_player.ship].BombSpeed / 10.0f / 16.0f;

            Vector2f target_pos = target.position;

         //   Vector2f seek_position = CalculateShot(game.GetPosition(), target_pos, bot_player.velocity, target.velocity, proj_speed);

            Vector2f projectile_trajectory = (bot_player.GetHeading() * proj_speed) + bot_player.velocity;

            Vector2f projectile_direction = Normalize(projectile_trajectory);

            float target_radius = game.GetSettings().ShipSettings[target.ship].GetRadius();

            float radius_multiplier = 0.80f;

            //if the target is cloaking and bot doesnt have xradar make the bot shoot wide
            if (!(game.GetPlayer().status & 4)) {
                if (target.status & 2) {
                    radius_multiplier = 3.0f;
                }
            }

            float nearby_radius = target_radius * radius_multiplier;

            Vector2f box_min = target.position - Vector2f(nearby_radius, nearby_radius);
            Vector2f box_extent(nearby_radius, nearby_radius);
            float dist;
            Vector2f norm;

            bool hit = RayBoxIntersect(bot_player.position, projectile_direction, box_min, box_extent, &dist, &norm);

            if (!hit) {
         //       box_min = seek_position - Vector2f(nearby_radius, nearby_radius);
                hit = RayBoxIntersect(bot_player.position, bot_player.GetHeading(), box_min, box_extent, &dist, &norm);
            }


       //     if (seek_position.DistanceSq(target_player->position) < 15 * 15) {
       //         ctx.blackboard.Set("target_position", seek_position);
       //     }
       //     else {
      //          ctx.blackboard.Set("target_position", target.position);
       //     }

            if (hit) {
                return behavior::ExecuteResult::Success;
            }

            return behavior::ExecuteResult::Failure;
        }



        behavior::ExecuteResult ShootGunNode::Execute(behavior::ExecuteContext& ctx) {
            ctx.pb->GetKeys().Press(VK_CONTROL);
            return behavior::ExecuteResult::Success;
        }


        behavior::ExecuteResult ShootBombNode::Execute(behavior::ExecuteContext& ctx) {
            ctx.pb->GetKeys().Press(VK_TAB);
            return behavior::ExecuteResult::Success;
        }



        behavior::ExecuteResult MoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.pb->GetGame();

         //   if (ctx.pb->HasWallShot()) {
                
          //      ctx.bot->Move(ctx.bot->GetWallShot(), 5.0f);

        //        ctx.bot->GetSteering().Face(ctx.bot->GetWallShot());
        //        return behavior::ExecuteResult::Success;
       //     }


            float energy_pct = game.GetEnergy() / (float)game.GetShipSettings().InitialEnergy;

            bool in_center = ctx.pb->GetRegions().IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));
            bool in_safe = game.GetMap().GetTileId(game.GetPlayer().position) == marvin::kSafeTileId;
            Vector2f target_position = ctx.blackboard.ValueOr("target_position", Vector2f());

            int ship = game.GetPlayer().ship;
            //#if 0
            Vector2f mine_position(0, 0);
            for (Weapon* weapon : game.GetWeapons()) {
                const Player* weapon_player = game.GetPlayerById(weapon->GetPlayerId());
                if (weapon_player == nullptr) continue;
                if (weapon_player->frequency == game.GetPlayer().frequency) continue;
                if (weapon->GetType() & 0x8000) mine_position = (weapon->GetPosition());
                if (mine_position.Distance(game.GetPosition()) < 5.0f) {
                    game.Repel(ctx.pb->GetKeys());
                }
            }
            //#endif
            float hover_distance = 0.0f;


            if (in_safe) hover_distance = 0.0f;
            else if (in_center) {
                hover_distance = 12.0f / energy_pct;
            }
            else {
                hover_distance = 3.0f;
            }
            if (hover_distance < 0.0f) hover_distance = 0.0f;
            const Player& shooter = *ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            ctx.pb->Move(target_position, hover_distance);

            ctx.pb->GetSteering().Face(target_position);

            return behavior::ExecuteResult::Success;
        }

        bool MoveToEnemyNode::IsAimingAt(GameProxy& game, const Player& shooter, const Player& target, Vector2f* dodge) {
            float proj_speed =
                game.GetShipSettings(shooter.ship).BulletSpeed / 10.0f / 16.0f;
            float radius = game.GetShipSettings(target.ship).GetRadius() * 1.5f;
            Vector2f box_pos = target.position - Vector2f(radius, radius);

            Vector2f shoot_direction =
                Normalize(shooter.velocity + (shooter.GetHeading() * proj_speed));

            if (shoot_direction.Dot(shooter.GetHeading()) < 0) {
                shoot_direction = shooter.GetHeading();
            }

            Vector2f extent(radius * 2, radius * 2);

            float shooter_radius = game.GetShipSettings(shooter.ship).GetRadius();
            Vector2f side = Perpendicular(shooter.GetHeading()) * shooter_radius * 1.5f;

            float distance;

            Vector2f directions[2] = { shoot_direction, shooter.GetHeading() };

            for (Vector2f direction : directions) {
                if (RayBoxIntersect(shooter.position, direction, box_pos, extent,
                    &distance, nullptr) ||
                    RayBoxIntersect(shooter.position + side, direction, box_pos, extent,
                        &distance, nullptr) ||
                    RayBoxIntersect(shooter.position - side, direction, box_pos, extent,
                        &distance, nullptr)) {
#if 0
                    Vector2f hit = shooter.position + shoot_direction * distance;

                    *dodge = Normalize(side * side.Dot(Normalize(hit - target.position)));

#endif

                    if (distance < 30) {
                        *dodge = Perpendicular(direction);

                        return true;
                    }
                }
            }

            return false;
        }
    } //namespace pb
} //namespace marvin