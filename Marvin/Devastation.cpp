#include "Devastation.h"

namespace marvin {
	namespace deva {

        behavior::ExecuteResult FreqWarpAttachNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
         
            uint64_t time = ctx.bot->GetTime();

            uint64_t freq_cooldown = ctx.blackboard.ValueOr<uint64_t>("FreqCooldown", 0);
  
            uint64_t toggle_cooldown = ctx.blackboard.ValueOr<uint64_t>("ToggleCooldown", 0);

            //bots on the same team tend to switch out at the same time when team is uneven, they need unique wait timers
            uint64_t freq_timer_offset = ((uint64_t)game.GetPlayer().id * 100) + 1000;

            //makes the timers load up for the first pass, needed to keep multiple bots from spamming frequency changes
            bool set_timer = ctx.blackboard.ValueOr<bool>("SetTimer", true);
            //reset the timer if it gets toggled but not toggled back
            if (time > freq_cooldown + 150) set_timer = true;

            //if player is on a non duel team size greater than 2, breaks the zone balancer
            if (game.GetPlayer().frequency != 00 && game.GetPlayer().frequency != 01) {
                if (ctx.bot->GetFreqList()[game.GetPlayer().frequency] > 1) {
                    if (set_timer) {
                        ctx.blackboard.Set("FreqCooldown", time + freq_timer_offset);
                        ctx.blackboard.Set("SetTimer", false);
                    }
                    //this team is too large, find an open freq
                    else if (time > freq_cooldown && CheckStatus(ctx)) {
                        game.SetFreq(ctx.bot->FindOpenFreq());
                        ctx.blackboard.Set("SetTimer", true);
                    }
                }
            }

            //duel team is uneven
            if (ctx.bot->GetFreqList()[0] != ctx.bot->GetFreqList()[1]) {
                    //bot is not on a duel team
                if (game.GetPlayer().frequency != 00 && game.GetPlayer().frequency != 01) {
                    //get on a baseduel freq as fast as possible
                    if (CheckStatus(ctx)) {
                        //get on freq 0
                        if (ctx.bot->GetFreqList()[0] < ctx.bot->GetFreqList()[1]) {
                            game.SetFreq(0);
                        }
                        //get on freq 1
                        else {
                            game.SetFreq(1);
                        }
                    }
                    return behavior::ExecuteResult::Success;
                }
                    //bot is on a duel team
                else if (ctx.bot->InCenter() || ctx.bot->GetDevaTeam().size() == 0) {

                    if ((game.GetPlayer().frequency == 00 && ctx.bot->GetFreqList()[0] > ctx.bot->GetFreqList()[1]) ||
                        (game.GetPlayer().frequency == 01 && ctx.bot->GetFreqList()[0] < ctx.bot->GetFreqList()[1])) {
                        
                        //look for players not on duel team, make bot wait longer to see if the other player will get in
                        for (std::size_t i = 2; i < ctx.bot->GetFreqList().size(); i++) {
                            if (ctx.bot->GetFreqList()[i] > 0) {
                                freq_timer_offset += 10000;
                                break;
                            }
                        }

                        /* set the timer first, the bot waits before it can switch out of a team, but only if the conditions are
                           still valid when the timer expires.  this keeps multiple bots on the same team from switching out at the ame time */
                        if (set_timer) {
                            ctx.blackboard.Set("FreqCooldown", time + freq_timer_offset);
                            ctx.blackboard.Set("SetTimer", false);
                        }
                            //this team is too large, find an open freq
                        else if (time > freq_cooldown && CheckStatus(ctx)) {

                            game.SetFreq(ctx.bot->FindOpenFreq());
                            ctx.blackboard.Set("SetTimer", true);
                        }
                        return behavior::ExecuteResult::Success;
                    }
                }
            }

            //if dueling then run checks for attaching
            if (game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01) {

                //if this check is valid the bot will halt here untill it attaches
                if (ctx.bot->InCenter() && TeamInBase(ctx)) {

                    SetAttachTarget(ctx);

                    //checks if energy is full
                    if (CheckStatus(ctx)) {
                        game.F7();
                        ctx.blackboard.Set("PressedF7", true);
                    }
                    return behavior::ExecuteResult::Success;
                }
                ctx.blackboard.Set("PressedF7", false);
                ctx.blackboard.Set("LastTarget", nullptr);
            }
            //if attached to a player detach
            if (game.GetPlayer().attach_id != 65535) {
                game.F7();
                return behavior::ExecuteResult::Success;
            }

            
            //std::string text = std::to_string(game.GetSelectedPlayerIndex());
            //RenderText(text.c_str(), GetWindowCenter() - Vector2f(0, 200), RGB(100, 100, 100), RenderText_Centered);

            bool x_active = (game.GetPlayer().status & 4) != 0; 
            bool stealthing = (game.GetPlayer().status & 1) != 0;
            bool cloaking = (game.GetPlayer().status & 2) != 0;

            bool has_xradar = (game.GetShipSettings().XRadarStatus & 3) != 0;
            bool has_stealth = (game.GetShipSettings().StealthStatus & 1) != 0;
            bool has_cloak = (game.GetShipSettings().CloakStatus & 3) != 0;

            //if stealth isnt on but availible, presses home key in continuumgameproxy.cpp
            if (!stealthing && has_stealth && time > toggle_cooldown) {
                game.Stealth();
                //ctx.blackboard.Set("ToggleCooldown", time + 300);
                return behavior::ExecuteResult::Success;
            }
            //same as stealth but presses shift first
            if (!cloaking && has_cloak && time > toggle_cooldown) {
                game.Cloak(ctx.bot->GetKeys());
                ctx.blackboard.Set("ToggleCooldown", time + 300);
                return behavior::ExecuteResult::Success;
            }
            //in deva xradar is free so just turn it on
            if (!x_active && has_xradar && time > toggle_cooldown) {
                    game.XRadar();
                    //ctx.blackboard.Set("ToggleCooldown", time + 300);
                    return behavior::ExecuteResult::Success;
            }
            return behavior::ExecuteResult::Failure;
        }

        bool FreqWarpAttachNode::CheckStatus(behavior::ExecuteContext& ctx) {
            
            auto& game = ctx.bot->GetGame();
            float energy_pct = (game.GetEnergy() / (float)game.GetShipSettings().MaximumEnergy) * 100.0f;
            bool result = false;
            if ((game.GetPlayer().status & 2) != 0) game.Cloak(ctx.bot->GetKeys());
            if (energy_pct == 100.0f) result = true;
            return result;
        }

        void FreqWarpAttachNode::SetAttachTarget(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();

            float closest_distance = std::numeric_limits<float>::max();
            float closest_team_distance_to_team = std::numeric_limits<float>::max();
            float closest_team_distance_to_enemy = std::numeric_limits<float>::max();
            float closest_enemy_distance_to_team = std::numeric_limits<float>::max();
            float closest_enemy_distance_to_enemy = std::numeric_limits<float>::max(); 

            bool enemy_leak = false;
            bool team_leak = false;

            const Player* closest_team_to_team = nullptr;
            Vector2f closest_enemy_to_team;
            const Player* closest_team_to_enemy = nullptr;
            const Player* closest_enemy_to_enemy = nullptr;

            //find closest player to team and enemy safe
            for (std::size_t i = 0; i < ctx.bot->GetDevaDuelers().size(); i++) {
                const Player& player = ctx.bot->GetDevaDuelers()[i];
                bool player_in_center = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));
                if (!player_in_center && IsValidPosition(player.position) && player.ship < 8) {
                    
                    if (player.frequency == game.GetPlayer().frequency) {
                        float distance_to_team = ctx.bot->PathLength(player.position, ctx.bot->GetTeamSafe());
                        float distance_to_enemy = ctx.bot->PathLength(player.position, ctx.bot->GetEnemySafe());

                        if (distance_to_team < closest_team_distance_to_team) {
                            closest_team_distance_to_team = distance_to_team;
                            closest_team_to_team = &ctx.bot->GetDevaDuelers()[i];
                        }
                        if (distance_to_enemy < closest_team_distance_to_enemy) {
                            closest_team_distance_to_enemy = distance_to_enemy;
                            closest_team_to_enemy = &ctx.bot->GetDevaDuelers()[i];
                        }
                    }
                    else {
                        float distance_to_team = ctx.bot->PathLength(player.position, ctx.bot->GetTeamSafe());
                        float distance_to_enemy = ctx.bot->PathLength(player.position, ctx.bot->GetEnemySafe());

                        if (distance_to_team < closest_enemy_distance_to_team) {
                            closest_enemy_distance_to_team = distance_to_team;
                            closest_enemy_to_team = player.position;
                        }
                        if (distance_to_enemy < closest_enemy_distance_to_enemy) {
                            closest_enemy_distance_to_enemy = distance_to_enemy;
                            closest_enemy_to_enemy = &ctx.bot->GetDevaDuelers()[i];
                        }
                    }
                }
            }



            if (closest_team_distance_to_team > closest_enemy_distance_to_team) {
                    enemy_leak = true;
            }
            if (closest_enemy_distance_to_enemy > closest_team_distance_to_enemy) {
                    team_leak = true;
            }

            if (team_leak || enemy_leak) {
                game.SetSelectedPlayer(*closest_team_to_team);
                return;
            }

            //find closest team player to the enemy closest to team safe
            for (std::size_t i = 0; i < ctx.bot->GetDevaTeam().size(); i++) {
                const Player& player = ctx.bot->GetDevaTeam()[i];

                bool player_in_center = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));

                //as long as the player is not in center and has a valid position, it will hold its position for a moment after death
                if (!player_in_center && IsValidPosition(player.position) && player.ship < 8) {

                    float distance_to_enemy_position = ctx.bot->PathLength(player.position, closest_enemy_to_team);

                    //get the closest player
                    if (distance_to_enemy_position < closest_distance) {
                        closest_distance = distance_to_enemy_position;
                        game.SetSelectedPlayer(ctx.bot->GetDevaTeam()[i]);
                    }   
                }
            }  
        }

        bool FreqWarpAttachNode::TeamInBase(behavior::ExecuteContext& ctx) {

            bool in_base = false;

            for (std::size_t i = 0; i < ctx.bot->GetDevaTeam().size(); i++) {
                const Player& player = ctx.bot->GetDevaTeam()[i];
                bool in_center = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));
                if (!in_center && IsValidPosition(player.position)) {
                    in_base = true;
                }
            }
            return in_base;
        }





        behavior::ExecuteResult FindEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            behavior::ExecuteResult result = behavior::ExecuteResult::Failure;
            float closest_cost = std::numeric_limits<float>::max();
            float cost = 0;
            auto& game = ctx.bot->GetGame();
            std::vector< Player > players = game.GetPlayers();
            const Player* target = nullptr;
            const Player& bot = ctx.bot->GetGame().GetPlayer();

            //bool in_center = ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));
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
            //if (target != nullptr) { ctx.bot->LookForWallShot(*target, false); }
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

            if (!ctx.bot->InCenter() && ctx.bot->LastBotStanding()) return false;
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
            
            if (energy_pct < 25.0f && ctx.bot->InCenter()) game.Repel(ctx.bot->GetKeys());

            Vector2f bot = game.GetPosition();
            Vector2f enemy = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr)->position;

            Path path;
            float radius = game.GetShipSettings().GetRadius();

            if (!ctx.bot->InCenter()) {
                if (game.GetPlayer().ship == 2) {
                    if (ctx.bot->PathLength(bot, enemy) < 55.0f || ctx.bot->LastBotStanding()) {
                        path = CreatePath(ctx, "path", bot, ctx.bot->GetTeamSafe(), radius);
                    }
                    else path = CreatePath(ctx, "path", bot, enemy, radius);
                }
                else if (ctx.bot->LastBotStanding()) {
                    path = CreatePath(ctx, "path", bot, ctx.bot->GetTeamSafe(), radius);
                }
                else path = CreatePath(ctx, "path", bot, enemy, radius);
            }
            else path = CreatePath(ctx, "path", bot, enemy, radius);

            ctx.blackboard.Set("path", path);
            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult PatrolNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            Vector2f from = game.GetPosition();
            Path path = ctx.blackboard.ValueOr("path", Path());
            float radius = game.GetShipSettings().GetRadius();
 
            std::vector<Vector2f> nodes;
            int index = ctx.blackboard.ValueOr<int>("patrol_index", 0);
            int taunt_index = ctx.blackboard.ValueOr<int>("taunt_index", 0);
            uint64_t time = ctx.bot->GetTime();
            uint64_t hoorah_expire = ctx.blackboard.ValueOr<uint64_t>("HoorahExpire", 0);
            bool in_safe = game.GetMap().GetTileId(game.GetPosition()) == kSafeTileId;

            if (ctx.bot->InCenter()) {
                nodes = { Vector2f(568, 568), Vector2f(454, 568), Vector2f(454, 454), Vector2f(568, 454), Vector2f(568, 568),
                Vector2f(454, 454), Vector2f(568, 454), Vector2f(454, 568), Vector2f(454, 454), Vector2f(568, 568), Vector2f(454, 568), Vector2f(568, 454) };
            }
            else {
                if (ctx.bot->LastBotStanding()) {
                    path = CreatePath(ctx, "path", from, ctx.bot->GetTeamSafe(), radius);
                    ctx.blackboard.Set("path", path);
                    return behavior::ExecuteResult::Success;
                }
                else {
                    path = CreatePath(ctx, "path", from, ctx.bot->GetEnemySafe(), radius);
                    ctx.blackboard.Set("path", path);
                    return behavior::ExecuteResult::Success;
                }
                /*
                if (time > hoorah_expire && !in_safe) {
                    std::vector<std::string> taunts = { "we gotem", "man, im glad im not on that freq", "high score is that bad",
                    "kill all humans", "that team is almost as bad as Daman24/7 is", "robots unite", "this is all thanks to my high quality attaching skills" };
                    std::string taunt = taunts.at(taunt_index);
                    game.SendChatMessage(taunt);
                    taunt_index = (taunt_index + 1) % taunts.size();
                    ctx.blackboard.Set("taunt_index", taunt_index);
                    ctx.blackboard.Set("HoorahExpire", time + 600000);
                }
            */
                //return behavior::ExecuteResult::Failure;

            }
          

            Vector2f to = nodes.at(index);

            if (game.GetPosition().DistanceSq(to) < 5.0f * 5.0f) {
                index = (index + 1) % nodes.size();
                ctx.blackboard.Set("patrol_index", index);
                to = nodes.at(index);
            }

            path = CreatePath(ctx, "path", from, to, radius);
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
            CastResult side1 = RayCast(game.GetMap(), from + side * radius, direction, distance);
            CastResult side2 = RayCast(game.GetMap(), from - side * radius, direction, distance);

            return !center.hit && !side1.hit && !side2.hit;
        }




        behavior::ExecuteResult InLineOfSightNode::Execute(behavior::ExecuteContext& ctx) {
            behavior::ExecuteResult result = behavior::ExecuteResult::Failure;
            const Vector2f* target = selector_(ctx);

            if (target == nullptr) { return behavior::ExecuteResult::Failure; }
            auto& game = ctx.bot->GetGame();  

            float bullet_speed = game.GetSettings().ShipSettings[game.GetPlayer().ship].BulletSpeed / 10.0f / 16.0f;
            float bomb_speed = game.GetSettings().ShipSettings[game.GetPlayer().ship].BombSpeed / 10.0f / 16.0f;
            int bullet_time = game.GetSettings().BulletAliveTime;
            int bomb_time = game.GetSettings().BombAliveTime;
            uint8_t bounces = game.GetSettings().ShipSettings[game.GetPlayer().ship].BombBounceCount;

            auto to_target = *target - game.GetPosition();
            CastResult ray_cast = RayCast(game.GetMap(), game.GetPosition(), Normalize(to_target), to_target.Length());

            if (!ray_cast.hit) {
                result = behavior::ExecuteResult::Success;
                //if bot is not in center, almost dead, and in line of sight of its target burst
                bool in_center = ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));
                float energy_pct = ctx.bot->GetGame().GetEnergy() / (float)ctx.bot->GetGame().GetShipSettings().MaximumEnergy;
                if (energy_pct < 0.10f && !in_center) {
                    ctx.bot->GetGame().Burst(ctx.bot->GetKeys());
                }
            }
            else {
                const Player* target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
                if (target_player != nullptr) {
                   // ctx.bot->LookForWallShot(target_player->position, target_player->velocity, bomb_speed, bomb_time, bounces);
                    //if (!ctx.bot->HasWallShot()) {
                        ctx.bot->LookForWallShot(target_player->position, target_player->velocity, bullet_speed, bullet_time, 10);
                        if (ctx.bot->HasWallShot()) {
                            result = behavior::ExecuteResult::Success;
                        }
                   // }
                }
            }

            return result;
        }




        behavior::ExecuteResult LookingAtEnemyNode::Execute(behavior::ExecuteContext& ctx) {

            if (ctx.bot->HasWallShot()) {
                ctx.blackboard.Set("target_position", ctx.bot->GetWallShot());
                return behavior::ExecuteResult::Success;
            }

            const auto target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            if (target_player == nullptr) return behavior::ExecuteResult::Failure;

            const Player& target = *target_player;
            auto& game = ctx.bot->GetGame();
            const Player& bot_player = game.GetPlayer(); 

            float proj_speed = game.GetSettings().ShipSettings[bot_player.ship].BulletSpeed / 10.0f / 16.0f;

            Vector2f seek_position = CalculateShot(game.GetPosition(), target.position, bot_player.velocity, target.velocity, proj_speed);

            Vector2f projectile_trajectory = (bot_player.GetHeading() * proj_speed) + bot_player.velocity;

            Vector2f projectile_direction = Normalize(projectile_trajectory);

            float target_radius = game.GetSettings().ShipSettings[target.ship].GetRadius();

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
            
            bool hit = RayBoxIntersect(bot_player.position, projectile_direction, box_min, box_extent, &dist, &norm);

            if (!hit) {
               Vector2f box_min = seek_position - Vector2f(nearby_radius, nearby_radius);
               hit = RayBoxIntersect(bot_player.position, bot_player.GetHeading(), box_min, box_extent, &dist, &norm);
            }

            if (seek_position.DistanceSq(target_player->position) < 15 * 15) {
                ctx.blackboard.Set("target_position", seek_position);
            }
            else {
                ctx.blackboard.Set("target_position", target.position);
            }

            if (hit) {
                if (ctx.bot->CanShootGun(game.GetMap(), bot_player, target)) {
                    return behavior::ExecuteResult::Success;
                }
            }

            return behavior::ExecuteResult::Failure;
        }




        behavior::ExecuteResult ShootEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            int ship = ctx.bot->GetGame().GetPlayer().ship;
            if (ship == 3 || ship == 6 || ship == 2) ctx.bot->GetKeys().Press(VK_TAB);
            else ctx.bot->GetKeys().Press(VK_CONTROL);

            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult MoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            
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
            if (ctx.bot->HasWallShot()) {
                target_position = ctx.bot->GetWallShot();
            }


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
                if (ctx.bot->InCenter()) {
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
                        hover_distance = 7.0f;                  
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