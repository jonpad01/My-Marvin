#include "Devastation.h"

namespace marvin {
	namespace deva {

        behavior::ExecuteResult FreqWarpAttachNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            Area bot_is = ctx.bot->InArea(game.GetPlayer().position);
            uint64_t time = ctx.bot->GetTime();
            uint64_t spam_check = ctx.blackboard.ValueOr<uint64_t>("SpamCheck", 0);
            uint64_t F7_check = ctx.blackboard.ValueOr<uint64_t>("F7", 0);
            bool no_spam = time > spam_check;
            bool no_F7_spam = time > F7_check;

            float deva_energy_pct = (game.GetEnergy() / (float)game.GetShipSettings().MaximumEnergy) * 100.0f;
            Duelers duelers = ctx.bot->FindDuelers();
            int open_freq = ctx.bot->FindOpenFreq();
            bool dueling = game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01;
            bool on_0 = game.GetPlayer().frequency == 00;
            bool on_1 = game.GetPlayer().frequency == 01;

//#if 0
            //if baseduelers are uneven jump in
            if (duelers.freq_0s > duelers.freq_1s) {
                if (!dueling) {
                    if (CheckStatus(ctx) && no_spam) {
                        game.SetFreq(1);
                        ctx.blackboard.Set("SpamCheck", time + 200);
                    }
                    return behavior::ExecuteResult::Success;
                }
                //if on team and uneven jump out
                else if (on_0 && (bot_is.in_center || (duelers.team_in_base || duelers.freq_0s < 2))) {
                    if (CheckStatus(ctx) && no_spam) {
                        game.SetFreq(open_freq);
                        ctx.blackboard.Set("SpamCheck", time + 200);
                    }
                    return behavior::ExecuteResult::Success;
                }
            }
            if (duelers.freq_0s < duelers.freq_1s) {
                if (!dueling) {
                    if (CheckStatus(ctx) && no_spam) {
                        game.SetFreq(0);
                        ctx.blackboard.Set("SpamCheck", time + 200);
                    }
                    return behavior::ExecuteResult::Success;
                }
                else if (on_1 && (bot_is.in_center || (duelers.team_in_base || duelers.freq_1s < 2))) {
                    if (CheckStatus(ctx) && no_spam) {
                        game.SetFreq(open_freq);
                        ctx.blackboard.Set("SpamCheck", time + 200);
                    }
                    return behavior::ExecuteResult::Success;
                }
            }
//#endif
            //if dueling then attach
            if (dueling) {
              
                if (bot_is.in_center && duelers.team.size() != 0 && duelers.team_in_base) {

                    //a saved value to keep the ticker moving up or down
                    bool up_down = ctx.blackboard.ValueOr<bool>("UpDown", true);
                    //used to skip f7 and make attaching more random
                    bool page_or_f7 = ctx.blackboard.ValueOr<bool>("PageOrF7", false);
                    //what player the ticker is currently on
                    const Player& selected_player = game.GetSelectedPlayer();

                    //if ticker is on a player in base attach
                    for (std::size_t i = 0; i < duelers.team.size(); i++) {
                        const Player& player = duelers.team[i];
                        Area player_is = ctx.bot->InArea(player.position);
                        if (!player_is.in_center && player.ship < 8 && player.id == selected_player.id && !page_or_f7/*IsValidPosition(player.position)*/) {
                            if (no_F7_spam) {
                                if (CheckStatus(ctx)) {
                                    game.F7();
                                    ctx.blackboard.Set("PageOrF7", true);
                                    ctx.blackboard.Set("F7", time + 150);
                                }
                            }
                            return behavior::ExecuteResult::Success;   
                        }
                    }

                    //if the ticker is not on a player in base then move the ticker for the next check
                    if (up_down) game.PageUp();
                    else game.PageDown();

                    if (page_or_f7) ctx.blackboard.Set("PageOrF7", false);
                    else ctx.blackboard.Set("PageOrF7", true);

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
                for (std::size_t i = 0; i < duelers.team.size(); i++) {
                    const Player& player = duelers.team[i];
                    if (player.position == bot_position && no_spam) {
                        game.F7();
                        ctx.blackboard.Set("SpamCheck", time + 150);
                        //return behavior::ExecuteResult::Success;
                    }
                } 
            }

            bool x_active = (game.GetPlayer().status & 4) != 0; 
            bool stealthing = (game.GetPlayer().status & 1) != 0;
            bool cloaking = (game.GetPlayer().status & 2) != 0;

            bool has_xradar = (game.GetShipSettings().XRadarStatus & 3) != 0;
            bool has_stealth = (game.GetShipSettings().StealthStatus & 1) != 0;
            bool has_cloak = (game.GetShipSettings().CloakStatus & 3) != 0;

            //if stealth isnt on but availible, presses home key in continuumgameproxy.cpp
            if (!stealthing && has_stealth && no_spam) {
                game.Stealth();
                ctx.blackboard.Set("SpamCheck", time + 300);
                return behavior::ExecuteResult::Success;
            }
            //same as stealth but presses shift first
            if (!cloaking && has_cloak && no_spam) {
                game.Cloak(ctx.bot->GetKeys());
                ctx.blackboard.Set("SpamCheck", time + 300);
                return behavior::ExecuteResult::Success;
            }
            //in deva xradar is free so just turn it on
            if (!x_active && has_xradar && no_spam) {
                    game.XRadar();
                    ctx.blackboard.Set("SpamCheck", time + 300);
                    return behavior::ExecuteResult::Success;
            }
            return behavior::ExecuteResult::Failure;
        }

        bool FreqWarpAttachNode::CheckStatus(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            float energy_pct = (game.GetEnergy() / (float)game.GetShipSettings().MaximumEnergy) * 100.0f;
            bool result = false;
            if ((game.GetPlayer().status & 2) != 0) game.Cloak(ctx.bot->GetKeys());
            //if ((game.GetPlayer().status & 1) != 0) game.Stealth();
            if (energy_pct == 100.0f) result = true;
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
            float energy_pct = (game.GetEnergy() / (float)game.GetShipSettings().MaximumEnergy) * 100.0f;
            Area bot_is = ctx.bot->InArea(game.GetPosition());
            if (energy_pct < 25.0f && bot_is.in_center) game.Repel(ctx.bot->GetKeys());

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
            Area bot_is = ctx.bot->InArea(from);
            Duelers enemy = ctx.bot->FindDuelers();
            std::vector<Vector2f> nodes;
            int index = ctx.blackboard.ValueOr<int>("patrol_index", 0);
            int taunt_index = ctx.blackboard.ValueOr<int>("taunt_index", 0);
            uint64_t time = ctx.bot->GetTime();
            uint64_t hoorah_expire = ctx.blackboard.ValueOr<uint64_t>("HoorahExpire", 0);
            bool in_safe = game.GetMap().GetTileId(game.GetPosition()) == kSafeTileId;

            if (bot_is.in_center) {
                nodes = { Vector2f(568, 568), Vector2f(454, 568), Vector2f(454, 454), Vector2f(568, 454), Vector2f(568, 568),
                Vector2f(454, 454), Vector2f(568, 454), Vector2f(454, 568), Vector2f(454, 454), Vector2f(568, 568), Vector2f(454, 568), Vector2f(568, 454) };
            }
            else {
                if (time > hoorah_expire && !enemy.enemy_in_base && !in_safe) {
                    std::vector<std::string> taunts = { "we gotem", "man, im glad im not on that freq", "high score is that bad",
                    "kill all humans", "that team is almost as bad as Daman24/7 is", "robots unite", "this is all thanks to my high quality attaching skills" };
                    std::string taunt = taunts.at(taunt_index);
                    game.SendChatMessage(taunt);
                    taunt_index = (taunt_index + 1) % taunts.size();
                    ctx.blackboard.Set("taunt_index", taunt_index);
                    ctx.blackboard.Set("HoorahExpire", time + 600000);
                }
            
                return behavior::ExecuteResult::Failure;
            }
          

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

            while (path.size() > 1 && CanMoveBetween(game, game.GetPosition(), path.at(1))) { //while
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
                //if bot is not in center, almost dead, and in line of sight of its target burst
                Area bot_is = ctx.bot->InArea(ctx.bot->GetGame().GetPlayer().position);
                float energy_pct = ctx.bot->GetGame().GetEnergy() / (float)ctx.bot->GetGame().GetShipSettings().MaximumEnergy;
                if (energy_pct < 0.10f && !bot_is.in_center) {
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
            float radius_multiplier = 1.1f;

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
            int ship = ctx.bot->GetGame().GetPlayer().ship;
            if (ship == 3 || ship == 6) ctx.bot->GetKeys().Press(VK_TAB);
            else ctx.bot->GetKeys().Press(VK_CONTROL);

            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult MoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            
            Area bot_is = ctx.bot->InArea(ctx.bot->GetGame().GetPlayer().position);
            
            float energy_pct = game.GetEnergy() / (float)game.GetShipSettings().MaximumEnergy;
            const Player& player = *ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
            float player_energy_pct = player.energy / (float)game.GetShipSettings().MaximumEnergy;
            
            float energy_check = ctx.blackboard.ValueOr<float>("EnergyCheck", 0);
            bool emped = ctx.blackboard.ValueOr<bool>("Emped", false);
            
            uint64_t time = ctx.bot->GetTime();
            uint64_t energy_time_check = ctx.blackboard.ValueOr<uint64_t>("EnergyTimeCheck", 0);
            uint64_t emp_cooldown_check = ctx.blackboard.ValueOr<uint64_t>("EmpCooldown", 0);
            uint64_t repel_cooldown = ctx.blackboard.ValueOr<uint64_t>("RepelCooldown", 0);
            uint64_t repel_check = ctx.blackboard.ValueOr<uint64_t>("RepelCheck", 0);

            bool in_safe = game.GetMap().GetTileId(game.GetPlayer().position) == marvin::kSafeTileId;
            Vector2f target_position = ctx.blackboard.ValueOr("target_position", Vector2f());

            /*monkey> no i dont have a list of the types
monkey> i think it's some kind of combined thing with weapon flags in it
monkey> im pretty sure type & 0x01 is whether or not it's bouncing
monkey> and the 0x8000 is the alternative bit for things like mines and multifire
monkey> i was using type & 0x8F00 to see if it was a mine and it worked decently well
monkey> 0x0F00 worked ok for seeing if its a bomb
monkey> and 0x00F0 for bullet, but i don't think it's exact*/

            /*
            0x0000 = nothing
            0x1000 & 0x0F00 & 0x1100 & 0x1800 & 0x1F00 & 0x0001 = mine or bomb
            0x0800 = emp mine or bomb
            0x8000 & 0x8100 = mine or multifire
            0x8001 = multi mine bomb
            0xF000 & 0x8800 & 0x8F00 & 0xF100 & 0xF800 & 0xFF00 = mine multi bomb
            0x000F & 0x00F0 = any weapon

            stopped at 0x0F01
            */
            
//#if 0
            //weapon type sees mines and bombs as same so this checks if the bomb is in the same position
            //so it only triggers for mines
            Vector2f weapon_position = ctx.blackboard.ValueOr<Vector2f>("WeaponPosition", Vector2f());
            const Player* weapon_player_ = ctx.blackboard.ValueOr<const Player*>("WeaponPlayer", nullptr);

            if (time > repel_check) {
                for (Weapon* weapon : game.GetWeapons()) {
                    const Player* weapon_player = game.GetPlayerById(weapon->GetPlayerId());
                    if (weapon_player == nullptr) continue;
                    if (weapon_player->frequency == game.GetPlayer().frequency) continue;
                    if (weapon->GetType() & 0x8000) {
                        ctx.blackboard.Set("WeaponPosition", weapon->GetPosition());
                        ctx.blackboard.Set("WeaponPlayer", weapon_player);
                        if (weapon_position == weapon->GetPosition() && weapon_player_ == weapon_player) {
                            //when it gets hit with a bomb its a false trigger so check against bots position
                            if (weapon_position.Distance(game.GetPosition()) < 8.0f) {
                                //if (time > repel_cooldown) {
                                    game.Repel(ctx.bot->GetKeys());
                                    //ctx.bot->GetKeys().Press(VK_TAB);
                                   // ctx.blackboard.Set("RepelCooldown", time + 50);
                                   // return behavior::ExecuteResult::Success;
                                //}
                            }
                        }
                    }
                }
                ctx.blackboard.Set("RepelCheck", time + 100);
            }
//#endif
            float hover_distance = 0.0f;
            if (time > energy_time_check) {
                float energy = (float)game.GetEnergy();
                ctx.blackboard.Set("EnergyTimeCheck", time + 25);
                ctx.blackboard.Set("EnergyCheck", energy);
                if (energy == energy_check && energy_pct < 0.95f) {
                    ctx.blackboard.Set("Emped", true);
                    ctx.blackboard.Set("EmpCooldown", time + 1000);
                }
            }
            if (emped && time > emp_cooldown_check) {
                ctx.blackboard.Set("Emped", 0);
            }

            if (in_safe) hover_distance = 0.0f;
            else {
                if (bot_is.in_center) {
                    if (emped) {
                        hover_distance = 30.0f;
                    }
                    else {
                        float diff = (energy_pct - player_energy_pct) * 20.0f;
                        hover_distance = 10.0f - diff;
                        if (hover_distance < 0.0f) hover_distance = 0.0f;
                    }
                }
                else {
                    hover_distance = 3.0f;
                }
            }

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
	} //namespace deva
} //namespace marvin