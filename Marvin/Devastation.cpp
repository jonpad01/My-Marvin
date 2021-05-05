#include <chrono>
#include <cstring>
#include <limits>



#include "Bot.h"
#include "Devastation.h"
#include "Debug.h"
#include "GameProxy.h"
#include "Map.h"
#include "RayCaster.h"
#include "RegionRegistry.h"
#include "platform/Platform.h"
#include "platform/ContinuumGameProxy.h"
#include "Shooter.h"
#include "Commands.h"



namespace marvin {
    namespace deva {


        behavior::ExecuteResult DevaSetRegionNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            auto& bb = ctx.blackboard;

           const std::vector<Vector2f>& team0_safes = ctx.bot->GetDeva0Safe();
           const std::vector<Vector2f>& team1_safes = ctx.bot->GetDeva1Safe();

           bool in_center = ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(), (MapCoord)bb.ValueOr<Vector2f>("Spawn", Vector2f(512, 512)));

            bb.Set<bool>("InCenter", in_center);

            
            if (!in_center && game.GetPlayer().active) {
                if (!ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(), (MapCoord)team0_safes[bb.ValueOr<std::size_t>("BaseIndex", 0)])) {

                    for (std::size_t i = 0; i < team0_safes.size(); i++) {
                        if (ctx.bot->GetRegions().IsConnected((MapCoord)game.GetPosition(), (MapCoord)team0_safes[i])) {
                            bb.Set<std::size_t>("BaseIndex", i);
                            if (game.GetPlayer().frequency == 00) {
                                bb.Set<Vector2f>("TeamSafe", team0_safes[i]);
                                bb.Set<Vector2f>("EnemySafe", team1_safes[i]);
                            }
                            else if (game.GetPlayer().frequency == 01) {
                                bb.Set<Vector2f>("TeamSafe", team1_safes[i]);
                                bb.Set<Vector2f>("EnemySafe", team0_safes[i]);
                            }
                            return behavior::ExecuteResult::Success;
                        }
                    }
                }
            }
            return behavior::ExecuteResult::Success;
        }





        behavior::ExecuteResult DevaFreqMan::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            auto& bb = ctx.blackboard;

            bool dueling = game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01;

            if (bb.ValueOr<bool>("FreqLock", false) || (bb.ValueOr<bool>("IsAnchor", false) && dueling)) {
                return behavior::ExecuteResult::Success;
            }
            uint64_t offset = ctx.bot->GetTime().DevaFreqTimer(ctx.bot->GetBotNames());
            //uint64_t offset = ctx.bot->GetTime().UniqueIDTimer(game.GetPlayer().id);

            std::vector<uint16_t> fList = bb.ValueOr<std::vector<uint16_t>>("FreqList", std::vector<uint16_t>());

            //if player is on a non duel team size greater than 2, breaks the zone balancer
            if (game.GetPlayer().frequency != 00 && game.GetPlayer().frequency != 01) {
                if (fList[game.GetPlayer().frequency] > 1) {

                    if (ctx.bot->GetTime().TimedActionDelay("freqchange", offset)) {
                        game.SetEnergy(100.0f); 
                        game.SetFreq(FindOpenFreq(fList, 2));
                    }
                    return behavior::ExecuteResult::Failure;
                }
            }

            //duel team is uneven
            if (fList[0] != fList[1]) {
                if (!dueling) {

                    if (ctx.bot->GetTime().TimedActionDelay("freqchange", offset)) {
                        game.SetEnergy(100.0f);
                        if (fList[0] < fList[1]) {
                            game.SetFreq(0);
                        }
                        //get on freq 1
                        else {
                            game.SetFreq(1);
                        }
                    }
                    return behavior::ExecuteResult::Failure;
                }
                //bot is on a duel team
                else if (bb.ValueOr<bool>("InCenter", true) || bb.ValueOr<std::vector<Player>>("TeamList", std::vector<Player>()).size() == 0 || bb.ValueOr<bool>("TeamInBase", false)) {

                    if ((game.GetPlayer().frequency == 00 && fList[0] > fList[1]) ||
                        (game.GetPlayer().frequency == 01 && fList[0] < fList[1])) {

                        //look for players not on duel team, make bot wait longer to see if the other player will get in
                        for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
                            const Player& player = game.GetPlayers()[i];

                            if (player.frequency > 01 && player.frequency < 100) {
                                float energy_pct = (player.energy / (float)game.GetSettings().ShipSettings[player.ship].MaximumEnergy) * 100.0f;
                                if (energy_pct != 100.0f) {
                                    return behavior::ExecuteResult::Failure;
                                }
                            }
                        }

                        if (ctx.bot->GetTime().TimedActionDelay("freqchange", offset)) {
                            game.SetEnergy(100.0f);
                            game.SetFreq(FindOpenFreq(fList, 2));
                        }
                        return behavior::ExecuteResult::Failure;
                    }
                }
            }
            return behavior::ExecuteResult::Success;
        }





        behavior::ExecuteResult DevaWarpNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            auto& bb = ctx.blackboard;
            auto& time = ctx.bot->GetTime();

            uint64_t base_empty_timer = 30000;

            if (!bb.ValueOr<bool>("InCenter", true)) {
                if (!bb.ValueOr<bool>("EnemyInBase", false)) {

                    if (time.TimedActionDelay("baseemptywarp", base_empty_timer)) {
                        game.SetEnergy(100.0f);
                            game.Warp();
                    }
                }
            }
            return behavior::ExecuteResult::Success;
        }





    behavior::ExecuteResult DevaAttachNode::Execute(behavior::ExecuteContext& ctx) { 
        auto& game = ctx.bot->GetGame();
        auto& bb = ctx.blackboard;

        //if dueling then run checks for attaching
        if (game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01) {

            if (bb.ValueOr<bool>("TeamInBase", false)) {
                if (bb.ValueOr<bool>("GetSwarm", false)) {
                    if (!game.GetPlayer().active) {
                        if (ctx.bot->GetTime().TimedActionDelay("respawn", 300)) {
                            int ship = 1;
                            if (game.GetPlayer().ship == 1) {
                                ship = 7;
                            }
                            else {
                                ship = 1;
                            }
                            game.SetEnergy(100.0f);
                            if (!game.SetShip(ship)) {
                                ctx.bot->GetTime().TimedActionDelay("respawn", 0);
                            }
                        }
                        return behavior::ExecuteResult::Failure;
                    }
                }
                //if this check is valid the bot will halt here untill it attaches
                if (bb.ValueOr<bool>("InCenter", true)) {

                    SetAttachTarget(ctx);

                    //checks if energy is full
                    if (CheckStatus(game, ctx.bot->GetKeys(), true)) {
                        game.F7();
                    }
                    return behavior::ExecuteResult::Failure;
                }
            }
        }
        //if attached to a player detach
        if (game.GetPlayer().attach_id != 65535) {
            if (bb.ValueOr<bool>("Swarm", false)) {
                game.SetEnergy(15.0f);
            }
            game.F7();
            return behavior::ExecuteResult::Failure;
        }
        return behavior::ExecuteResult::Success;
    }

    void DevaAttachNode::SetAttachTarget(behavior::ExecuteContext& ctx) {
        auto& game = ctx.bot->GetGame();
        auto& bb = ctx.blackboard;

        std::vector<Vector2f> path = ctx.bot->GetBasePath();

        if (path.empty()) {
            return;
        }

        std::vector<Player> team_list = bb.ValueOr<std::vector<Player>>("TeamList", std::vector<Player>());
        std::vector<Player> combined_list = bb.ValueOr<std::vector<Player>>("CombinedList", std::vector<Player>());

        float closest_distance = std::numeric_limits<float>::max();
        float closest_team_distance_to_team = std::numeric_limits<float>::max();
        float closest_team_distance_to_enemy = std::numeric_limits<float>::max();
        float closest_enemy_distance_to_team = std::numeric_limits<float>::max();
        float closest_enemy_distance_to_enemy = std::numeric_limits<float>::max();

        bool enemy_leak = false;
        bool team_leak = false;

        uint16_t closest_team_to_team = 0;
        Vector2f closest_enemy_to_team;
        uint16_t closest_team_to_enemy = 0;
        uint16_t closest_enemy_to_enemy = 0;

        //find closest player to team and enemy safe
        for (std::size_t i = 0; i < combined_list.size(); i++) {

            const Player& player = combined_list[i];
            //bool player_in_center = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));
            bool player_in_base = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, MapCoord(path[0]));

            if (player_in_base) {
            //if (!player_in_center && IsValidPosition(player.position) && player.ship < 8) {

                
                float distance_to_team = PathLength(path, player.position, bb.ValueOr<Vector2f>("TeamSafe", Vector2f()));
                float distance_to_enemy = PathLength(path, player.position, bb.ValueOr<Vector2f>("EnemySafe", Vector2f()));

                if (player.frequency == game.GetPlayer().frequency) {
                    //float distance_to_team = ctx.deva->PathLength(player.position, ctx.deva->GetTeamSafe());
                    //float distance_to_enemy = ctx.deva->PathLength(player.position, ctx.deva->GetEnemySafe());

                    if (distance_to_team < closest_team_distance_to_team) {
                        closest_team_distance_to_team = distance_to_team;
                        closest_team_to_team = bb.ValueOr<std::vector<Player>>("CombinedList", std::vector<Player>())[i].id;
                    }
                    if (distance_to_enemy < closest_team_distance_to_enemy) {
                        closest_team_distance_to_enemy = distance_to_enemy;
                        closest_team_to_enemy = bb.ValueOr<std::vector<Player>>("CombinedList", std::vector<Player>())[i].id;
                    }
                }
                else {
                    //float distance_to_team = ctx.deva->PathLength(player.position, ctx.deva->GetTeamSafe());
                    //float distance_to_enemy = ctx.deva->PathLength(player.position, ctx.deva->GetEnemySafe());

                    if (distance_to_team < closest_enemy_distance_to_team) {
                        closest_enemy_distance_to_team = distance_to_team;
                        closest_enemy_to_team = player.position;
                    }
                    if (distance_to_enemy < closest_enemy_distance_to_enemy) {
                        closest_enemy_distance_to_enemy = distance_to_enemy;
                        closest_enemy_to_enemy = bb.ValueOr<std::vector<Player>>("CombinedList", std::vector<Player>())[i].id;
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

        if (team_leak || enemy_leak || bb.ValueOr<bool>("IsAnchor", false)) {
            game.SetSelectedPlayer(closest_team_to_team);
            return;
        }

        

        //find closest team player to the enemy closest to team safe
        for (std::size_t i = 0; i < team_list.size(); i++) {

            const Player& player = team_list[i];

            //bool player_in_center = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));

            bool player_in_base = ctx.bot->GetRegions().IsConnected((MapCoord)player.position, MapCoord(path[0]));

            if (player_in_base) {

            //as long as the player is not in center and has a valid position, it will hold its position for a moment after death
            //if (!player_in_center && IsValidPosition(player.position) && player.ship < 8) {

                // float distance_to_enemy_position = ctx.deva->PathLength(player.position, closest_enemy_to_team);

                float distance_to_enemy_position = 0.0f;

                distance_to_enemy_position = PathLength(path, player.position, closest_enemy_to_team);

                //get the closest player
                if (distance_to_enemy_position < closest_distance) {
                    closest_distance = distance_to_enemy_position;
                    game.SetSelectedPlayer(player.id);
                }
            }
        }
    }






        behavior::ExecuteResult DevaToggleStatusNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            auto& bb = ctx.blackboard;

            if (bb.ValueOr<bool>("UseMultiFire", false)) {
                if (!game.GetPlayer().multifire_status) {
                    game.MultiFire();
                    return behavior::ExecuteResult::Failure;
                }
            }
            else {
                if (game.GetPlayer().multifire_status) {
                    game.MultiFire();
                    return behavior::ExecuteResult::Failure;
                }
            }


            //RenderText(text.c_str(), GetWindowCenter() - Vector2f(0, 200), RGB(100, 100, 100), RenderText_Centered);

            bool x_active = (game.GetPlayer().status & 4) != 0;
            bool stealthing = (game.GetPlayer().status & 1) != 0;
            bool cloaking = (game.GetPlayer().status & 2) != 0;

            bool has_xradar = (game.GetShipSettings().XRadarStatus & 3) != 0;
            bool has_stealth = (game.GetShipSettings().StealthStatus & 1) != 0;
            bool has_cloak = (game.GetShipSettings().CloakStatus & 3) != 0;

            //in deva these are free so just turn them on
            if (!x_active && has_xradar) {
                if (ctx.bot->GetTime().TimedActionDelay("xradar", 800)) {
                    game.XRadar();
                    return behavior::ExecuteResult::Failure;
                }
                return behavior::ExecuteResult::Success;
            }
            if (!stealthing && has_stealth) {
                if (ctx.bot->GetTime().TimedActionDelay("stealth", 800)) {
                    game.Stealth();
                    return behavior::ExecuteResult::Failure;
                }
                return behavior::ExecuteResult::Success;
            }
            if (!cloaking && has_cloak) {
                if (ctx.bot->GetTime().TimedActionDelay("cloak", 800)) {
                    game.Cloak(ctx.bot->GetKeys());
                    return behavior::ExecuteResult::Failure;
                }
                return behavior::ExecuteResult::Success;
            }

            return behavior::ExecuteResult::Success;
        }






        behavior::ExecuteResult DevaPatrolBaseNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            auto& bb = ctx.blackboard;


            Vector2f enemy_safe = bb.ValueOr<Vector2f>("EnemySafe", Vector2f());
            Path path = bb.ValueOr<Path>("Path", Path());

            path = ctx.bot->GetPathfinder().CreatePath(path, game.GetPosition(), enemy_safe, game.GetShipSettings().GetRadius());

            bb.Set<Path>("Path", path);

            return behavior::ExecuteResult::Success;
        }


        





        behavior::ExecuteResult DevaRepelEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            auto& bb = ctx.blackboard;

            if (!bb.ValueOr<bool>("InCenter", true)) {

                if (bb.ValueOr<bool>("IsAnchor", false)) {
                    if (game.GetEnergyPercent() < 20.0f && game.GetPlayer().repels > 0) {
                        game.Repel(ctx.bot->GetKeys());
                        return behavior::ExecuteResult::Success;
                    }
                }
                else if (bb.ValueOr<bool>("UseRepel", false)) {
                    if (game.GetEnergyPercent() < 10.0f && game.GetPlayer().repels > 0 && game.GetPlayer().bursts == 0) {
                        game.Repel(ctx.bot->GetKeys());
                        return behavior::ExecuteResult::Success;
                    }
                }
            }
            return behavior::ExecuteResult::Failure;
        }





        behavior::ExecuteResult DevaBurstEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            auto& bb = ctx.blackboard;

            if (!bb.ValueOr<bool>("InCenter", true)) {

                const Player* target = bb.ValueOr<const Player*>("Target", nullptr);
                if (!target) { return behavior::ExecuteResult::Failure; }

                if ((target->position.Distance(game.GetPosition()) < 7.0f || game.GetEnergyPercent() < 10.0f) && game.GetPlayer().bursts > 0) {
                    game.Burst(ctx.bot->GetKeys());
                    return behavior::ExecuteResult::Success;
                }
            }
            return behavior::ExecuteResult::Failure;
        }





        behavior::ExecuteResult DevaMoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.bot->GetGame();
            auto& bb = ctx.blackboard;
            
            float energy_pct = game.GetEnergy() / (float)game.GetShipSettings().MaximumEnergy;

            const Player* target = bb.ValueOr<const Player*>("Target", nullptr);
            if (!target) { return behavior::ExecuteResult::Failure; }

            float player_energy_pct = target->energy / (float)game.GetShipSettings().MaximumEnergy;
            

            bool in_safe = game.GetMap().GetTileId(game.GetPlayer().position) == marvin::kSafeTileId;

            Vector2f shot_position = bb.ValueOr<Vector2f>("Solution", Vector2f());

            
            float hover_distance = 0.0f;
           

            if (in_safe) hover_distance = 0.0f;
            else {
                if (bb.ValueOr<bool>("InCenter", true)) {

                    float diff = (energy_pct - player_energy_pct) * 10.0f;
                    hover_distance = 10.0f - diff;
                    if (hover_distance < 0.0f) hover_distance = 0.0f;

                }
                else {
                    if (bb.ValueOr<bool>("IsAnchor", false)) {
                        hover_distance = 20.0f;
                    }
                    else {
                        hover_distance = 7.0f;
                    }
                }
            }

            ctx.bot->Move(shot_position, hover_distance);
          
            ctx.bot->GetSteering().Face(shot_position);

            return behavior::ExecuteResult::Success;
        }

        bool DevaMoveToEnemyNode::IsAimingAt(GameProxy& game, const Player& shooter, const Player& target, Vector2f* dodge) {
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