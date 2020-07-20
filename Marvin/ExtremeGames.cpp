#include "ExtremeGames.h"

namespace marvin {
    namespace eg {

        behavior::ExecuteResult FreqWarpAttachNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            Area bot_is = ctx.bot->InArea(game.GetPlayer().position);

            uint64_t time = ctx.bot->GetTime();
            uint64_t spam_check = ctx.blackboard.ValueOr<uint64_t>("SpamCheck", 0);
            uint64_t f7_check = ctx.blackboard.ValueOr<uint64_t>("F7SpamCheck", 0);
            uint64_t chat_check = ctx.blackboard.ValueOr<uint64_t>("ChatWait", 0);
            uint64_t p_check = ctx.blackboard.ValueOr<uint64_t>("!PCheck", 0);
            bool no_spam = time > spam_check;
            bool chat_wait = time > chat_check;
            bool no_p = time > p_check;
            bool no_f7_spam = time > f7_check;

            std::vector<Player> duelers;
            std::vector<Player> players = game.GetPlayers();
            bool team_in_base = false;
            
            //find and store attachable team players
            for (std::size_t i = 0; i < players.size(); i++) {
                const Player& player = players[i];
                Area player_is = ctx.bot->InArea(player.position);
                bool same_team = player.frequency == game.GetPlayer().frequency;
                bool not_bot = player.id != game.GetPlayer().id;
                if (!player_is.in_center && same_team && not_bot && player.ship < 8) {
                    duelers.push_back(players[i]);
                    team_in_base = true;
                }
            }

            //read private messages for !p and send the same message to join a duel team
            std::vector<std::string> chat = game.GetChat(5);

                for (std::size_t i = 0; i < chat.size(); i++) {
                    if (chat[i] == "!p" && no_p) {
                        game.P();
                        ctx.blackboard.Set("!PCheck", time + 3000);
                        ctx.blackboard.Set("ChatWait", time + 200);
                        return behavior::ExecuteResult::Success;
                    }
                    else if (chat[i] == "!l" && no_p) {
                        game.L();
                        ctx.blackboard.Set("!PCheck", time + 3000);
                        ctx.blackboard.Set("ChatWait", time + 200);
                        return behavior::ExecuteResult::Success;
                    }
                    else if (chat[i] == "!r" && no_p) {
                        game.R();
                        ctx.blackboard.Set("!PCheck", time + 3000);
                        ctx.blackboard.Set("ChatWait", time + 200);
                        return behavior::ExecuteResult::Success;
                    }
                }
                //bot needs to halt so eg can process chat input, i guess
                if (!chat_wait) {
                    return behavior::ExecuteResult::Success;
                }

                
                bool dueling = game.GetPlayer().frequency != 00 && game.GetPlayer().frequency != 01;
            
            // attach code
                //the region registry doesnt thing the eg center bases are connected to center 
            if (bot_is.in_center && duelers.size() != 0 && team_in_base && dueling) {

                //a saved value to keep the ticker moving up or down
                bool up_down = ctx.blackboard.ValueOr<bool>("UpDown", true);
                
                //what player the ticker is currently on
                const Player& selected_player = game.GetSelectedPlayer();

                //if ticker is on a player in base attach
                for (std::size_t i = 0; i < duelers.size(); i++) {
                    const Player& player = duelers[i];
                    Area player_is = ctx.bot->InArea(player.position);
                    if (player.id == selected_player.id && IsValidPosition(player.position)) {
                        if (CheckStatus(ctx)) game.F7();
                        return behavior::ExecuteResult::Success;
                    }
                }

                //if the ticker is not on a player in base then move the ticker for the next check
                if (up_down) game.PageUp();
                else game.PageDown();

                //find the ticker position
                int64_t ticker = game.TickerPosition();
                //if the ticker has reached the end of the list switch direction
                if (ticker <= 0) ctx.blackboard.Set("UpDown", false);
                else if (ticker >= game.GetPlayers().size() - 1) ctx.blackboard.Set("UpDown", true);

                return behavior::ExecuteResult::Success;
            }

                    Vector2f bot_position = game.GetPlayer().position;
                    bool in_safe = game.GetMap().GetTileId(bot_position) == kSafeTileId;
                    //try to detach
                    for (std::size_t i = 0; i < duelers.size(); i++) {
                        const Player& player = duelers[i];
                        if (player.position == bot_position && no_f7_spam && !bot_is.in_center) {
                            game.F7();
                            ctx.blackboard.Set("F7SpamCheck", time + 150);
                            return behavior::ExecuteResult::Success;
                        }
                    }
        
            
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

        bool FreqWarpAttachNode::CheckStatus(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            float energy_pct = (game.GetEnergy() / (float)game.GetShipSettings().InitialEnergy) * 100.0f;
            bool result = false;
            if ((game.GetPlayer().status & 2) != 0) {
                game.Cloak(ctx.bot->GetKeys());
                return false;
            }
            if ((game.GetPlayer().status & 1) != 0) {
                game.Stealth();
                return false;
            }
            if (energy_pct >= 100.0f) result = true;
            return result;
        }


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
            const auto& game = ctx.bot->GetGame();
            const Player& bot_player = game.GetPlayer();
            //Area bot_is = ctx.bot->InArea(bot_player.position);

            //if (target.dead && !bot_is.in_center) return false;
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
            Vector2f from = game.GetPosition();
            Path path = ctx.blackboard.ValueOr("path", Path());
            std::vector<Vector2f> nodes { Vector2f(570, 540), Vector2f(455, 540), Vector2f(455, 500), Vector2f(570, 500) };
            int index = ctx.blackboard.ValueOr<int>("patrol_index", 0);
            
            Vector2f to = nodes.at(index);

            if (game.GetPosition().DistanceSq(to) < 5.0f * 5.0f) {
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
            if (bot_player.position.DistanceSq(target.position) > 60 * 60) return false;
            if (map.GetTileId(bot_player.position) == marvin::kSafeTileId) return false;

            return true;
        }




        behavior::ExecuteResult ShootEnemyNode::Execute(behavior::ExecuteContext& ctx) {
           
            const Player& target_player = *ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
           
            if (ctx.bot->GetGame().GetPosition().DistanceSq(target_player.position) > 18 * 18) {
                ctx.bot->GetKeys().Press(VK_TAB);
            }
            else ctx.bot->GetKeys().Press(VK_CONTROL);

            return behavior::ExecuteResult::Success;
        }



        behavior::ExecuteResult MoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            
            float energy_pct = game.GetEnergy() / (float)game.GetShipSettings().InitialEnergy;
            
            Area bot_is = ctx.bot->InArea(game.GetPosition());
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
                    game.Repel(ctx.bot->GetKeys());
                }
            }
            //#endif
            float hover_distance = 0.0f;
         

            if (in_safe) hover_distance = 0.0f;
            else if (bot_is.in_center) {
                hover_distance = 10.0f / energy_pct;  
            }
            else {
                hover_distance = 3.0f / energy_pct;
            }
            if (hover_distance < 0.0f) hover_distance = 0.0f;
            const Player& shooter = *ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

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
    } //namespace eg
} //namespace marvin