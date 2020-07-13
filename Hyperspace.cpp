#include "Hyperspace.h"

namespace marvin {
	namespace hs {

        /*monkey> no i dont have a list of the types
monkey> i think it's some kind of combined thing with weapon flags in it
monkey> im pretty sure type & 0x01 is whether or not it's bouncing
monkey> and the 0x8000 is the alternative bit for things like mines and multifire
monkey> i was using type & 0x8F00 to see if it was a mine and it worked decently well
monkey> 0x0F00 worked ok for seeing if its a bomb
monkey> and 0x00F0 for bullet, but i don't think it's exact*/

        behavior::ExecuteResult FreqWarpAttachNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            
            Flaggers flaggers = ctx.bot->FindFlaggers();
            const Player& enemy_anchor = flaggers.enemy_anchor, team_anchor = flaggers.team_anchor;
            
            Area bot_is = ctx.bot->InArea(game.GetPlayer().position);
            Area enemy_anchor_is = ctx.bot->InArea(enemy_anchor.position);
            Area team_anchor_is = ctx.bot->InArea(team_anchor.position);
            
            bool team_anchor_in_safe = game.GetMap().GetTileId(flaggers.team_anchor.position) == kSafeTileId;
            bool in_safe = game.GetMap().GetTileId(game.GetPosition()) == kSafeTileId;
            float energy_pct = (game.GetEnergy() / (float)game.GetShipSettings().InitialEnergy) * 100.0f;
            bool respawned = in_safe && bot_is.in_center && energy_pct == 100.0f;
            
            uint64_t time = ctx.bot->GetTime();
            uint64_t spam_check = ctx.blackboard.ValueOr<uint64_t>("SpamCheck", 0);
            uint64_t flash_check = ctx.blackboard.ValueOr<uint64_t>("FlashCoolDown", 0);
            bool no_spam = time > spam_check;
            bool flash_cooldown = time > flash_check;
            
            bool flagging = game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91;
            bool in_lanc = game.GetPlayer().ship == 6; bool in_spid = game.GetPlayer().ship == 2;
            bool anchor = flagging && in_lanc;
            
            
                if (bot_is.in_base && enemy_anchor_is.in_diff) {
                    if (ctx.bot->CheckStatus()) game.Warp();
                    return behavior::ExecuteResult::Success;
                }
                //checkstatus turns off stealth and cloak before warping
                if (!bot_is.in_center && !flagging) {
                    if (ctx.bot->CheckStatus()) game.Warp();
                    return behavior::ExecuteResult::Success;
                }
                //join a flag team, spams too much without the timer
                if (flaggers.flaggers < 14 && !flagging && respawned && no_spam) {
                    if (ctx.bot->CheckStatus()) game.Flag();
                    ctx.blackboard.Set("SpamCheck", time + 200);
                }
                //switch to lanc if team doesnt have one
                if (flagging && flaggers.team_lancs == 0 && !in_lanc && respawned && no_spam) {
                    //try to remember what ship it was in
                    ctx.blackboard.Set<int>("last_ship", (int)game.GetPlayer().ship);
                    if (ctx.bot->CheckStatus()) game.SetShip(6);
                    ctx.blackboard.Set("SpamCheck", time + 200);
                }
                //if there is more than one lanc on the team, switch back to original ship
                if (flagging && flaggers.team_lancs > 1 && in_lanc && no_spam) {
                    if (team_anchor_is.in_base || !bot_is.in_base) {
                        int ship = ctx.blackboard.ValueOr<int>("last_ship", 0);
                        if (ctx.bot->CheckStatus()) game.SetShip(ship);
                        ctx.blackboard.Set("SpamCheck", time + 200);
                        return behavior::ExecuteResult::Success;
                    }
                }

                //switch to spider if team doesnt have one
                if (flagging && flaggers.team_spiders == 0 && !in_spid && respawned && no_spam) {
                    //try to remember what ship it was in
                    ctx.blackboard.Set<int>("last_ship", (int)game.GetPlayer().ship);
                    if (ctx.bot->CheckStatus()) game.SetShip(2);
                    ctx.blackboard.Set("SpamCheck", time + 200);
                }
                //if there is more than two spiders on the team, switch back to original ship
                if (flagging && flaggers.team_spiders > 2 && in_spid && respawned && no_spam) {
                        int ship = ctx.blackboard.ValueOr<int>("last_ship", 0);
                        if (ctx.bot->CheckStatus()) game.SetShip(ship);
                        ctx.blackboard.Set("SpamCheck", time + 200);
                        return behavior::ExecuteResult::Success;      
                }

                //leave a flag team by spectating, will reenter ship on the next update
                if (flaggers.flaggers > 16 && flagging && !in_lanc && respawned && no_spam) {
                    int freq = ctx.bot->FindOpenFreq();
                    if (ctx.bot->CheckStatus()) game.SetFreq(freq);// game.Spec();
                    ctx.blackboard.Set("SpamCheck", time + 200);
                }

                //if flagging try to attach
                if (flagging && !team_anchor_in_safe && !in_lanc && no_spam) {
                    uint64_t last_target = ctx.blackboard.ValueOr<uint64_t>("LastTarget", 0);
                    uint64_t target = flaggers.team_anchor.id;
                    bool same_target = target == last_target;
                    //all ships will attach after respawning, all but lancs will attach if not in the same base
                    if (respawned || (!team_anchor_is.connected && (same_target || flaggers.team_lancs < 2))) {
                        if (ctx.bot->CheckStatus()) game.Attach(flaggers.team_anchor.name);
                        ctx.blackboard.Set("SpamCheck", time + 200);
                        ctx.blackboard.Set("LastTarget", target);
                        return behavior::ExecuteResult::Success;
                    }
                }
            
            
            
            
            
            bool x_active = (game.GetPlayer().status & 4) != 0;
            bool has_xradar = (game.GetShipSettings().XRadarStatus & 1);
            bool stealthing = (game.GetPlayer().status & 1) != 0;
            bool cloaking = (game.GetPlayer().status & 2) != 0;
            bool has_stealth = (game.GetShipSettings().StealthStatus & 2);
            bool has_cloak = (game.GetShipSettings().CloakStatus & 2);

            //if stealth isnt on but availible, presses home key in continuumgameproxy.cpp
            if (!stealthing && has_stealth && no_spam) {
                game.Stealth();
                ctx.blackboard.Set("SpamCheck", time + 500);
                return behavior::ExecuteResult::Success;
            }
            //same as stealth but presses shift first
            if ((!cloaking && has_cloak && no_spam) || (anchor && flash_cooldown)) {
                game.Cloak(ctx.bot->GetKeys());
                ctx.blackboard.Set("SpamCheck", time + 500);
                ctx.blackboard.Set("FlashCoolDown", time + 30000);
                return behavior::ExecuteResult::Success;
            }
            //TODO: make this better
           // if (!x_active && has_xradar && no_spam) {
                 //   game.XRadar();
                 //    ctx.blackboard.Set("SpamCheck", time + 300);
                    //return behavior::ExecuteResult::Success;
           // }

            return behavior::ExecuteResult::Failure;
        }

        behavior::ExecuteResult FindEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            behavior::ExecuteResult result = behavior::ExecuteResult::Failure;
            float closest_cost = std::numeric_limits<float>::max();
            float cost = 0.0f;
            auto& game = ctx.bot->GetGame();
            Flaggers flaggers = ctx.bot->FindFlaggers();
            const Player& team_anchor = flaggers.team_anchor;
            std::vector< Player > players = game.GetPlayers();
            const Player* target = nullptr;
            const Player& bot = ctx.bot->GetGame().GetPlayer();
            bool flagging = bot.frequency == 90 || bot.frequency == 91;
            bool anchor = (bot.frequency == 90 || bot.frequency == 91) && bot.ship == 6;
            Area bot_is = ctx.bot->InArea(bot.position);
            Vector2f position = game.GetPosition();
         
            Vector2f resolution(1920, 1080);
            view_min_ = bot.position - resolution / 2.0f / 16.0f;
            view_max_ = bot.position + resolution / 2.0f / 16.0f;
            //if anchor and not in base switch to patrol mode
            if (anchor && !bot_is.in_base) {
                return behavior::ExecuteResult::Failure;
            }
            //this loop checks every player and finds the closest one based on a cost formula
            //if this does not succeed then there is no target, it will go into patrol mode
            for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
                const Player& player = players[i];

                if (!IsValidTarget(ctx, player)) continue;
                auto to_target = player.position - position;
                CastResult in_sight = RayCast(game.GetMap(), game.GetPosition(), Normalize(to_target), to_target.Length());

                //make players in line of sight high priority
                
                if (flagging && player.position.Distance(team_anchor.position) < 10.0f) {
                    cost = CalculateCost(game, bot, player) / 100.0f;
                }
                else if (player.bounty >= 100 && !flagging) {
                    cost = CalculateCost(game, bot, player) / 100.0f;
                }
                else if (!in_sight.hit) {
                    if (anchor) cost = CalculateCost(game, bot, player) / 1000.0f;
                    else cost = CalculateCost(game, bot, player) / 10.0f;
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

            if (target.dead) return false;
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

            bool anchor = (game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91) && game.GetPlayer().ship == 6;
            bool flagging = game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91;

            Vector2f bot = game.GetPosition();
            Vector2f enemy = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr)->position;

            uint64_t time = ctx.bot->GetTime();
            uint64_t path_check = ctx.blackboard.ValueOr<uint64_t>("PathCheck", 0);

            //keep the anchor on the same path until it performs another path check
            //path probably gets wiped from the blackboard somewhere so use destination to track anchor paths
            Path path = ctx.blackboard.ValueOr<Path>("Destination", std::vector<Vector2f>());
            bool path_switch = ctx.blackboard.ValueOr<bool>("PathSwitch", false);
            float enemy_length = ctx.blackboard.ValueOr<float>("EnemyLength", 0.0f);
            float bot_length = ctx.blackboard.ValueOr<float>("BotLength", 0.0f);

            if (anchor) {
               if (time > path_check) {
                    float length = PathLength(ctx, bot, enemy);
                    if (length < 32) {
                        Area bot_is = ctx.bot->InArea(bot);
                        std::vector<Vector2f> entrance = { Vector2f(827, 339), Vector2f(811, 530), Vector2f(729, 893),
                                                        Vector2f(444, 757), Vector2f(127, 848), Vector2f(268, 552), Vector2f(181, 330) };
                        std::vector<Vector2f> room = { Vector2f(826, 229), Vector2f(834, 540), Vector2f(745, 828),
                                                            Vector2f(489, 836), Vector2f(292, 812), Vector2f(159, 571), Vector2f(205, 204) };
                        for (std::size_t i = 0; i < room.size(); i++) {
                            if (bot_is.in[i]) {
                                if (path_switch) {
                                    enemy_length = PathLength(ctx, enemy, room[i]);
                                    ctx.blackboard.Set("PathSwitch", false);
                                    ctx.blackboard.Set("EnemyLength", enemy_length);
                                }
                                else {
                                    bot_length = PathLength(ctx, bot, room[i]);
                                    ctx.blackboard.Set("PathSwitch", true);
                                    ctx.blackboard.Set("BotLength", bot_length);
                                }

                                if (enemy_length > bot_length) {
                                    path = CreatePath(ctx, "path", bot, room[i], game.GetShipSettings().GetRadius());
                                }
                                else {
                                    path = CreatePath(ctx, "path", bot, entrance[i], game.GetShipSettings().GetRadius());
                                }
                            }
                        }
                    }
                    else if (length >= 32 && length < 38) {
                        path.clear();
                    }
                    else {
                        path = CreatePath(ctx, "path", bot, enemy, game.GetShipSettings().GetRadius());
                    }
                    ctx.blackboard.Set("PathCheck", time + 200);
               }
            }
            else if (game.GetPlayer().ship == 2 && flagging) {
                Flaggers flaggers = ctx.bot->FindFlaggers();
                const Player& team_anchor = flaggers.team_anchor;
                Area anchor_is = ctx.bot->InArea(team_anchor.position);
                if (team_anchor.position.Distance(game.GetPosition()) > 15.0f && anchor_is.connected) {
                    path = CreatePath(ctx, "path", bot, team_anchor.position, game.GetShipSettings().GetRadius());
                }
                else path = CreatePath(ctx, "path", bot, enemy, game.GetShipSettings().GetRadius());
            }
            else {
                path = CreatePath(ctx, "path", bot, enemy, game.GetShipSettings().GetRadius());
            }
            ctx.blackboard.Set("Destination", path);
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


            //this is where the anchor decides what base to move to
            bool flagging = (ctx.bot->GetGame().GetPlayer().frequency == 90 || ctx.bot->GetGame().GetPlayer().frequency == 91);

            //the base selector changes base every 10 mintues by adding one to gate
            int gate = ctx.bot->BaseSelector();
            //gate coords for each base
            std::vector<Vector2f> gates = { Vector2f(961, 62), Vector2f(961, 349), Vector2f(961, 961),
                                            Vector2f(512, 961), Vector2f(61, 960), Vector2f(62, 673), Vector2f(60, 62) };

            Vector2f center_gate_l = Vector2f(388, 396);
            Vector2f center_gate_r = Vector2f(570, 677);
            Area bot_is = ctx.bot->InArea(from);

            if (flagging) {

                Flaggers flaggers = ctx.bot->FindFlaggers();
                const Player& enemy_anchor = flaggers.enemy_anchor;
                Area enemy_anchor_is = ctx.bot->InArea(enemy_anchor.position);

                if (bot_is.in_center) {
                    if (!enemy_anchor_is.in_base) {
                        if (gate == 0 || gate == 1 || gate == 2) {
                            path = CreatePath(ctx, "path", from, center_gate_r, game.GetShipSettings().GetRadius());
                        }
                        else {
                            path = CreatePath(ctx, "path", from, center_gate_l, game.GetShipSettings().GetRadius());
                        }
                    }
                    if (enemy_anchor_is.in_1 || enemy_anchor_is.in_2 || enemy_anchor_is.in_3) {
                        path = CreatePath(ctx, "path", from, center_gate_r, game.GetShipSettings().GetRadius());
                    }
                    else {
                        path = CreatePath(ctx, "path", from, center_gate_l, game.GetShipSettings().GetRadius());
                    }
                }
                if (bot_is.in_tunnel) {
                    for (std::size_t i = 0; i < gates.size(); i++) {
                        if (enemy_anchor_is.in[i]) path = CreatePath(ctx, "path", from, gates[i], game.GetShipSettings().GetRadius());
                    }
                    if (!enemy_anchor_is.in_base) path = CreatePath(ctx, "path", from, Vector2f(gates[gate]), game.GetShipSettings().GetRadius());
                }
                if (!enemy_anchor_is.in_base) {
                    //camp is coords for position part way into base for bots to move to when theres no enemy
                    std::vector< Vector2f > camp = { Vector2f(855, 245), Vector2f(801, 621), Vector2f(773, 839),
                                                      Vector2f(554, 825), Vector2f(325, 877), Vector2f(208, 614), Vector2f(151, 253) };

                    for (std::size_t i = 0; i < camp.size(); i++) {
                        if (bot_is.in[i]) {
                            float dist_to_camp = bot.position.Distance(camp[i]);
                            path = CreatePath(ctx, "path", from, camp[i], game.GetShipSettings().GetRadius());
                            if (dist_to_camp < 10.0f) return behavior::ExecuteResult::Failure;
                        }
                    }
                }
            }

            else {

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

            }

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

                bool anchor = (game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91) && game.GetPlayer().ship == 6;
                if (anchor && game.GetPosition().Distance(*target) > 5) {
                    return behavior::ExecuteResult::Failure;
                }
            
            if (!ray_cast.hit) result = behavior::ExecuteResult::Success;
            else result = behavior::ExecuteResult::Failure;

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

            uint16_t multifire_delay = game.GetShipSettings().MultiFireDelay;
            uint16_t multifire_energy = game.GetShipSettings().MultiFireEnergy;
            int ship = game.GetPlayer().ship;

            //jav lev weasel shark
            if (ship == 1 || ship == 3 || ship == 5 || ship == 7) radius_multiplier = 0.8f;
            if (ship == 2) radius_multiplier = 1.1f; //spid
            if (ship == 6) radius_multiplier = 1.1f; //lanc

            //if the bot has multifire weapon then allow it to shoot wider
            if (multifire_delay != 100 && multifire_energy != 75) {
                if (ship == 1 || ship == 3 || ship == 5 || ship == 7) radius_multiplier = 1.0f;
                if (ship == 0) radius_multiplier = 1.1f;
                if (ship == 2) radius_multiplier = 1.2f;
                if (ship == 4) radius_multiplier = 1.1f;
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
            const auto target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
            const Player& target = *target_player; auto& game = ctx.bot->GetGame();
            Area bot_is = ctx.bot->InArea(ctx.bot->GetGame().GetPlayer().position);

            uint64_t time = ctx.bot->GetTime();
            uint64_t gun_delay = 3;
            uint64_t gun_expire = ctx.blackboard.ValueOr<uint64_t>("GunExpire", 0);
            bool gun_trigger = ctx.blackboard.ValueOr<int>("GunTrigger", 0);

            if (target_player == nullptr) return behavior::ExecuteResult::Failure;

            int ship = game.GetPlayer().ship;
            float distance = target.position.Distance(game.GetPlayer().position);

            uint64_t bomb_expire = ctx.blackboard.ValueOr<uint64_t>("BombCooldownExpire", 0);
            uint64_t bomb_delay = 0;
            uint64_t bomb_fire_delay = game.GetShipSettings().BombFireDelay;
            uint64_t bomb_level = game.GetShipSettings().InitialBombs;
            float fire_distance = 0.0f;

            if (bomb_level == 0) fire_distance = 99.0f / 16.0f;
            if (bomb_level == 1) fire_distance = 198.0f / 16.0f;
            if (bomb_level == 2) fire_distance = 297.0f / 16.0f;
            if (bomb_level == 3) fire_distance = 396.0f / 16.0f;

            //warbird spid terr
            if (ship == 0 || ship == 2 || ship == 4) {
                bomb_delay = bomb_fire_delay * 70;
            }
            //jav lev weasel shark
            if (ship == 1 || ship == 3 || ship == 5 || ship == 7) {
                bomb_delay = bomb_fire_delay * 15;
            }
            //lanc
            if (ship == 6) {
                bomb_delay = bomb_fire_delay * 200;
            }

            if (time > bomb_expire && distance > fire_distance) {
                ctx.bot->GetKeys().Press(VK_TAB);
                //int chat_ = ctx.bot->GetGame().GetSelected();
                //std::string chat = std::to_string(chat_);
                //std::string chat = ctx.bot->GetGame().TickName();
                 //ctx.bot->GetGame().SendChatMessage(chat);
                ctx.blackboard.Set("BombCooldownExpire", time + bomb_delay);
            }
            else {
                ctx.bot->GetKeys().Press(VK_CONTROL);
                ctx.blackboard.Set("GunExpire", time + gun_delay);
            }

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
            Flaggers flaggers = ctx.bot->FindFlaggers();
            float energy_pct = (game.GetEnergy() / (float)game.GetShipSettings().InitialEnergy);

            Vector2f target_position = ctx.blackboard.ValueOr("target_position", Vector2f());
            
            int ship = game.GetPlayer().ship;
            
            std::vector<float> ship_distance{ 5.0f, 10.0f, 5.0f, 15.0f, 5.0f, 10.0f, 5.0f, 5.0f };

            float hover_distance = 0.0f;

            if (bot_is.in_base) {
                if (ship == 2) {
                    const Player& team_anchor = flaggers.team_anchor;
                    ctx.bot->Move(team_anchor.position, 0.0f);
                    ctx.bot->GetSteering().Face(target_position);
                    return behavior::ExecuteResult::Success;
                }
                else {
                    for (std::size_t i = 0; i < ship_distance.size(); i++) {
                        if (ship == i) {
                            hover_distance = ship_distance[i];
                        }
                    }
                }
            }
            else {
                for (std::size_t i = 0; i < ship_distance.size(); i++) {
                    if (ship == i) {
                        hover_distance = (ship_distance[i] / energy_pct);
                        if (ship == 0 || ship == 2 || ship == 4) {
                            //hover_distance = 2.0f;
                        }
                    }
                }
            }

            MapCoord spawn =
                ctx.bot->GetGame().GetSettings().SpawnSettings[0].GetCoord();

            if (Vector2f(spawn.x, spawn.y)
                .DistanceSq(ctx.bot->GetGame().GetPosition()) < 35.0f * 35.0f) {
                hover_distance = 0.0f;
            }

            const Player& shooter =
                *ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
#if 0  // Simple weapon avoidance but doesn't work great
            bool weapon_dodged = false;
            for (Weapon* weapon : game.GetWeapons()) {
                const Player* weapon_player = game.GetPlayerById(weapon->GetPlayerId());

                if (weapon_player == nullptr) continue;

                if (weapon_player->frequency == game.GetPlayer().frequency) continue;

                const auto& player = game.GetPlayer();

                if (weapon->GetType() & 0x8F00 && weapon->GetPosition().DistanceSq(player.position) < 20 * 20) {
                    Vector2f direction = Normalize(player.position - weapon->GetPosition());
                    Vector2f perp = Perpendicular(direction);

                    if (perp.Dot(player.velocity) < 0) {
                        perp = -perp;
                    }

                    ctx.bot->GetSteering().Seek(player.position + perp, 100.0f);
                    weapon_dodged = true;
                }
                else if (weapon->GetPosition().DistanceSq(player.position) < 50 * 50) {
                    Vector2f direction = Normalize(weapon->GetVelocity());

                    float radius = game.GetShipSettings(player.ship).GetRadius() * 6.0f;
                    Vector2f box_pos = player.position - Vector2f(radius, radius);
                    Vector2f extent(radius * 2, radius * 2);

                    float dist;
                    Vector2f norm;

                    if (RayBoxIntersect(weapon->GetPosition(), direction, box_pos, extent, &dist, &norm)) {
                        Vector2f perp = Perpendicular(direction);

                        if (perp.Dot(player.velocity) < 0) {
                            perp = perp * -1.0f;
                        }

                        ctx.bot->GetSteering().Seek(player.position + perp, 2.0f);
                        weapon_dodged = true;
                    }
                }
            }

            if (weapon_dodged) {
                return behavior::ExecuteResult::Success;
            }
#endif


            if (energy_pct < 0.25f && !bot_is.in_base) {
                Vector2f dodge;

                if (IsAimingAt(game, shooter, game.GetPlayer(), &dodge)) {
                    ctx.bot->Move(game.GetPosition() + dodge, 0.0f);
                    return behavior::ExecuteResult::Success;
                }
            }
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


	} //namespace hs
} //namespace marvin