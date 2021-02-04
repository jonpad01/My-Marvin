#include "Hockey.h"

namespace marvin {
    namespace hz {

        behavior::ExecuteResult FreqWarpAttachNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
           
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
            
           
            auto& game = ctx.bot->GetGame();
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
            const auto& game = ctx.bot->GetGame();
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

            if (!ctx.bot->GetRegions().IsConnected(bot_coord, target_coord)) {
                return false;
            }

            return true;
        }




        behavior::ExecuteResult PathToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();

            Vector2f bot = game.GetPosition();
            Vector2f enemy = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr)->position;

            Path path = ctx.bot->CreatePath(ctx, "path", bot, enemy, game.GetShipSettings().GetRadius());

            ctx.blackboard.Set("path", path);
            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult PatrolNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            Vector2f from = game.GetPosition();
            
            Path path;
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
                path = ctx.bot->CreatePath(ctx, "path", from, goal_0, game.GetShipSettings().GetRadius());

                if (game.GetPosition().DistanceSq(goal_1) < 25.0f * 25.0f || game.GetPosition().DistanceSq(goal_0) < 25.0f * 25.0f) {
                    ctx.bot->GetKeys().Press(VK_CONTROL, ctx.bot->GetTime(), 50);
                }

            }
            else path = ctx.bot->CreatePath(ctx, "path", from, ball.position, game.GetShipSettings().GetRadius());
            
            
            
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
            auto& game = ctx.bot->GetGame();


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

            ctx.bot->Move(current, 0.0f);

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

            auto& game = ctx.bot->GetGame();


            auto to_target = *target - game.GetPosition();
            CastResult ray_cast = RayCast(game.GetMap(), game.GetPosition(), Normalize(to_target), to_target.Length());

            if (!ray_cast.hit) {
                result = behavior::ExecuteResult::Success;
            }

            return result;
        }




        behavior::ExecuteResult LookingAtEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            const auto target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            if (target_player == nullptr) return behavior::ExecuteResult::Failure;

            const Player& target = *target_player;
            auto& game = ctx.bot->GetGame();
            const Player& bot_player = game.GetPlayer();

            float proj_speed =
                game.GetSettings().ShipSettings[bot_player.ship].BulletSpeed / 10.0f /
                16.0f;

            Vector2f target_pos = target.position;

            Vector2f seek_position =
                CalculateShot(game.GetPosition(), target_pos, bot_player.velocity,
                    target.velocity, proj_speed);

            Vector2f projectile_trajectory =
                (bot_player.GetHeading() * proj_speed) + bot_player.velocity;

            Vector2f projectile_direction = Normalize(projectile_trajectory);
            float target_radius =
                game.GetSettings().ShipSettings[target.ship].GetRadius();

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
                box_min = seek_position - Vector2f(nearby_radius, nearby_radius);
                hit = RayBoxIntersect(bot_player.position, bot_player.GetHeading(),
                    box_min, box_extent, &dist, &norm);
            }

            if (seek_position.DistanceSq(target_player->position) < 15 * 15) {
                ctx.blackboard.Set("target_position", seek_position);
            }
            else {
                ctx.blackboard.Set("target_position", target.position);
            }

            if (hit) {
                if (CanShoot(game.GetMap(), bot_player, target)) {
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

            ctx.bot->GetKeys().Press(VK_CONTROL, ctx.bot->GetTime(), 50);

            return behavior::ExecuteResult::Success;
        }



        behavior::ExecuteResult MoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();

           
            Vector2f target_position = ctx.blackboard.ValueOr("target_position", Vector2f());
    
            float hover_distance = 1.0f;

            ctx.bot->Move(target_position, hover_distance);

            ctx.bot->GetSteering().Face(target_position);

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