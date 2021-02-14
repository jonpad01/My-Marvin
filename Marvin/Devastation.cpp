#include <chrono>
#include <cstring>
#include <limits>

//#include <iostream>
//#include <string>

#include "Devastation.h"
#include "Debug.h"
#include "GameProxy.h"
#include "Map.h"
#include "RayCaster.h"
#include "RegionRegistry.h"
#include "platform/Platform.h"
#include "platform/ContinuumGameProxy.h"
#include "Shooter.h"



namespace marvin {

    Devastation::Devastation(std::unique_ptr<GameProxy> game) : game_(std::move(game)), keys_(time_.GetTime()), steering_(*game_, keys_) {
        auto processor = std::make_unique<path::NodeProcessor>(*game_);

        if (game_->GetName() == "LilMarv") {
            ctx_.blackboard.SetShip(1);
        }
        else {
            ctx_.blackboard.SetShip(8);
        }

        pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor));
        regions_ = RegionRegistry::Create(game_->GetMap());
        pathfinder_->CreateMapWeights(game_->GetMap());


        
        auto find_enemy = std::make_unique<deva::FindEnemyNode>();
        auto looking_at_enemy = std::make_unique<deva::LookingAtEnemyNode>();
        auto target_in_los = std::make_unique<deva::InLineOfSightNode>();

        auto shoot_enemy = std::make_unique<deva::ShootEnemyNode>();
        auto path_to_enemy = std::make_unique<deva::PathToEnemyNode>();
        auto move_to_enemy = std::make_unique<deva::MoveToEnemyNode>();
        auto follow_path = std::make_unique<deva::FollowPathNode>();
        auto patrol = std::make_unique<deva::PatrolNode>();

        auto move_method_selector = std::make_unique<behavior::SelectorNode>(move_to_enemy.get());
        auto shoot_sequence = std::make_unique<behavior::SequenceNode>(looking_at_enemy.get(), shoot_enemy.get());
        auto parallel_shoot_enemy = std::make_unique<behavior::ParallelNode>(shoot_sequence.get(), move_method_selector.get());
        auto los_shoot_conditional = std::make_unique<behavior::SequenceNode>(target_in_los.get(), parallel_shoot_enemy.get());
        auto enemy_path_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy.get(), follow_path.get());
        auto patrol_path_sequence = std::make_unique<behavior::SequenceNode>(patrol.get(), follow_path.get());
        auto path_or_shoot_selector = std::make_unique<behavior::SelectorNode>(los_shoot_conditional.get(), enemy_path_sequence.get());
        auto handle_enemy = std::make_unique<behavior::SequenceNode>(find_enemy.get(), path_or_shoot_selector.get());

        auto action_selector = std::make_unique<behavior::SelectorNode>(handle_enemy.get(), patrol_path_sequence.get());

        auto toggle_status = std::make_unique<deva::ToggleStatusNode>();
        auto attach = std::make_unique<deva::AttachNode>();
        auto warp = std::make_unique<deva::WarpNode>();
        auto set_freq = std::make_unique<deva::SetFreqNode>();
        auto set_ship = std::make_unique<deva::SetShipNode>();
        auto commands = std::make_unique<deva::CommandNode>();

        auto root_sequence = std::make_unique<behavior::SequenceNode>(commands.get(), set_ship.get(), set_freq.get(), warp.get(), attach.get(), toggle_status.get(), action_selector.get());

        
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
        behavior_nodes_.push_back(std::move(action_selector));

        behavior_nodes_.push_back(std::move(toggle_status));
        behavior_nodes_.push_back(std::move(attach));
        behavior_nodes_.push_back(std::move(warp));
        behavior_nodes_.push_back(std::move(set_freq));
        behavior_nodes_.push_back(std::move(set_ship));
        behavior_nodes_.push_back(std::move(commands));
        behavior_nodes_.push_back(std::move(root_sequence));

        behavior_ = std::make_unique<behavior::BehaviorEngine>(behavior_nodes_.back().get());

        CreateBasePaths();
        ctx_.deva = this;
    }

    void Devastation::Update(float dt) {

        keys_.ReleaseAll();
        game_->Update(dt);

        //game_->SetEnergy(50, game_->GetShipSettings().MaximumEnergy);

        //check chat for disconected message and terminate continuum
     
        std::string disconnected = "WARNING: ";
        
        Chat chat = game_->GetChat();

        if (chat.type == 0) {
            if (chat.message.compare(0, 9, disconnected) == 0) {
                exit(5);
            }
        }

        SortPlayers();
        SetRegion();

        steering_.Reset();

        ctx_.dt = dt;
#if !DEBUG_NO_BEHAVIOR
        behavior_->Update(ctx_);
#endif
#if DEBUG_RENDER
         //common_.RenderPath(GetGame(), ctx_.blackboard);
#endif
#if !DEBUG_NO_ARROWS
        steering_.Steer();
#endif
    }

    void Devastation::Move(const Vector2f& target, float target_distance) {
        const Player& bot_player = game_->GetPlayer();
        float distance = bot_player.position.Distance(target);

        if (distance > target_distance) {

            steering_.Seek(target);
        }

        else if (distance <= target_distance) {

            Vector2f to_target = target - bot_player.position;

            steering_.Seek(target - Normalize(to_target) * target_distance);
        }

    }



    void Devastation::SetRegion() {

        in_center_ = GetRegions().IsConnected((MapCoord)game_->GetPosition(), MapCoord(512, 512));

        if (!in_center_ && game_->GetPlayer().active) {
            if (!GetRegions().IsConnected((MapCoord)game_->GetPosition(), (MapCoord)deva::team0_safes[base_index_])) {

                for (std::size_t i = 0; i < deva::team0_safes.size(); i++) {
                    if (GetRegions().IsConnected((MapCoord)game_->GetPosition(), (MapCoord)deva::team0_safes[i])) {
                        base_index_ = i;
                        if (game_->GetPlayer().frequency == 00) {
                            team_safe = marvin::deva::team0_safes[i];
                            enemy_safe = marvin::deva::team1_safes[i];
                        }
                        else if (game_->GetPlayer().frequency == 01) {
                            team_safe = marvin::deva::team1_safes[i];
                            enemy_safe = marvin::deva::team0_safes[i];
                        }
                        return;
                    }
                }
            }
        }
    }



    void Devastation::SortPlayers() {

        const Player& bot = game_->GetPlayer();

        freq_list.clear();
        freq_list.resize(100, 0);
        team.clear();
        enemy_team.clear();
        duelers.clear();
        last_bot_standing = true;
        team_in_base_ = false;
        enemy_in_base_ = false;

        //find team sizes and push team mates into a vector
        for (std::size_t i = 0; i < game_->GetPlayers().size(); i++) {
            const Player& player = game_->GetPlayers()[i];

            if (player.frequency < 100) freq_list[player.frequency]++;

            if ((player.frequency == 00 || player.frequency == 01) && (player.id != bot.id)) {

                duelers.push_back(game_->GetPlayers()[i]);

                if (player.frequency == bot.frequency) {
                    team.push_back(game_->GetPlayers()[i]);

                    bool in_center = regions_->IsConnected((MapCoord)player.position, MapCoord(512, 512));
                    if (!in_center && IsValidPosition(player.position)) {
                        team_in_base_ = true;
                    }
                }
                else {
                    enemy_team.push_back(game_->GetPlayers()[i]);

                    bool in_center = regions_->IsConnected((MapCoord)player.position, MapCoord(512, 512));
                    if (!in_center && IsValidPosition(player.position)) {
                        enemy_in_base_ = true;
                    }
                }
            }
        }

        if (team.size() == 0) {
            last_bot_standing = false;
        }
        else {
            //check if team is in base
            for (std::size_t i = 0; i < team.size(); i++) {
                const Player& player = team[i];

                if (!GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512)) && player.active) {
                    last_bot_standing = false;
                    break;
                }
            }
        }
    }



    void Devastation::CreateBasePaths() {
        base_paths;
        std::vector<Vector2f> mines;

        for (std::size_t i = 0; i < marvin::deva::team0_safes.size(); i++) {

            Vector2f safe_1 = marvin::deva::team0_safes[i];
            Vector2f safe_2 = marvin::deva::team1_safes[i];

            float radius = 0.8f; // wont fit through 3 tile holes

            Path base_path = GetPathfinder().FindPath(game_->GetMap(), mines, safe_1, safe_2, radius);
            base_path = GetPathfinder().SmoothPath(base_path, game_->GetMap(), radius);

            base_paths.push_back(base_path);
        }
    }


    uint64_t Devastation::GetUniqueTimer() {

        uint64_t offset = 200;

        for (std::size_t i = 0; i < deva::bot_names.size(); i++) {
            if (game_->GetPlayer().name == deva::bot_names[i]) {
                offset = 100 + (((uint64_t)i + 1) * 100);
            }
        }
        return offset;
    }


    void Devastation::LookForWallShot(Vector2f target_pos, Vector2f target_vel, float proj_speed, int alive_time, uint8_t bounces) {


        for (std::size_t i = 0; i < 1; i++) {

            int left_rotation = game_->GetPlayer().discrete_rotation - i;
            int right_rotation = game_->GetPlayer().discrete_rotation + i;

            if (left_rotation < 0) {
                left_rotation += 40;
            }
            if (right_rotation > 39) {
                right_rotation -= 40;
            }

            Vector2f left_trajectory = (game_->GetPlayer().ConvertToHeading(left_rotation) * proj_speed) + game_->GetPlayer().velocity;
            Vector2f right_trajectory = (game_->GetPlayer().ConvertToHeading(right_rotation) * proj_speed) + game_->GetPlayer().velocity;

            Vector2f left_direction = Normalize(left_trajectory);
            Vector2f right_direction = Normalize(right_trajectory);

           
        }

    }

 

    namespace deva {


        behavior::ExecuteResult CommandNode::Execute(behavior::ExecuteContext& ctx) {

        auto& game = ctx.deva->GetGame();

        Chat chat = game.GetChat();

        if (ctx.blackboard.GetChatCount() == chat.count) {
            return behavior::ExecuteResult::Success;
        }
        else {
            ctx.blackboard.SetChatCount(chat.count);
        }

        bool mod = chat.player == "tm_master" || chat.player == "Baked Cake" || chat.player == "X-Demo" || chat.player == "Lyra." || chat.player == "Profile" || chat.player == "Monkey";
        
        bool mArena = chat.type == 0;
        bool mPublic = chat.type == 2;
        bool mPrivate = chat.type == 5;
        bool mChannel = chat.type == 9;
        

        if (chat.message == "!help" && mPrivate) {

            game.SendChatMessage(":" + chat.player + ":Command List:");
            game.SendChatMessage(":" + chat.player + ":");
            game.SendChatMessage(":" + chat.player + ":!lockmarv - only mods can send bot commands (mod only command)");
            game.SendChatMessage(":" + chat.player + ":!unlockmarv - anyone can send commands (mod only command)");
            game.SendChatMessage(":" + chat.player + ":!lockfreq - prevent bot from hopping freqs automatically");
            game.SendChatMessage(":" + chat.player + ":!unlockfreq - allow bot to hop freqs to balance teams");
            game.SendChatMessage(":" + chat.player + ":!specmarv - send marv to spec");
            game.SendChatMessage(":" + chat.player + ":!setship # - specify the ship marv should switch to");
            game.SendChatMessage(":" + chat.player + ":!setfreq # - usefull for pulling bots back onto dueling freqs");
        }

        if (chat.message == "!lockmarv" && mod) {

            if (ctx.blackboard.GetChatLock() == false) { 

                ctx.blackboard.SetChatLock(true); 
                
                if (chat.type == 2) {
                    game.SendChatMessage("Marv Locked.");
                }
            }
            if (chat.type == 5) {
                game.SendChatMessage(":" + chat.player + ":Marv Locked.");
            }
        }

        if (chat.message == "!unlockmarv" && mod) {

            if (ctx.blackboard.GetChatLock() == true) {

                ctx.blackboard.SetChatLock(false);

                if (chat.type == 2) {
                    game.SendChatMessage("Marv Unlocked.");
                }
            }
            if (chat.type == 5) {
                game.SendChatMessage(":" + chat.player + ":Marv Unlocked.");
            }
        }

        bool locked = ctx.blackboard.GetChatLock();

        if (chat.message == "!lockfreq" && (mod || !locked)) {

            if (ctx.blackboard.GetFreqLock() == false) {

                ctx.blackboard.SetFreqLock(true);

                if (chat.type == 2) {
                    game.SendChatMessage("Freq Locked.");
                }
            }
            if (chat.type == 5) {
                game.SendChatMessage(":" + chat.player + ":Freq Locked.");
            }
        }

        if (chat.message == "!unlockfreq" && (mod || !locked)) {

            if (ctx.blackboard.GetFreqLock() == true) {

                ctx.blackboard.SetFreqLock(false);

                if (chat.type == 2) {
                    game.SendChatMessage("Freq Unlocked.");
                }
            }
            if (chat.type == 5) {
                game.SendChatMessage(":" + chat.player + ":Freq Unlocked.");
            }
        }



        if (chat.message == "!specmarv" && (mod || !locked)) {
            ctx.blackboard.SetShip(8);
        }

        if (chat.message.compare(0, 9, "!setship ") == 0 && (mod || !locked)) {
            int number = 12;
            if (std::isdigit(chat.message[9])) {
                number = (std::stoi(std::string(1, chat.message[9])) - 1);
            }
            if (number >= 0 && number < 8) {
                ctx.blackboard.SetShip(number);
            }
        }

        if (chat.message.compare(0, 9, "!setfreq ") == 0 && (mod || !locked)) {
            int number = 111;
            if (std::isdigit(chat.message[9])) {

                std::string freq{ chat.message[9], chat.message[10] };

                number = std::stoi(freq);
            }
            if (number >= 0 && number < 100) {
                ctx.blackboard.SetFreq(number);
            }
        }

        return behavior::ExecuteResult::Success;
    }


    behavior::ExecuteResult SetShipNode::Execute(behavior::ExecuteContext& ctx) {

        auto& game = ctx.deva->GetGame();

        int cShip = (int)game.GetPlayer().ship;
        int dShip = ctx.blackboard.GetShip();

        uint64_t ship_cooldown = 3000;

        if (cShip != dShip) {

            if (ctx.deva->GetTime().TimedActionDelay(ship_cooldown, "ship")) {
                if (cShip == 8 || CheckStatus(game, ctx.deva->GetKeys(), true)) {
                    if (!game.SetShip(dShip)) {
                        ctx.deva->GetTime().TimedActionDelay(0, "ship");
                    }
                }
                else {
                    ctx.deva->GetTime().TimedActionDelay(0, "ship");
                }
            }
            return behavior::ExecuteResult::Failure;
        }
        return behavior::ExecuteResult::Success;
    }



    behavior::ExecuteResult SetFreqNode::Execute(behavior::ExecuteContext& ctx) {

        auto& game = ctx.deva->GetGame();

        if (ctx.blackboard.GetFreq() != 999) {
            if (ctx.deva->GetTime().TimedActionDelay(ctx.deva->GetUniqueTimer(), "tempchange")) {

                if (CheckStatus(game, ctx.deva->GetKeys(), true)) {
                    game.SetFreq(ctx.blackboard.GetFreq());
                    ctx.blackboard.SetFreq(999);
                }
                else {
                    ctx.deva->GetTime().TimedActionDelay(0, "tempchange");
                }
            }
            return behavior::ExecuteResult::Failure;
        }
        if (ctx.blackboard.GetFreqLock()) {
            return behavior::ExecuteResult::Success;
        }

        //if player is on a non duel team size greater than 2, breaks the zone balancer
        if (game.GetPlayer().frequency != 00 && game.GetPlayer().frequency != 01) {
            if (ctx.deva->GetFreqList()[game.GetPlayer().frequency] > 1) {

                if (ctx.deva->GetTime().TimedActionDelay(ctx.deva->GetUniqueTimer(), "splitteam")) {

                    if (CheckStatus(game, ctx.deva->GetKeys(), true)) {
                        game.SetFreq(FindOpenFreq(ctx.deva->GetFreqList(), 2));  
                    }   
                    else {
                        ctx.deva->GetTime().TimedActionDelay(0, "splitteam");
                    }
                }
                return behavior::ExecuteResult::Failure;
            }
        }

        //duel team is uneven
        if (ctx.deva->GetFreqList()[0] != ctx.deva->GetFreqList()[1]) {
            if (game.GetPlayer().frequency != 00 && game.GetPlayer().frequency != 01) {

                if (ctx.deva->GetTime().TimedActionDelay((ctx.deva->GetUniqueTimer() / 2), "getin")) {

                    if (CheckStatus(game, ctx.deva->GetKeys(), true)) {
                        //get on freq 0
                        if (ctx.deva->GetFreqList()[0] < ctx.deva->GetFreqList()[1]) {
                            game.SetFreq(0);
                        }
                        //get on freq 1
                        else {
                            game.SetFreq(1);
                        }
                    }
                    else {
                        ctx.deva->GetTime().TimedActionDelay(0, "getin");
                    }
                }
                return behavior::ExecuteResult::Failure;
            }
            //bot is on a duel team
            else if (ctx.deva->InCenter() || ctx.deva->GetDevaTeam().size() == 0 || ctx.deva->TeamInBase()) {

                if ((game.GetPlayer().frequency == 00 && ctx.deva->GetFreqList()[0] > ctx.deva->GetFreqList()[1]) ||
                    (game.GetPlayer().frequency == 01 && ctx.deva->GetFreqList()[0] < ctx.deva->GetFreqList()[1])) {
                    //#if 0
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
                    //#endif

                    if (ctx.deva->GetTime().TimedActionDelay(ctx.deva->GetUniqueTimer(), "getout")) {

                        if (CheckStatus(game, ctx.deva->GetKeys(), true)) {
                            game.SetFreq(FindOpenFreq(ctx.deva->GetFreqList(), 2));
                        }
                        else {
                            ctx.deva->GetTime().TimedActionDelay(0, "getout");
                        }
                    }
                    return behavior::ExecuteResult::Failure;
                }
            }
        }
        return behavior::ExecuteResult::Success;
    }



    behavior::ExecuteResult WarpNode::Execute(behavior::ExecuteContext& ctx) {

        auto& game = ctx.deva->GetGame();

        uint64_t base_empty_timer = 30000;

        if (!ctx.deva->InCenter()) {
            if (!ctx.deva->EnemyInBase()) {

                if (ctx.deva->GetTime().TimedActionDelay(base_empty_timer, "warp")) {

                    if (CheckStatus(game, ctx.deva->GetKeys(), true)) {
                        game.Warp();
                    }
                    else {
                        ctx.deva->GetTime().TimedActionDelay(0, "warp");
                    }
                }             
            }
        }
        return behavior::ExecuteResult::Success;
    }

    behavior::ExecuteResult AttachNode::Execute(behavior::ExecuteContext& ctx) {

        auto& game = ctx.deva->GetGame();

        //if dueling then run checks for attaching
        if (game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01) {

            //if this check is valid the bot will halt here untill it attaches
            if (ctx.deva->InCenter() && ctx.deva->TeamInBase()) {

                SetAttachTarget(ctx);

                //checks if energy is full
                if (CheckStatus(game, ctx.deva->GetKeys(), true)) {
                    game.F7();
                }
                return behavior::ExecuteResult::Failure;
            }
        }
        //if attached to a player detach
        if (game.GetPlayer().attach_id != 65535) {
            game.F7();
            return behavior::ExecuteResult::Failure;
        }
        return behavior::ExecuteResult::Success;
    }

    void AttachNode::SetAttachTarget(behavior::ExecuteContext& ctx) {
        auto& game = ctx.deva->GetGame();

        std::vector<Vector2f> path = ctx.deva->GetBasePath();

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
        for (std::size_t i = 0; i < ctx.deva->GetDevaDuelers().size(); i++) {
            const Player& player = ctx.deva->GetDevaDuelers()[i];
            bool player_in_center = ctx.deva->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));
            if (!player_in_center && IsValidPosition(player.position) && player.ship < 8) {

                float distance_to_team = 0.0f;
                float distance_to_enemy = 0.0f;

                if (!PathLength(path, player.position, ctx.deva->GetTeamSafe(), &distance_to_team)) { return; }
                if (!PathLength(path, player.position, ctx.deva->GetEnemySafe(), &distance_to_enemy)) { return; }

                if (player.frequency == game.GetPlayer().frequency) {
                    //float distance_to_team = ctx.deva->PathLength(player.position, ctx.deva->GetTeamSafe());
                    //float distance_to_enemy = ctx.deva->PathLength(player.position, ctx.deva->GetEnemySafe());

                    if (distance_to_team < closest_team_distance_to_team) {
                        closest_team_distance_to_team = distance_to_team;
                        closest_team_to_team = ctx.deva->GetDevaDuelers()[i].id;
                    }
                    if (distance_to_enemy < closest_team_distance_to_enemy) {
                        closest_team_distance_to_enemy = distance_to_enemy;
                        closest_team_to_enemy = ctx.deva->GetDevaDuelers()[i].id;
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
                        closest_enemy_to_enemy = ctx.deva->GetDevaDuelers()[i].id;
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
            game.SetSelectedPlayer(closest_team_to_team);
            return;
        }

        //find closest team player to the enemy closest to team safe
        for (std::size_t i = 0; i < ctx.deva->GetDevaTeam().size(); i++) {
            const Player& player = ctx.deva->GetDevaTeam()[i];

            bool player_in_center = ctx.deva->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));

            //as long as the player is not in center and has a valid position, it will hold its position for a moment after death
            if (!player_in_center && IsValidPosition(player.position) && player.ship < 8) {

                // float distance_to_enemy_position = ctx.deva->PathLength(player.position, closest_enemy_to_team);

                float distance_to_enemy_position = 0.0f;

                if (!PathLength(path, player.position, closest_enemy_to_team, &distance_to_enemy_position)) { return; }

                //get the closest player
                if (distance_to_enemy_position < closest_distance) {
                    closest_distance = distance_to_enemy_position;
                    game.SetSelectedPlayer(ctx.deva->GetDevaTeam()[i].id);
                }
            }
        }
    }


        behavior::ExecuteResult ToggleStatusNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.deva->GetGame();

            //RenderText(text.c_str(), GetWindowCenter() - Vector2f(0, 200), RGB(100, 100, 100), RenderText_Centered);

            bool x_active = (game.GetPlayer().status & 4) != 0;
            bool stealthing = (game.GetPlayer().status & 1) != 0;
            bool cloaking = (game.GetPlayer().status & 2) != 0;

            bool has_xradar = (game.GetShipSettings().XRadarStatus & 3) != 0;
            bool has_stealth = (game.GetShipSettings().StealthStatus & 1) != 0;
            bool has_cloak = (game.GetShipSettings().CloakStatus & 3) != 0;

            //if stealth isnt on but availible, presses home key in continuumgameproxy.cpp
            if (!stealthing && has_stealth) {
                game.Stealth();
                return behavior::ExecuteResult::Failure;
            }
            //same as stealth but presses shift first
            if (!cloaking && has_cloak) {
                game.Cloak(ctx.deva->GetKeys());
                return behavior::ExecuteResult::Failure;
            }
            //in deva xradar is free so just turn it on
            if (!x_active && has_xradar) {
                game.XRadar();
                return behavior::ExecuteResult::Failure;
            }
            return behavior::ExecuteResult::Success;
        }



        behavior::ExecuteResult FindEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            behavior::ExecuteResult result = behavior::ExecuteResult::Failure;
            float closest_cost = std::numeric_limits<float>::max();
            float cost = 0;
            auto& game = ctx.deva->GetGame();
            std::vector< Player > players = game.GetPlayers();
            const Player* target = nullptr;
            const Player& bot = ctx.deva->GetGame().GetPlayer();

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
            const auto& game = ctx.deva->GetGame();
            const Player& bot_player = game.GetPlayer();

            if (!ctx.deva->InCenter() && ctx.deva->LastBotStanding()) return false;
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

            if (!ctx.deva->GetRegions().IsConnected(bot_coord, target_coord)) {
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
            auto& game = ctx.deva->GetGame();
            float energy_pct = (game.GetEnergy() / (float)game.GetShipSettings().MaximumEnergy) * 100.0f;
            
            if (energy_pct < 25.0f && ctx.deva->InCenter() && game.GetPlayer().repels > 0) {
                game.Repel(ctx.deva->GetKeys());
            }

            Vector2f bot = game.GetPosition();

            const Player* enemy = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            if (!enemy) { return behavior::ExecuteResult::Failure; }

            Path path = ctx.blackboard.ValueOr<Path>("path", Path());
            float radius = game.GetShipSettings().GetRadius();
#if 0
            std::string enemy_ = "Enemy Target: " + std::to_string(ctx.deva->PathLength(bot, enemy));
            std::string teamsafe = "Team Safe: " + std::to_string(ctx.deva->PathLength(bot, ctx.deva->GetTeamSafe()));
            std::string enemysafe = "Enemy Safe: " + std::to_string(ctx.deva->PathLength(bot, ctx.deva->GetEnemySafe()));

            RenderText(enemy_, GetWindowCenter() - Vector2f(200, 300), RGB(100, 100, 100), RenderText_Centered);
            RenderText(teamsafe, GetWindowCenter() - Vector2f(0, 300), RGB(100, 100, 100), RenderText_Centered);
            RenderText(enemysafe, GetWindowCenter() - Vector2f(-200, 300), RGB(100, 100, 100), RenderText_Centered);
#endif
            if (!ctx.deva->InCenter()) {
                if (game.GetPlayer().ship == 2) {
                    float pathlength = 0.0f;
                    PathLength(ctx.deva->GetBasePath(), bot, enemy->position, &pathlength);
                    if (pathlength < 45.0f || ctx.deva->LastBotStanding()) {
                    //if (ctx.deva->PathLength(bot, enemy) < 45.0f || ctx.deva->LastBotStanding()) {
                        path = ctx.deva->GetPathfinder().CreatePath(path, bot, ctx.deva->GetTeamSafe(), radius);
                    }
                    else path = ctx.deva->GetPathfinder().CreatePath(path, bot, enemy->position, radius);
                }
                else if (ctx.deva->LastBotStanding()) {
                    path = ctx.deva->GetPathfinder().CreatePath(path, bot, ctx.deva->GetTeamSafe(), radius);
                }
                else path = ctx.deva->GetPathfinder().CreatePath(path, bot, enemy->position, radius);
            }
            else path = ctx.deva->GetPathfinder().CreatePath(path, bot, enemy->position, radius);

            ctx.blackboard.Set<Path>("path", path);
            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult PatrolNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.deva->GetGame();
            Vector2f from = game.GetPosition();
            Path path = ctx.blackboard.ValueOr<Path>("path", Path());
            float radius = game.GetShipSettings().GetRadius();
 
            std::vector<Vector2f> nodes;
            int index = ctx.blackboard.ValueOr<int>("patrol_index", 0);
            int taunt_index = ctx.blackboard.ValueOr<int>("taunt_index", 0);
            uint64_t time = ctx.deva->GetTime().GetTime();
            uint64_t hoorah_expire = ctx.blackboard.ValueOr<uint64_t>("HoorahExpire", 0);
            bool in_safe = game.GetMap().GetTileId(game.GetPosition()) == kSafeTileId;

            if (ctx.deva->InCenter()) {
                nodes = { Vector2f(568, 568), Vector2f(454, 568), Vector2f(454, 454), Vector2f(568, 454), Vector2f(568, 568),
                Vector2f(454, 454), Vector2f(568, 454), Vector2f(454, 568), Vector2f(454, 454), Vector2f(568, 568), Vector2f(454, 568), Vector2f(568, 454) };
            }
            else {
                if (ctx.deva->LastBotStanding()) {
                    path = ctx.deva->GetPathfinder().CreatePath(path, from, ctx.deva->GetTeamSafe(), radius);
                    ctx.blackboard.Set<Path>("path", path);
                    return behavior::ExecuteResult::Success;
                }
                else {
                    path = ctx.deva->GetPathfinder().CreatePath(path, from, ctx.deva->GetEnemySafe(), radius);
                    ctx.blackboard.Set<Path>("path", path);
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

            path = ctx.deva->GetPathfinder().CreatePath(path, from, to, radius);
            ctx.blackboard.Set<Path>("path", path);

            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult FollowPathNode::Execute(behavior::ExecuteContext& ctx) {
            auto path = ctx.blackboard.ValueOr<Path>("path", std::vector<Vector2f>());

            size_t path_size = path.size();
            auto& game = ctx.deva->GetGame();

            if (path.empty()) { return behavior::ExecuteResult::Failure; }

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

            if (path.size() == 1 && path.front().DistanceSq(game.GetPosition()) < 2.0f * 2.0f) {
                path.clear();
            }

            if (path.size() != path_size) {
                ctx.blackboard.Set("path", path);
            }

            ctx.deva->Move(current, 0.0f);

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

            const Player* target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
            if (!target_player) { return behavior::ExecuteResult::Failure; }

            auto& game = ctx.deva->GetGame();

            auto to_target = target_player->position - game.GetPosition();
            Vector2f direction = Normalize(to_target);
            Vector2f side = Perpendicular(direction);
            float radius = game.GetShipSettings().GetRadius();

            CastResult center = RayCast(game.GetMap(), game.GetPosition(), direction, to_target.Length());

            if (!center.hit) {

                CastResult side1 = RayCast(game.GetMap(), game.GetPosition() + side * radius, direction, to_target.Length());
                CastResult side2 = RayCast(game.GetMap(), game.GetPosition() - side * radius, direction, to_target.Length());

                if (!side1.hit && !side2.hit) {
                    result = behavior::ExecuteResult::Success;
                    //if bot is not in center, almost dead, and in line of sight of its target burst
                    bool in_center = ctx.deva->GetRegions().IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));
                    float energy_pct = ctx.deva->GetGame().GetEnergy() / (float)ctx.deva->GetGame().GetShipSettings().MaximumEnergy;
                    if (energy_pct < 0.10f && !in_center && game.GetPlayer().bursts > 0) {
                        ctx.deva->GetGame().Burst(ctx.deva->GetKeys());
                    }
                }
            }
            else {
         
                
                    float target_radius = game.GetSettings().ShipSettings[target_player->ship].GetRadius();
                    bool shoot = false;
                    bool bomb_hit = false;
                    Vector2f wall_pos;
                      
                        if (BounceShot(game, target_player->position, target_player->velocity, target_radius, game.GetPlayer().GetHeading(), &bomb_hit, &wall_pos)) {
                            if (game.GetMap().GetTileId(game.GetPosition()) != marvin::kSafeTileId) {
                                if (bomb_hit) {
                                    ctx.deva->GetKeys().Press(VK_TAB);
                                }
                                else {
                                    ctx.deva->GetKeys().Press(VK_CONTROL);
                                }
                            }
                        }
                   
                
            }

            return result;
        }




        behavior::ExecuteResult LookingAtEnemyNode::Execute(behavior::ExecuteContext& ctx) {

            const auto target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            if (!target_player) { return behavior::ExecuteResult::Failure; }

            const Player& target = *target_player;
            auto& game = ctx.deva->GetGame();

            const Player& bot_player = game.GetPlayer(); 

            float target_radius = game.GetSettings().ShipSettings[target.ship].GetRadius();
            float radius = game.GetShipSettings().GetRadius();
            float proj_speed = game.GetSettings().ShipSettings[bot_player.ship].BulletSpeed / 10.0f / 16.0f;

            Vector2f solution;
        

            if (!CalculateShot(game.GetPosition(), target.position, bot_player.velocity, target.velocity, proj_speed, &solution)) {
                ctx.blackboard.Set<Vector2f>("shot_position", solution);
                ctx.blackboard.Set<bool>("has_shot", false);
                return behavior::ExecuteResult::Failure;
            }

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
                if (CanShootGun(game, game.GetMap(), game.GetPosition(), solution)) {
                    return behavior::ExecuteResult::Success;
                }
            }
            return behavior::ExecuteResult::Failure;
        }




        behavior::ExecuteResult ShootEnemyNode::Execute(behavior::ExecuteContext& ctx) {
      
            ctx.deva->GetKeys().Press(VK_CONTROL);

            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult MoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.deva->GetGame();
            
            float energy_pct = game.GetEnergy() / (float)game.GetShipSettings().MaximumEnergy;

            const Player* target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
            if (!target_player) { return behavior::ExecuteResult::Failure; }

            float player_energy_pct = target_player->energy / (float)game.GetShipSettings().MaximumEnergy;
            
            float energy_check = ctx.blackboard.ValueOr<float>("EnergyCheck", 0);
            bool emped = ctx.blackboard.ValueOr<bool>("Emped", false);
            
            uint64_t time = ctx.deva->GetTime().GetTime();
            uint64_t energy_time_check = ctx.blackboard.ValueOr<uint64_t>("EnergyTimeCheck", 0);
            uint64_t emp_cooldown_check = ctx.blackboard.ValueOr<uint64_t>("EmpCooldown", 0);
            uint64_t repel_cooldown = ctx.blackboard.ValueOr<uint64_t>("RepelCooldown", 0);
            uint64_t repel_check = ctx.blackboard.ValueOr<uint64_t>("RepelCheck", 0);

            bool in_safe = game.GetMap().GetTileId(game.GetPlayer().position) == marvin::kSafeTileId;

            Vector2f target_position = ctx.blackboard.ValueOr("target_position", Vector2f());
            Vector2f shot_position = ctx.blackboard.ValueOr("shot_position", Vector2f());
            bool has_shot = ctx.blackboard.ValueOr<bool>("has_shot", true);


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
                            if (weapon_position.Distance(game.GetPosition()) < 8.0f && game.GetPlayer().repels > 0) {
                                //if (time > repel_cooldown) {
                                    game.Repel(ctx.deva->GetKeys());
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
                if (ctx.deva->InCenter()) {
                    if (emped) {
                        hover_distance = 30.0f;
                    }
                    else {
                        float diff = (energy_pct - player_energy_pct) * 10.0f;
                        hover_distance = 10.0f - diff;
                        if (hover_distance < 0.0f) hover_distance = 0.0f;
                    }
                }
                else {
                    if (game.GetPlayer().ship == 2) {
                        hover_distance = 10.0f;
                    }
                    else {
                        hover_distance = 7.0f;
                    }
                }
            }

           // if (!has_shot && hover_distance < target_player.position.Distance(game.GetPosition())) {
           //     hover_distance = target_player.position.Distance(game.GetPosition());



            ctx.deva->Move(shot_position, hover_distance);
          
            ctx.deva->GetSteering().Face(shot_position);

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