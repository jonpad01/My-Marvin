#include "Hockey.h"

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

namespace marvin {

    Hockey::Hockey(std::unique_ptr<marvin::GameProxy> game) : game_(std::move(game)), steering_(*game_), common_(*game_), keys_(common_.GetTime()) {

        auto processor = std::make_unique<path::NodeProcessor>(*game_);

        last_ship_change_ = 0;
        ship_ = game_->GetPlayer().ship;


        if (ship_ > 7) {
            ship_ = 2;
        }

        pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor));
        regions_ = RegionRegistry::Create(game_->GetMap());
        pathfinder_->CreateMapWeights(game_->GetMap());

        auto freq_warp_attach = std::make_unique<hz::FreqWarpAttachNode>();
        auto find_enemy = std::make_unique<hz::FindEnemyNode>();
        auto looking_at_enemy = std::make_unique<hz::LookingAtEnemyNode>();
        auto target_in_los = std::make_unique<hz::InLineOfSightNode>(
            [](marvin::behavior::ExecuteContext& ctx) {
                const Vector2f* result = nullptr;

                const Player* target_player =
                    ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

                if (target_player) {
                    result = &target_player->position;
                }
                return result;
            });

        auto shoot_enemy = std::make_unique<hz::ShootEnemyNode>();
        auto path_to_enemy = std::make_unique<hz::PathToEnemyNode>();
        auto move_to_enemy = std::make_unique<hz::MoveToEnemyNode>();
        auto follow_path = std::make_unique<hz::FollowPathNode>();
        auto patrol = std::make_unique<hz::PatrolNode>();


        auto move_method_selector = std::make_unique<behavior::SelectorNode>(move_to_enemy.get());
        auto shoot_sequence = std::make_unique<behavior::SequenceNode>(looking_at_enemy.get(), shoot_enemy.get());
        auto parallel_shoot_enemy = std::make_unique<behavior::ParallelNode>(shoot_sequence.get(), move_method_selector.get());
        auto los_shoot_conditional = std::make_unique<behavior::SequenceNode>(target_in_los.get(), parallel_shoot_enemy.get());
        auto enemy_path_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy.get(), follow_path.get());
        auto patrol_path_sequence = std::make_unique<behavior::SequenceNode>(patrol.get(), follow_path.get());
        auto path_or_shoot_selector = std::make_unique<behavior::SelectorNode>(los_shoot_conditional.get(), enemy_path_sequence.get());
        auto handle_enemy = std::make_unique<behavior::SequenceNode>(find_enemy.get(), path_or_shoot_selector.get());
        auto root_selector = std::make_unique<behavior::SelectorNode>(freq_warp_attach.get(), handle_enemy.get(), patrol_path_sequence.get());

        behavior_nodes_.push_back(std::move(freq_warp_attach));
        behavior_nodes_.push_back(std::move(find_enemy));
        behavior_nodes_.push_back(std::move(looking_at_enemy));
        behavior_nodes_.push_back(std::move(target_in_los));
        behavior_nodes_.push_back(std::move(shoot_enemy));
        behavior_nodes_.push_back(std::move(path_to_enemy));
        behavior_nodes_.push_back(std::move(move_to_enemy));
        behavior_nodes_.push_back(std::move(follow_path));
        behavior_nodes_.push_back(std::move(patrol));

        behavior_nodes_.push_back(std::move(move_method_selector));
        behavior_nodes_.push_back(std::move(shoot_sequence));
        behavior_nodes_.push_back(std::move(parallel_shoot_enemy));
        behavior_nodes_.push_back(std::move(los_shoot_conditional));
        behavior_nodes_.push_back(std::move(enemy_path_sequence));
        behavior_nodes_.push_back(std::move(patrol_path_sequence));
        behavior_nodes_.push_back(std::move(path_or_shoot_selector));
        behavior_nodes_.push_back(std::move(handle_enemy));
        behavior_nodes_.push_back(std::move(root_selector));

        behavior_ = std::make_unique<behavior::BehaviorEngine>(behavior_nodes_.back().get());
        ctx_.hz = this;
        ctx_.com = &common_;
    }

    void Hockey::Update(float dt) {
        keys_.ReleaseAll();
        game_->Update(dt);

        uint64_t timestamp = common_.GetTime();
        uint64_t ship_cooldown = 10000;

        //check chat for disconected message and terminate continuum
        std::string name = game_->GetName();
        std::string disconnected = "WARNING: ";

        std::vector<std::string> chat = game_->GetChat(0);

        for (std::size_t i = 0; i < chat.size(); i++) {
            if (chat[i].compare(0, 9, disconnected) == 0) {
                exit(5);
            }
        }

        //then check if specced for lag
        if (game_->GetPlayer().ship > 7) {
            uint64_t timestamp = common_.GetTime();
            if (timestamp - last_ship_change_ > ship_cooldown) {
                if (game_->SetShip(ship_)) {
                    last_ship_change_ = timestamp;
                }
            }
            return;
        }

        steering_.Reset();


        ctx_.dt = dt;

        behavior_->Update(ctx_);

#if DEBUG_RENDER
        // RenderPath(GetGame(), behavior_ctx_.blackboard);
#endif

        Steer();

    }

    void Hockey::Steer() {
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

            if (behind) { keys_.Press(VK_DOWN, common_.GetTime(), 30); }
            else { keys_.Press(VK_UP, common_.GetTime(), 30); }

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

    void Hockey::Move(const Vector2f& target, float target_distance) {
        const Player& bot_player = game_->GetPlayer();
        float distance = bot_player.position.Distance(target);
        Vector2f heading = game_->GetPlayer().GetHeading();

        Vector2f target_position = ctx_.blackboard.ValueOr("target_position", Vector2f());
        float dot = heading.Dot(Normalize(target_position - game_->GetPosition()));



        if (distance > target_distance) {

            steering_.Seek(target);
        }

        else if (distance <= target_distance) {

            Vector2f to_target = target - bot_player.position;

            steering_.Seek(target - Normalize(to_target) * target_distance);
        }
    }


    namespace hz {

        behavior::ExecuteResult FreqWarpAttachNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.hz->GetGame();
           
            //#if 0

            bool x_active = (game.GetPlayer().status & 4) != 0;
            bool stealthing = (game.GetPlayer().status & 1) != 0;
            bool cloaking = (game.GetPlayer().status & 2) != 0;

            bool has_xradar = (game.GetShipSettings().XRadarStatus & 1) != 0;
            bool has_stealth = (game.GetShipSettings().StealthStatus & 1) != 0;
            bool has_cloak = (game.GetShipSettings().CloakStatus & 3) != 0;
#if 0
            //if stealth isnt on but availible, presses home key in continuumgameproxy.cpp
            if (!stealthing && has_stealth && no_spam) {
                game.Stealth();
                ctx.blackboard.Set("SpamCheck", time + 300);
                return behavior::ExecuteResult::Success;
            }
#endif
#if 0
            //same as stealth but presses shift first
            if (!cloaking && has_cloak && no_spam) {
                game.Cloak(ctx.bot->GetKeys());
                ctx.blackboard.Set("SpamCheck", time + 300);
                return behavior::ExecuteResult::Success;
            }
            //#endif
            //#if 0
                        //in deva xradar is free so just turn it on
            if (!x_active && has_xradar && no_spam) {
                game.XRadar();
                ctx.blackboard.Set("SpamCheck", time + 300);
                return behavior::ExecuteResult::Success;
            }
            //#endif
#endif
            return behavior::ExecuteResult::Failure;
        }


        behavior::ExecuteResult FindEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            behavior::ExecuteResult result = behavior::ExecuteResult::Failure;
            
           
            auto& game = ctx.hz->GetGame();
            std::vector< Player > players = game.GetPlayers();
            const Player* target = nullptr;

            const BallData ball = game.GetBallData();

            for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
                const Player& player = players[i];

                if (!IsValidTarget(ctx, player)) continue;
                
                if (ball.held && player.id == ball.last_holder_id) {
                    target = &game.GetPlayers()[i];
                    result = behavior::ExecuteResult::Success;
                }
            }
            
            ctx.blackboard.Set("target_player", target);

            return result;
        }

        bool FindEnemyNode::IsValidTarget(behavior::ExecuteContext& ctx, const Player& target) {
            const auto& game = ctx.hz->GetGame();
            const Player& bot_player = game.GetPlayer();
            
            if (!target.active) return false;
            if (target.id == game.GetPlayer().id) return false;
            if (target.ship > 7) return false;
            if (target.frequency == game.GetPlayer().frequency) return false;
            if (target.name[0] == '<') return false;

            if (game.GetMap().GetTileId(target.position) == marvin::kSafeTileId) {
                return false;
            }

            if (!IsValidPosition(target.position)) {
                return false;
            }

            MapCoord bot_coord(bot_player.position);
            MapCoord target_coord(target.position);

            if (!ctx.hz->GetRegions().IsConnected(bot_coord, target_coord)) {
                return false;
            }

            return true;
        }




        behavior::ExecuteResult PathToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.hz->GetGame();
            std::vector<Vector2f> path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());
            Vector2f bot = game.GetPosition();
            Vector2f enemy = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr)->position;

            path = ctx.com->CreatePath(ctx.hz->GetPathfinder(), path, bot, enemy, game.GetShipSettings().GetRadius());

            ctx.blackboard.Set("path", path);
            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult PatrolNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.hz->GetGame();
            Vector2f from = game.GetPosition();
            
            std::vector<Vector2f> path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());
            const BallData ball = game.GetBallData();
            

            const Vector2f goal_0 = Vector2f(390, 512);
            const Vector2f goal_1 = Vector2f(633, 512);

            if (ball.held && ball.last_holder_id == game.GetPlayer().id) {
#if 0
                if (game.GetPlayer().frequency == 01) {
                    if (ctx.bot->GetRegions().IsConnected(game.GetPosition(), goal_0)) {
                        path = CreatePath(ctx, "path", from, goal_0, game.GetShipSettings().GetRadius());
                    }
                    else path = CreatePath(ctx, "path", from, goal_1, game.GetShipSettings().GetRadius());
                }
                else if (game.GetPlayer().frequency == 00) {
                 
                    if (ctx.bot->GetRegions().IsConnected(game.GetPosition(), goal_1)) {
                        path = CreatePath(ctx, "path", from, goal_1, game.GetShipSettings().GetRadius());
                    }
                    else path = CreatePath(ctx, "path", from, goal_0, game.GetShipSettings().GetRadius());
                }
#endif
                path = ctx.com->CreatePath(ctx.hz->GetPathfinder(), path, from, goal_0, game.GetShipSettings().GetRadius());

                if (game.GetPosition().DistanceSq(goal_1) < 25.0f * 25.0f || game.GetPosition().DistanceSq(goal_0) < 25.0f * 25.0f) {
                    ctx.hz->GetKeys().Press(VK_CONTROL, ctx.com->GetTime(), 50);
                }

            }
            else path = ctx.com->CreatePath(ctx.hz->GetPathfinder(), path, from, ball.position, game.GetShipSettings().GetRadius());
            
            
            
#if 0
            std::vector<Vector2f> nodes{ Vector2f(570, 540), Vector2f(455, 540), Vector2f(455, 500), Vector2f(570, 500) };
            int index = ctx.blackboard.ValueOr<int>("patrol_index", 0);
            Vector2f to = nodes.at(index);

            if (game.GetPosition().DistanceSq(to) < 5.0f * 5.0f) {
                index = (index + 1) % nodes.size();
                ctx.blackboard.Set("patrol_index", index);
                to = nodes.at(index);
            }
#endif
            
                
            ctx.blackboard.Set("path", path);

            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult FollowPathNode::Execute(behavior::ExecuteContext& ctx) {
            auto path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());

            size_t path_size = path.size();
            auto& game = ctx.hz->GetGame();


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

            ctx.hz->Move(current, 0.0f);

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

            if (target == nullptr) return behavior::ExecuteResult::Failure;

            auto& game = ctx.hz->GetGame();


            auto to_target = *target - game.GetPosition();
            CastResult ray_cast = RayCast(game.GetMap(), game.GetPosition(), Normalize(to_target), to_target.Length());

            if (!ray_cast.hit) {
                result = behavior::ExecuteResult::Success;
            }

            return result;
        }




        behavior::ExecuteResult LookingAtEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            const auto target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            if (!target_player) { return behavior::ExecuteResult::Failure; }

            const Player& target = *target_player;
            auto& game = ctx.hz->GetGame();

            const Player& bot_player = game.GetPlayer();

            float target_radius = game.GetSettings().ShipSettings[target.ship].GetRadius();
            float radius = game.GetShipSettings().GetRadius();
            float proj_speed = game.GetSettings().ShipSettings[bot_player.ship].BulletSpeed / 10.0f / 16.0f;

            Vector2f solution;


            if (!ctx.com->CalculateShot(game.GetPosition(), target.position, bot_player.velocity, target.velocity, proj_speed, &solution)) {
                ctx.blackboard.Set<Vector2f>("shot_position", solution);
                ctx.blackboard.Set<bool>("has_shot", false);
                return behavior::ExecuteResult::Failure;
            }

            Vector2f totarget = solution - game.GetPosition();
            RenderLine(GetWindowCenter(), GetWindowCenter() + (Normalize(totarget) * totarget.Length() * 16), RGB(100, 0, 0));

            //Vector2f projectile_trajectory = (bot_player.GetHeading() * proj_speed) + bot_player.velocity;
            //Vector2f projectile_direction = Normalize(projectile_trajectory);

            float radius_multiplier = 1.2f;

            //if the target is cloaking and bot doesnt have xradar make the bot shoot wide
            if (!(game.GetPlayer().status & 4)) {
                if (target.status & 2) {
                    radius_multiplier = 3.0f;
                }
            }

            float nearby_radius = target_radius * radius_multiplier;

            //Vector2f box_min = target.position - Vector2f(nearby_radius, nearby_radius);
            Vector2f box_min = solution - Vector2f(nearby_radius, nearby_radius);
            Vector2f box_extent(nearby_radius * 1.2f, nearby_radius * 1.2f);
            float dist;
            Vector2f norm;
            bool rHit = false;

            if ((game.GetShipSettings().DoubleBarrel & 1) != 0) {
                Vector2f side = Perpendicular(bot_player.GetHeading());

                bool rHit1 = RayBoxIntersect(bot_player.position + side * radius, bot_player.GetHeading(), box_min, box_extent, &dist, &norm);
                bool rHit2 = RayBoxIntersect(bot_player.position - side * radius, bot_player.GetHeading(), box_min, box_extent, &dist, &norm);
                if (rHit1 || rHit2) { rHit = true; }

            }
            else {
                rHit = RayBoxIntersect(bot_player.position, bot_player.GetHeading(), box_min, box_extent, &dist, &norm);
            }

            ctx.blackboard.Set<bool>("has_shot", rHit);
            ctx.blackboard.Set<Vector2f>("shot_position", solution);

            // bool hit = RayBoxIntersect(bot_player.position, projectile_direction, box_min, box_extent, &dist, &norm);


            if (rHit) {
                if (ctx.com->CanShootGun(game.GetMap(), bot_player, target)) {
                    return behavior::ExecuteResult::Success;
                }
            }
            return behavior::ExecuteResult::Failure;
        }

        bool LookingAtEnemyNode::CanShoot(const marvin::Map& map, const marvin::Player& bot_player, const marvin::Player& target) {
            if (bot_player.position.DistanceSq(target.position) > 4 * 4) return false;
            if (map.GetTileId(bot_player.position) == marvin::kSafeTileId) return false;

            return true;
        }




        behavior::ExecuteResult ShootEnemyNode::Execute(behavior::ExecuteContext& ctx) {

            ctx.hz->GetKeys().Press(VK_CONTROL, ctx.com->GetTime(), 50);

            return behavior::ExecuteResult::Success;
        }



        behavior::ExecuteResult MoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.hz->GetGame();

           
            Vector2f target_position = ctx.blackboard.ValueOr("shot_position", Vector2f());
    
            float hover_distance = 1.0f;

            ctx.hz->Move(target_position, hover_distance);

            ctx.hz->GetSteering().Face(target_position);

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
    } //namespace hz
} //namespace marvin