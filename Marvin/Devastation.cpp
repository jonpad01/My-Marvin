#include "Devastation.h"

namespace marvin {
	namespace deva {


        behavior::ExecuteResult FindEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            behavior::ExecuteResult result = behavior::ExecuteResult::Failure;
            float closest_cost = std::numeric_limits<float>::max();
            float cost = 0;
            auto& game = ctx.bot->GetGame();
            std::vector< Player > players = game.GetPlayers();
            const Player* target = nullptr;
            const Player& bot = ctx.bot->GetGame().GetPlayer();

            Area bot_is = ctx.bot->InArea(bot.position);
            Vector2f position = game.GetPosition();
            Vector2f resolution(1920, 1080);
            view_min_ = bot.position - resolution / 2.0f / 16.0f;
            view_max_ = bot.position + resolution / 2.0f / 16.0f;

            //this loop checks every player and finds the closest one based on a cost formula
            //if this does not succeed then there is no target, it will go into patrol mode
            for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
                const Player& player = players[i];

                if (!IsValidTarget(ctx, player)) continue;
                auto to_target = player.position - position;
                CastResult in_sight = RayCast(game.GetMap(), game.GetPosition(), Normalize(to_target), to_target.Length());

                //make players in line of sight high priority
                //TODO: add flaggers switch targets if enemy gets close to anchor
                if (!in_sight.hit) {
                    cost = CalculateCost(game, bot, player) / 1000;
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

            if (current_target != target) {
                ctx.blackboard.Set("aggression", 0.0f);
            }
            //and this sets the current_target for the next loop
            ctx.blackboard.Set("target_player", target);
            const Player* r =
                ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);


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
            const auto& game = ctx.bot->GetGame();
            const Player& bot_player = game.GetPlayer();

            if (target.energy < 0.0f) return false;
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
            //TODO: check if player is cloaking and outside radar range
            //1 = stealth, 2= cloak, 3 = both, 4 = xradar 
            bool stealthing = (target.status & 1) != 0;
            bool cloaking = (target.status & 2) != 0;

            //if the bot doesnt have xradar
            if (!(game.GetPlayer().status & 4)) {
                if (stealthing && cloaking) return false;

                bool visible = InRect(target.position, view_min_, view_max_);

                if (stealthing && !visible) return false;
            }

            return true;
        }




        behavior::ExecuteResult PathToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();

            Vector2f bot = game.GetPosition();
            Vector2f enemy = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr)->position;

            Path path = CreatePath(ctx, "path", bot, enemy, game.GetShipSettings().GetRadius());

            ctx.blackboard.Set("path", path);
            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult PatrolNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            const Player& bot = game.GetPlayer();
            Vector2f from = game.GetPosition();
            Path path = ctx.blackboard.ValueOr("path", Path());

            auto nodes = ctx.blackboard.ValueOr("patrol_nodes", std::vector<Vector2f>());
            auto index = ctx.blackboard.ValueOr("patrol_index", 0);

            if (nodes.empty()) {
                return behavior::ExecuteResult::Failure;
            }

            Vector2f to = nodes.at(index);

            if (game.GetPosition().DistanceSq(to) < 3.0f * 3.0f) {
                index = (index + 1) % nodes.size();
                ctx.blackboard.Set("patrol_index", index);
                to = nodes.at(index);
            }

            path = CreatePath(ctx, "path", from, to, game.GetShipSettings().GetRadius());
            ctx.blackboard.Set("path", path);

            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult FollowPathNode::Execute(behavior::ExecuteContext& ctx) {
            auto path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());

            size_t path_size = path.size();
            auto& game = ctx.bot->GetGame();


            if (path.empty()) return behavior::ExecuteResult::Failure;

            Vector2f current = path.front();

            while (path.size() > 1 && CanMoveBetween(game, game.GetPosition(), path.at(1))) {  //while
                path.erase(path.begin());
                current = path.front();
            }

            if (path.size() == 1 &&
                path.front().DistanceSq(game.GetPosition()) < 2 * 2) {
                path.clear();
            }

            if (path.size() != path_size) {
                ctx.blackboard.Set("path", path); //"path"
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
                //if bot is not in center, almost dead, and in line of sight of its target burst
                Area bot_is = ctx.bot->InArea(ctx.bot->GetGame().GetPlayer().position);
                float energy_pct = ctx.bot->GetGame().GetEnergy() / (float)ctx.bot->GetGame().GetShipSettings().InitialEnergy;
                if (energy_pct < 0.10f && !bot_is.in_deva_center) {
                    ctx.bot->GetGame().Burst(ctx.bot->GetKeys());
                }
            }

            return result;
        }




        behavior::ExecuteResult LookingAtEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            const auto target_player =
                ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

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




        behavior::ExecuteResult ShootEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            ctx.bot->GetKeys().Press(VK_CONTROL);
            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult BundleShots::Execute(behavior::ExecuteContext& ctx) {
            const uint64_t cooldown = 50000000000000;
            const uint64_t duration = 1500;

            uint64_t bundle_expire = ctx.blackboard.ValueOr<uint64_t>("BundleCooldownExpire", 0);
            uint64_t current_time = ctx.bot->GetTime();

            if (current_time < bundle_expire) return behavior::ExecuteResult::Failure;

            if (running_ && current_time >= start_time_ + duration) {
                float energy_pct =
                    ctx.bot->GetGame().GetEnergy() /
                    (float)ctx.bot->GetGame().GetShipSettings().InitialEnergy;
                uint64_t expiration = current_time + cooldown + (rand() % 1000);

                expiration -= (uint64_t)((cooldown / 2ULL) * energy_pct);

                ctx.blackboard.Set("BundleCooldownExpire", expiration);
                running_ = false;
                return behavior::ExecuteResult::Success;
            }

            auto& game = ctx.bot->GetGame();

            const Player& target =
                *ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            if (!running_) {
                if (!ShouldActivate(ctx, target)) {
                    return behavior::ExecuteResult::Failure;
                }

                start_time_ = current_time;
                running_ = true;
            }

            RenderText("BundleShots", GetWindowCenter() + Vector2f(0, 100), RGB(100, 100, 100), RenderText_Centered);
            ctx.bot->Move(target.position, 0.0f);
            ctx.bot->GetSteering().Face(target.position);

            ctx.bot->GetKeys().Press(VK_UP);
            ctx.bot->GetKeys().Press(VK_CONTROL);

            return behavior::ExecuteResult::Running;
        }

        bool BundleShots::ShouldActivate(behavior::ExecuteContext& ctx, const Player& target) {
            auto& game = ctx.bot->GetGame();
            Vector2f position = game.GetPosition();
            Vector2f velocity = game.GetPlayer().velocity;
            Vector2f relative_velocity = velocity - target.velocity;
            float dot = game.GetPlayer().GetHeading().Dot(
                Normalize(target.position - position));

            if (game.GetMap().GetTileId(game.GetPosition()) == kSafeTileId) {
                return false;
            }

            if (dot <= 0.7f) {
                return false;
            }

            if (relative_velocity.LengthSq() < 1.0f * 1.0f) {
                return true;
            }

            // Activate this if pointing near the target and moving away from them.
            if (game.GetPlayer().velocity.Dot(target.position - position) >= 0.0f) {
                return false;
            }

            return true;
        }




        behavior::ExecuteResult MoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();

            Area bot_is = ctx.bot->InArea(ctx.bot->GetGame().GetPlayer().position);
            float energy_pct = game.GetEnergy() / (float)game.GetShipSettings().InitialEnergy;

            Vector2f target_position = ctx.blackboard.ValueOr("target_position", Vector2f());
            Vector2f heading = game.GetPlayer().GetHeading();
            float dot = heading.Dot(Normalize(target_position - game.GetPosition()));
            int ship = game.GetPlayer().ship;
            float hover_distance = 0.0f;

            if (bot_is.in_deva_center) {
                hover_distance = 12.0f;
                if (dot >= 0.80f) {
                    hover_distance = 2.0f;
                }
            }
            else {
                hover_distance = 2.0f;
            }

            MapCoord spawn =
                ctx.bot->GetGame().GetSettings().SpawnSettings[0].GetCoord();

            if (Vector2f(spawn.x, spawn.y)
                .DistanceSq(ctx.bot->GetGame().GetPosition()) < 35.0f * 35.0f) {
                hover_distance = 0.0f;
            }

            const Player& shooter = *ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            ctx.bot->Move(target_position, hover_distance);

            //ctx.bot->GetSteering().Face(target_position);

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
	} //namespace deva
} //namespace marvin