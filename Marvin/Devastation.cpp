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



namespace marvin {
    //namespace deva {

    Devastation::Devastation(std::shared_ptr<GameProxy> game, std::shared_ptr<GameProxy> game2, std::shared_ptr<GameProxy> game3) : game_(std::move(game)), steering_(std::move(game2)), common_(std::move(game3)), keys_(0) {

        auto processor = std::make_unique<path::NodeProcessor>(*game_);

        last_ship_change_ = 0;
        ship_ = game_->GetPlayer().ship;

        if (ship_ > 7) {
            ship_ = 1;
        }

        pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor));
        regions_ = RegionRegistry::Create(game_->GetMap());
        pathfinder_->CreateMapWeights(game_->GetMap());


        auto freq_warp_attach = std::make_unique<deva::FreqWarpAttachNode>();
        auto find_enemy = std::make_unique<deva::FindEnemyNode>();
        auto looking_at_enemy = std::make_unique<deva::LookingAtEnemyNode>();
        auto target_in_los = std::make_unique<deva::InLineOfSightNode>(
            [](marvin::behavior::ExecuteContext& ctx) {
                const Vector2f* result = nullptr;

                const Player* target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

                if (target_player) {
                    result = &target_player->position;
                }
                return result;
            });

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

        CreateBasePaths();
        ctx_.deva = this;
        ctx_.com = &common_;
    }

    void Devastation::Update(float dt) {

        keys_.ReleaseAll();
        //keys_.Update(common_.GetTime());
        game_->Update(dt);

        //game_->SetEnergy(50, game_->GetShipSettings().MaximumEnergy);

        has_wall_shot = false;

        uint64_t timestamp = common_.GetTime();
        uint64_t ship_cooldown = 10800000;

        //check chat for disconected message and terminate continuum
     
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


        SortPlayers();
        SetRegion();

        steering_.Reset();

        ctx_.dt = dt;

        behavior_->Update(ctx_);

#if DEBUG_RENDER
         //common_.RenderPath(GetGame(), ctx_.blackboard);
#endif

        Steer();
    }


    void Devastation::Steer() {

        Vector2f center = GetWindowCenter();
        float debug_y = 100.0f;
        Vector2f force = steering_.GetSteering();
        float rotation = steering_.GetRotation();
        //RenderText("force" + std::to_string(force.Length()), center - Vector2f(0, debug_y), RGB(100, 100, 100), RenderText_Centered);
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
            if (behind) { keys_.Press(VK_DOWN, common_.GetTime(), 30); }
            else { keys_.Press(VK_UP, common_.GetTime(), 30); }
        }

        //above 0.996 and bot is constantly correcting creating a wobble
        if (heading.Dot(steering_direction) < 0.996f) {  //1.0f

            if (clockwise) {
              //  if (!keys_.IsPressed(VK_LEFT)) {
                    keys_.Press(VK_RIGHT, common_.GetTime(), 30);
               // }
            }
            else {
              //  if (!keys_.IsPressed(VK_RIGHT)) {
                    keys_.Press(VK_LEFT, common_.GetTime(), 30);
              //  }
            }


            //ensure that only one arrow key is pressed at any given time
            //keys_.Set(VK_RIGHT, clockwise, GetTime());
            //keys_.Set(VK_LEFT, !clockwise, GetTime());
        }
#if 0
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
            if (!GetRegions().IsConnected((MapCoord)game_->GetPosition(), (MapCoord)current_base_)) {

                for (std::size_t i = 0; i < deva::team0_safes.size(); i++) {
                    if (GetRegions().IsConnected((MapCoord)game_->GetPosition(), (MapCoord)deva::team0_safes[i])) {
                        base_index_ = i;
                        current_base_ = deva::team0_safes[i];
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

        //find team sizes and push team mates into a vector
        for (std::size_t i = 0; i < game_->GetPlayers().size(); i++) {
            const Player& player = game_->GetPlayers()[i];

            if (player.frequency < 100) freq_list[player.frequency]++;

            if ((player.frequency == 00 || player.frequency == 01) && (player.id != bot.id)) {

                duelers.push_back(game_->GetPlayers()[i]);

                if (player.frequency == bot.frequency) {
                    team.push_back(game_->GetPlayers()[i]);
                }
                else {
                    enemy_team.push_back(game_->GetPlayers()[i]);
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

    

 

    Path Devastation::CreatePath(behavior::ExecuteContext& ctx, const std::string& pathname, Vector2f from, Vector2f to, float radius) {
        bool build = true;
        auto& game = GetGame();

        Path path = ctx.blackboard.ValueOr(pathname, Path());

        if (!path.empty()) {
            // Check if the current destination is the same as the requested one.
            if (path.back().DistanceSq(to) < 3 * 3) {
                Vector2f pos = game.GetPosition();
                Vector2f next = path.front();
                Vector2f direction = Normalize(next - pos);
                Vector2f side = Perpendicular(direction);
                float radius = game.GetShipSettings().GetRadius();

                float distance = next.Distance(pos);

                // Rebuild the path if the bot isn't in line of sight of its next node.
                CastResult center = RayCast(game.GetMap(), pos, direction, distance);
                CastResult side1 = RayCast(game.GetMap(), pos + side * radius, direction, distance);
                CastResult side2 = RayCast(game.GetMap(), pos - side * radius, direction, distance);

                if (!center.hit && !side1.hit && !side2.hit) {
                    build = false;
                }
            }
        }

        if (build) {
            std::vector<Vector2f> mines;
            //#if 0
            for (Weapon* weapon : game.GetWeapons()) {
                const Player* weapon_player = game.GetPlayerById(weapon->GetPlayerId());
                if (weapon_player == nullptr) continue;
                if (weapon_player->frequency == game.GetPlayer().frequency) continue;
                if (weapon->GetType() & 0x8000) mines.push_back(weapon->GetPosition());
            }
            //#endif
            path = GetPathfinder().FindPath(game.GetMap(), mines, from, to, radius);
            path = GetPathfinder().SmoothPath(path, game.GetMap(), radius);
        }

        return path;
    }

    bool Devastation::CanShootGun(const Map& map, const Player& bot_player, const Player& target) {

        float bullet_speed = game_->GetSettings().ShipSettings[game_->GetPlayer().ship].BulletSpeed / 10.0f / 16.0f;
        float speed_adjust = (game_->GetPlayer().velocity * game_->GetPlayer().GetHeading());

        float bullet_travel = ((bullet_speed + speed_adjust) / 100.0f) * game_->GetSettings().BulletAliveTime;

        if (bot_player.position.Distance(target.position) > bullet_travel) return false;
        if (map.GetTileId(bot_player.position) == marvin::kSafeTileId) return false;

        return true;
    }

    bool Devastation::CanShootBomb(const Map& map, const Player& bot_player, const Player& target) {

        float bomb_speed = game_->GetSettings().ShipSettings[game_->GetPlayer().ship].BombSpeed / 10.0f / 16.0f;
        float speed_adjust = (game_->GetPlayer().velocity * game_->GetPlayer().GetHeading());

        float bomb_travel = ((bomb_speed + speed_adjust) / 100.0f) * game_->GetSettings().BombAliveTime;

        if (bot_player.position.Distance(target.position) > bomb_travel) return false;
        if (map.GetTileId(bot_player.position) == marvin::kSafeTileId) return false;

        return true;
    }

    void Devastation::LookForWallShot(Vector2f target_pos, Vector2f target_vel, float proj_speed, int alive_time, uint8_t bounces) {

        has_wall_shot = false;

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

            if (CalculateWallShot(target_pos, target_vel, left_direction, proj_speed, alive_time, bounces)) {
                has_wall_shot = true;
                return;
            }
            else if (i > 0) {
                if (CalculateWallShot(target_pos, target_vel, right_direction, proj_speed, alive_time, bounces)) {
                    has_wall_shot = true;
                    return;
                }
            }
        }
        has_wall_shot = false;
    }

    bool Devastation::CalculateWallShot(Vector2f target_pos, Vector2f target_vel, Vector2f direction, float proj_speed, int alive_time, uint8_t bounces) {

        Vector2f position = game_->GetPosition();
        Vector2f center = GetWindowCenter();
        Vector2f center_adjust = GetWindowCenter();
        float wall_debug_y = 300.0f;

        // RenderText("Bot::CalculateWallShot", center - Vector2f(wall_debug_y, 300), RGB(100, 100, 100), RenderText_Centered);

        float speed_adjust = (game_->GetPlayer().velocity * game_->GetPlayer().GetHeading());
        float travel = (proj_speed + speed_adjust) * (alive_time / 100.0f);
        const float max_travel = travel;

        // RenderText("Speed Adjust: " + std::to_string(speed_adjust), center - Vector2f(wall_debug_y, 280), RGB(100, 100, 100), RenderText_Centered);
        // RenderText("Bullet Travel: " + std::to_string(travel), center - Vector2f(wall_debug_y, 260), RGB(100, 100, 100), RenderText_Centered);

        for (int i = 0; i < bounces + 1; ++i) {

            if (i != 0) {

                //the shot solution, calculated at each bounce position
                Vector2f seek_position = CalculateShot(position, target_pos, game_->GetPlayer().velocity, target_vel, proj_speed);

                Vector2f to_target = target_pos - position;
                Vector2f target_direction = Normalize(to_target);

                CastResult target_line = RayCast(game_->GetMap(), position, target_direction, to_target.Length());

                if (!target_line.hit) {

                    float test_radius = 6.0f;
                    Vector2f box_min = seek_position - Vector2f(test_radius, test_radius);
                    Vector2f box_extent(test_radius * 1.2f, test_radius * 1.2f);
                    float dist;
                    Vector2f norm;

                    if (RayBoxIntersect(position, direction, box_min, box_extent, &dist, &norm)) {
                        if (dist < travel) {
                 
                            return true;
                        }
                    }
                }
            }

            CastResult wall_line = RayCast(game_->GetMap(), position, direction, travel);

            if (i == 0) {
                wall_shot = wall_line.position;
                std::string first_bounce_normal = "First Bounce Normal: " + std::to_string(wall_line.normal.x) + ", " + std::to_string(wall_line.normal.y);
                // RenderText(first_bounce_normal, center - Vector2f(wall_debug_y, 200), RGB(100, 100, 100), RenderText_Centered);

                std::string first_bounce_position = "First Bounce Position: " + std::to_string(wall_line.position.x) + ", " + std::to_string(wall_line.position.y);
                // RenderText(first_bounce_position, center - Vector2f(wall_debug_y, 180), RGB(100, 100, 100), RenderText_Centered);

                 //RenderLine(center, center + (seek_direction * to_seek.Length() * 16), RGB(100, 100, 100)); 
            }

            if (wall_line.hit) {
                //  RenderLine(center_adjust, center_adjust + (direction * wall_line.distance * 16), RGB(100, 100, 100));

                center_adjust += (direction * wall_line.distance * 16);

                direction = direction - (wall_line.normal * (2.0f * direction.Dot(wall_line.normal)));

                position = wall_line.position;

                travel -= wall_line.distance;
            }
            else {
                // RenderLine(center_adjust, center_adjust + (direction * travel * 16), RGB(100, 100, 100));
                return false;
            }
        }
        return false;
    }


namespace deva {

        behavior::ExecuteResult FreqWarpAttachNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.deva->GetGame();
         
            uint64_t time = ctx.com->GetTime();

            uint64_t freq_cooldown = ctx.blackboard.ValueOr<uint64_t>("FreqCooldown", 0);
            uint64_t warp_cooldown = ctx.blackboard.ValueOr<uint64_t>("WarpCooldown", 0);
            uint64_t toggle_cooldown = ctx.blackboard.ValueOr<uint64_t>("ToggleCooldown", 0);

            //bots on the same team tend to switch out at the same time when team is uneven, they need unique wait timers
            uint64_t freq_timer_offset = ((uint64_t)game.GetPlayer().id * 10) + 200;

            //makes the timers load up for the first pass, needed to keep multiple bots from spamming frequency changes
            bool set_freq_timer = ctx.blackboard.ValueOr<bool>("SetTimer", true);
            bool set_warp_timer = ctx.blackboard.ValueOr<bool>("SetWarpTimer", true);
            //reset the timer if it gets toggled but not toggled back
            if (time > freq_cooldown + 150) set_freq_timer = true;
            if (time > warp_cooldown + 150) set_warp_timer = true;

            //warp out of base after a base duel win
            if (!ctx.deva->InCenter()) {
                if (!EnemyInBase(ctx)) {
                    //set the timer
                    if (set_warp_timer) {
                        ctx.blackboard.Set("WarpCooldown", time + 30000);
                        ctx.blackboard.Set("SetWarpTimer", false);
                    }
                    //warp out when timer expires
                    else if (time > warp_cooldown ) {
                        if (CheckStatus(ctx)) {
                            game.Warp();
                            ctx.blackboard.Set("SetWarpTimer", true);
                        }
                        return behavior::ExecuteResult::Success;
                    }
                }
            }

            //if player is on a non duel team size greater than 2, breaks the zone balancer
            if (game.GetPlayer().frequency != 00 && game.GetPlayer().frequency != 01) {
                if (ctx.deva->GetFreqList()[game.GetPlayer().frequency] > 1) {
                    if (set_freq_timer) {
                        ctx.blackboard.Set("FreqCooldown", time + freq_timer_offset);
                        ctx.blackboard.Set("SetTimer", false);
                    }
                    //this team is too large, find an open freq
                    else if (time > freq_cooldown) {
                        if (CheckStatus(ctx)) {
                            game.SetFreq(ctx.com->FindOpenFreq(ctx.deva->GetFreqList(), 2));
                            ctx.blackboard.Set("SetTimer", true);
                        }
                        return behavior::ExecuteResult::Success;
                    }
                }
            }

            //duel team is uneven
            if (ctx.deva->GetFreqList()[0] != ctx.deva->GetFreqList()[1]) {
                    //bot is not on a duel team
                if (game.GetPlayer().frequency != 00 && game.GetPlayer().frequency != 01) {
                    //get on a baseduel freq as fast as possible
                    if (CheckStatus(ctx)) {
                        //get on freq 0
                        if (ctx.deva->GetFreqList()[0] < ctx.deva->GetFreqList()[1]) {
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
                else if (ctx.deva->InCenter() || ctx.deva->GetDevaTeam().size() == 0 || TeamInBase(ctx)) {

                    if ((game.GetPlayer().frequency == 00 && ctx.deva->GetFreqList()[0] > ctx.deva->GetFreqList()[1]) ||
                        (game.GetPlayer().frequency == 01 && ctx.deva->GetFreqList()[0] < ctx.deva->GetFreqList()[1])) {
                        
                        //look for players not on duel team, make bot wait longer to see if the other player will get in
                        for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
                            const Player& player = game.GetPlayers()[i];

                            if (player.frequency > 01 && player.frequency < 100) {
                                float energy_pct = (player.energy / (float)game.GetSettings().ShipSettings[player.ship].MaximumEnergy) * 100.0f;
                                if (energy_pct != 100.0f) {
                                    return behavior::ExecuteResult::Success;
                                }
                            }
                        }

                        /* set the timer first, the bot waits before it can switch out of a team, but only if the conditions are
                           still valid when the timer expires.  this keeps multiple bots on the same team from switching out at the ame time */
                        if (set_freq_timer) {
                            ctx.blackboard.Set("FreqCooldown", time + freq_timer_offset);
                            ctx.blackboard.Set("SetTimer", false);
                        }
                            //this team is too large, find an open freq
                        else if (time > freq_cooldown && CheckStatus(ctx)) {

                            game.SetFreq(ctx.com->FindOpenFreq(ctx.deva->GetFreqList(), 2));
                            ctx.blackboard.Set("SetTimer", true);
                        }
                        return behavior::ExecuteResult::Success;
                    }
                }
            }

            //if dueling then run checks for attaching
            if (game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01) {

                //if this check is valid the bot will halt here untill it attaches
                if (ctx.deva->InCenter() && TeamInBase(ctx)) {

                    SetAttachTarget(ctx);

                    //checks if energy is full
                    if (CheckStatus(ctx)) {
                        game.F7();
                    }
                    return behavior::ExecuteResult::Success;
                }
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
                game.Cloak(ctx.deva->GetKeys());
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
            
            auto& game = ctx.deva->GetGame();
            float energy_pct = (game.GetEnergy() / (float)game.GetShipSettings().MaximumEnergy) * 100.0f;
            bool result = false;
            if ((game.GetPlayer().status & 2) != 0) game.Cloak(ctx.deva->GetKeys());
            if (energy_pct == 100.0f) result = true;
            return result;
        }

        void FreqWarpAttachNode::SetAttachTarget(behavior::ExecuteContext& ctx) {
            auto& game = ctx.deva->GetGame();

            std::vector<Vector2f> path = ctx.deva->GetBasePath();

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
            for (std::size_t i = 0; i < ctx.deva->GetDevaDuelers().size(); i++) {
                const Player& player = ctx.deva->GetDevaDuelers()[i];
                bool player_in_center = ctx.deva->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));
                if (!player_in_center && IsValidPosition(player.position) && player.ship < 8) {
                    
                    float distance_to_team = ctx.com->PathLength(path, ctx.com->FindPathNode(path, player.position), ctx.com->FindPathNode(path, ctx.deva->GetTeamSafe()));
                    float distance_to_enemy = ctx.com->PathLength(path, ctx.com->FindPathNode(path, player.position), ctx.com->FindPathNode(path, ctx.deva->GetEnemySafe()));

                    if (player.frequency == game.GetPlayer().frequency) {
                        //float distance_to_team = ctx.deva->PathLength(player.position, ctx.deva->GetTeamSafe());
                        //float distance_to_enemy = ctx.deva->PathLength(player.position, ctx.deva->GetEnemySafe());

                        

                        if (distance_to_team < closest_team_distance_to_team) {
                            closest_team_distance_to_team = distance_to_team;
                            closest_team_to_team = &ctx.deva->GetDevaDuelers()[i];
                        }
                        if (distance_to_enemy < closest_team_distance_to_enemy) {
                            closest_team_distance_to_enemy = distance_to_enemy;
                            closest_team_to_enemy = &ctx.deva->GetDevaDuelers()[i];
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
                            closest_enemy_to_enemy = &ctx.deva->GetDevaDuelers()[i];
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
            for (std::size_t i = 0; i < ctx.deva->GetDevaTeam().size(); i++) {
                const Player& player = ctx.deva->GetDevaTeam()[i];

                bool player_in_center = ctx.deva->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));

                //as long as the player is not in center and has a valid position, it will hold its position for a moment after death
                if (!player_in_center && IsValidPosition(player.position) && player.ship < 8) {

                   // float distance_to_enemy_position = ctx.deva->PathLength(player.position, closest_enemy_to_team);

                    float distance_to_enemy_position = ctx.com->PathLength(path, ctx.com->FindPathNode(path, player.position), ctx.com->FindPathNode(path, closest_enemy_to_team));

                    //get the closest player
                    if (distance_to_enemy_position < closest_distance) {
                        closest_distance = distance_to_enemy_position;
                        game.SetSelectedPlayer(ctx.deva->GetDevaTeam()[i]);
                    }   
                }
            }  
        }

        bool FreqWarpAttachNode::TeamInBase(behavior::ExecuteContext& ctx) {

            bool in_base = false;

            for (std::size_t i = 0; i < ctx.deva->GetDevaTeam().size(); i++) {
                const Player& player = ctx.deva->GetDevaTeam()[i];
                bool in_center = ctx.deva->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));
                if (!in_center && IsValidPosition(player.position)) {
                    in_base = true;
                }
            }
            return in_base;
        }

        bool FreqWarpAttachNode::EnemyInBase(behavior::ExecuteContext& ctx) {

            bool in_base = false;

            for (std::size_t i = 0; i < ctx.deva->GetDevaEnemyTeam().size(); i++) {
                const Player& player = ctx.deva->GetDevaEnemyTeam()[i];
                bool in_center = ctx.deva->GetRegions().IsConnected((MapCoord)player.position, MapCoord(512, 512));
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
            Vector2f enemy = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr)->position;

            Path path;
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
                    float pathlength = ctx.com->PathLength(ctx.deva->GetBasePath(), ctx.com->FindPathNode(path, bot), ctx.com->FindPathNode(path, enemy));
                    if (pathlength < 45.0f || ctx.deva->LastBotStanding()) {
                    //if (ctx.deva->PathLength(bot, enemy) < 45.0f || ctx.deva->LastBotStanding()) {
                        path = ctx.deva->CreatePath(ctx, "path", bot, ctx.deva->GetTeamSafe(), radius);
                    }
                    else path = ctx.deva->CreatePath(ctx, "path", bot, enemy, radius);
                }
                else if (ctx.deva->LastBotStanding()) {
                    path = ctx.deva->CreatePath(ctx, "path", bot, ctx.deva->GetTeamSafe(), radius);
                }
                else path = ctx.deva->CreatePath(ctx, "path", bot, enemy, radius);
            }
            else path = ctx.deva->CreatePath(ctx, "path", bot, enemy, radius);

            ctx.blackboard.Set("path", path);
            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult PatrolNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.deva->GetGame();
            Vector2f from = game.GetPosition();
            Path path = ctx.blackboard.ValueOr("path", Path());
            float radius = game.GetShipSettings().GetRadius();
 
            std::vector<Vector2f> nodes;
            int index = ctx.blackboard.ValueOr<int>("patrol_index", 0);
            int taunt_index = ctx.blackboard.ValueOr<int>("taunt_index", 0);
            uint64_t time = ctx.com->GetTime();
            uint64_t hoorah_expire = ctx.blackboard.ValueOr<uint64_t>("HoorahExpire", 0);
            bool in_safe = game.GetMap().GetTileId(game.GetPosition()) == kSafeTileId;

            if (ctx.deva->InCenter()) {
                nodes = { Vector2f(568, 568), Vector2f(454, 568), Vector2f(454, 454), Vector2f(568, 454), Vector2f(568, 568),
                Vector2f(454, 454), Vector2f(568, 454), Vector2f(454, 568), Vector2f(454, 454), Vector2f(568, 568), Vector2f(454, 568), Vector2f(568, 454) };
            }
            else {
                if (ctx.deva->LastBotStanding()) {
                    path = ctx.deva->CreatePath(ctx, "path", from, ctx.deva->GetTeamSafe(), radius);
                    ctx.blackboard.Set("path", path);
                    return behavior::ExecuteResult::Success;
                }
                else {
                    path = ctx.deva->CreatePath(ctx, "path", from, ctx.deva->GetEnemySafe(), radius);
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

            path = ctx.deva->CreatePath(ctx, "path", from, to, radius);
            ctx.blackboard.Set("path", path);

            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult FollowPathNode::Execute(behavior::ExecuteContext& ctx) {
            auto path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());

            size_t path_size = path.size();
            auto& game = ctx.deva->GetGame();

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
            const Vector2f* target = selector_(ctx);

            if (!target) { return behavior::ExecuteResult::Failure; }
            auto& game = ctx.deva->GetGame();

            if (game.GetPlayer().ship == 2) { return behavior::ExecuteResult::Failure; }

            float bullet_speed = game.GetSettings().ShipSettings[game.GetPlayer().ship].BulletSpeed / 10.0f / 16.0f;
            float bomb_speed = game.GetSettings().ShipSettings[game.GetPlayer().ship].BombSpeed / 10.0f / 16.0f;
            int bullet_time = game.GetSettings().BulletAliveTime;
            int bomb_time = game.GetSettings().BombAliveTime;
            uint8_t bounces = game.GetSettings().ShipSettings[game.GetPlayer().ship].BombBounceCount;

            auto to_target = *target - game.GetPosition();
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
                const Player* target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
                if (target_player) {
                    float target_radius = game.GetSettings().ShipSettings[target_player->ship].GetRadius();
                      
                        if (ctx.com->ShootWall(target_player->position, target_player->velocity, target_radius, game.GetPlayer().GetHeading(), bullet_speed, bullet_time, 10)) {
                           ctx.deva->GetKeys().Press(VK_CONTROL, ctx.com->GetTime(), 30);
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
                //RenderText("HIT", GetWindowCenter() - Vector2f(0, 40), RGB(100, 100, 100), RenderText_Centered);
                if (ctx.deva->CanShootGun(game.GetMap(), bot_player, target)) {
                   // RenderText("CANSHOOT", GetWindowCenter() - Vector2f(0, 60), RGB(100, 100, 100), RenderText_Centered);
                    return behavior::ExecuteResult::Success;
                }
            }

            

            return behavior::ExecuteResult::Failure;
        }




        behavior::ExecuteResult ShootEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            int ship = ctx.deva->GetGame().GetPlayer().ship;
            if (ship == 3 || ship == 6 || ship == 2) ctx.deva->GetKeys().Press(VK_TAB, ctx.com->GetTime(), 30);
            else ctx.deva->GetKeys().Press(VK_CONTROL, ctx.com->GetTime(), 30);

            return behavior::ExecuteResult::Success;
        }




        behavior::ExecuteResult MoveToEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.deva->GetGame();
            
            float energy_pct = game.GetEnergy() / (float)game.GetShipSettings().MaximumEnergy;

            const Player* target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
            if (!target_player) {
                return behavior::ExecuteResult::Failure;
            }

            float player_energy_pct = target_player->energy / (float)game.GetShipSettings().MaximumEnergy;
            
            float energy_check = ctx.blackboard.ValueOr<float>("EnergyCheck", 0);
            bool emped = ctx.blackboard.ValueOr<bool>("Emped", false);
            
            uint64_t time = ctx.com->GetTime();
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