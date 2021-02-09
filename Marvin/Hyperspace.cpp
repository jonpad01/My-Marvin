#include "Hyperspace.h"

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
#include "Shooter.h"

namespace marvin {


    Hyperspace::Hyperspace(std::unique_ptr<marvin::GameProxy> game) : game_(std::move(game)), steering_(*game_), common_(*game_), keys_(common_.GetTime()) {

        auto processor = std::make_unique<path::NodeProcessor>(*game_);

        last_ship_change_ = 0;
        ship_ = game_->GetPlayer().ship;


        if (ship_ > 7) {
            ship_ = 1;
        }

        pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor));
        regions_ = RegionRegistry::Create(game_->GetMap());
        pathfinder_->CreateMapWeights(game_->GetMap());

        CreateHSBasePaths();
        ctx_.blackboard.Set("patrol_nodes", std::vector<Vector2f>({ Vector2f(585, 540), Vector2f(400, 570) }));
        ctx_.blackboard.Set("patrol_index", 0);

        auto freq_warp_attach = std::make_unique<hs::FreqWarpAttachNode>();
        auto find_enemy = std::make_unique<hs::FindEnemyNode>();
        auto looking_at_enemy = std::make_unique<hs::LookingAtEnemyNode>();
        auto target_in_los = std::make_unique<hs::InLineOfSightNode>(
            [](marvin::behavior::ExecuteContext& ctx) {
                const Vector2f* result = nullptr;

                const Player* target_player =
                    ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

                if (target_player) {
                    result = &target_player->position;
                }
                return result;
            });

        auto shoot_enemy = std::make_unique<hs::ShootEnemyNode>();
        auto path_to_enemy = std::make_unique<hs::PathToEnemyNode>();
        auto move_to_enemy = std::make_unique<hs::MoveToEnemyNode>();
        auto follow_path = std::make_unique<hs::FollowPathNode>();
        auto patrol = std::make_unique<hs::PatrolNode>();
        auto bundle_shots = std::make_unique<hs::BundleShots>();



        //if bundle shots fails, then move to enemy
        //move to enemy is how the bot behaves when fighting another player
        auto move_method_selector = std::make_unique<behavior::SelectorNode>(bundle_shots.get(), move_to_enemy.get());
        //if looking at enemy fails, the bot wont shoot
        auto shoot_sequence = std::make_unique<behavior::SequenceNode>(looking_at_enemy.get(), shoot_enemy.get());
        //a parallel node means it runs both regardless of failure
        auto parallel_shoot_enemy = std::make_unique<behavior::ParallelNode>(shoot_sequence.get(), move_method_selector.get());
        //if target in los fails, it falls back to path to enemy
        auto los_shoot_conditional = std::make_unique<behavior::SequenceNode>(target_in_los.get(), parallel_shoot_enemy.get());
        //if target in los fails it moves here and creates a path to the enemy, if there is no path it fails
        //not sure if it falls back to patrol or goes limp dick
        auto enemy_path_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy.get(), follow_path.get());
        //if find enemy fails then it moves into this, the only time this fails is if its patrol coordinates are unreachable
        //the bot goes limp dick and does nothing
        auto patrol_path_sequence = std::make_unique<behavior::SequenceNode>(patrol.get(), follow_path.get());
        //if find enemey succeeds, it moves directly into this selector, and tries los shoot first
        auto path_or_shoot_selector = std::make_unique<behavior::SelectorNode>(los_shoot_conditional.get(), enemy_path_sequence.get());
        //handle enemy is the first step, it moves directly into find_enemy, if find enemy fails it falls back to patrol path sequence
        auto handle_enemy = std::make_unique<behavior::SequenceNode>(find_enemy.get(), path_or_shoot_selector.get());
        //this is the beginning of the behavoir tree, the selector nodes always start with the first argument
        //if the first argument returns a failure then the selector runs the next node
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
        behavior_nodes_.push_back(std::move(bundle_shots));

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
        ctx_.hs = this;
        ctx_.com = &common_;
    }

    void Hyperspace::Update(float dt) {
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

        CheckCenterRegion();

        steering_.Reset();

        ctx_.dt = dt;

        behavior_->Update(ctx_);

#if DEBUG_RENDER
        // RenderPath(GetGame(), behavior_ctx_.blackboard);
#endif
        Steer();
    }


    void Hyperspace::Steer() {
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
           // keys_.Set(VK_RIGHT, clockwise, GetTime());
           // keys_.Set(VK_LEFT, !clockwise, GetTime());
        }

    }

    void Hyperspace::Move(const Vector2f& target, float target_distance) {
        const Player& bot_player = game_->GetPlayer();
        float distance = bot_player.position.Distance(target);
        Vector2f heading = game_->GetPlayer().GetHeading();

        Vector2f target_position = ctx_.blackboard.ValueOr("target_position", Vector2f());
        float dot = heading.Dot(Normalize(target_position - game_->GetPosition()));
        bool anchor = (bot_player.frequency == 90 || bot_player.frequency == 91) && bot_player.ship == 6;
        Flaggers flaggers = FindFlaggers();

        //seek makes the bot hunt the target hard
        //arrive makes the bot slow down as it gets closer
        //this works with steering.Face in MoveToEnemyNode
        //there is a small gap where the bot wont do anything at all
        if (distance > target_distance) {
            if (flaggers.enemy_anchor) {
                bool enemy_connected = regions_->IsConnected((MapCoord)bot_player.position, (MapCoord)flaggers.enemy_anchor->position);
                if (InHSBase(bot_player.position) && enemy_connected && (anchor || game_->GetPlayer().ship == 2)) {
                    steering_.AnchorSpeed(target);
                }
                else steering_.Seek(target);
            }
            else steering_.Seek(target);
        }

        else if (distance <= target_distance) {

            Vector2f to_target = target - bot_player.position;

            steering_.Seek(target - Normalize(to_target) * target_distance);
        }
    }

    void Hyperspace::CheckHSBaseRegions(Vector2f position) {

        hs_in_1 = regions_->IsConnected((MapCoord)position, MapCoord(854, 358));
        hs_in_2 = regions_->IsConnected((MapCoord)position, MapCoord(823, 488));
        hs_in_3 = regions_->IsConnected((MapCoord)position, MapCoord(696, 817));
        hs_in_4 = regions_->IsConnected((MapCoord)position, MapCoord(587, 776));
        hs_in_5 = regions_->IsConnected((MapCoord)position, MapCoord(125, 877));
        hs_in_6 = regions_->IsConnected((MapCoord)position, MapCoord(254, 493));
        hs_in_7 = regions_->IsConnected((MapCoord)position, MapCoord(154, 358));
        hs_in_8 = regions_->IsConnected((MapCoord)position, MapCoord(620, 250));
    }

    bool Hyperspace::InHSBase(Vector2f position) {
        CheckHSBaseRegions(position);
        return hs_in_1 || hs_in_2 || hs_in_3 || hs_in_4 || hs_in_5 || hs_in_6 || hs_in_7;
    }

    std::vector<bool> Hyperspace::InHSBaseNumber(Vector2f position) {
        CheckHSBaseRegions(position);
        std::vector<bool> in = { hs_in_1, hs_in_2, hs_in_3, hs_in_4, hs_in_5, hs_in_6, hs_in_7 };
        return in;
    }

    int Hyperspace::BaseSelector() {
        uint64_t timestamp = common_.GetTime();
        bool base_cooldown = timestamp - last_base_change_ > 200000;
        int base = ctx_.blackboard.ValueOr<int>("base", 5);
        if (base_cooldown) {
            base++;
            ctx_.blackboard.Set<int>("base", base);
            last_base_change_ = timestamp;
        }

        if (base > 6) {
            base = 0;
            ctx_.blackboard.Set<int>("base", base);
        }
        return base;
    }

    Flaggers Hyperspace::FindFlaggers() {

        const Player& bot = game_->GetPlayer();
        Vector2f position = game_->GetPlayer().position;
        std::vector < const Player* > enemy_anchors, team_anchors;
        Flaggers result = { nullptr, nullptr, 0, 0, 0 };

        //grab all flaggers in lancs and sort by team then store in a vector
        for (std::size_t i = 0; i < game_->GetPlayers().size(); ++i) {
            const Player& player = game_->GetPlayers()[i];
            bool flagging = (player.frequency == 90 || player.frequency == 91);
            bool not_bot = player.id != bot.id;
            bool enemy_team = player.frequency != bot.frequency;
            bool same_team = player.frequency == bot.frequency;

            if (flagging && player.name[0] != '<') {
                result.flaggers++;
                if (player.ship == 6 && same_team) result.team_lancs++;
                if (player.ship == 2 && same_team) result.team_spiders++;
                if (player.ship == 6 && not_bot) {
                    //find and store in a vector
                    if (enemy_team) enemy_anchors.push_back(&game_->GetPlayers()[i]);
                    if (same_team) team_anchors.push_back(&game_->GetPlayers()[i]);
                }
            }
        }

        //loops through lancs and rejects any not in base, unless it reaches the last one
        if (enemy_anchors.size() != 0) {
            for (std::size_t i = 0; i < enemy_anchors.size(); i++) {
                const Player* player = enemy_anchors[i];

                if (!InHSBase(player->position) && i != enemy_anchors.size() - 1) {
                    continue;
                }
                else {
                    result.enemy_anchor = enemy_anchors[i];
                }
            }
        }
        if (team_anchors.size() != 0) {
            for (std::size_t i = 0; i < team_anchors.size(); i++) {
                const Player* player = team_anchors[i];

                if (!InHSBase(player->position) && i != team_anchors.size() - 1) {
                    continue;
                }
                else {
                    result.team_anchor = team_anchors[i];
                }
            }
        }
        return result;
    }


    bool Hyperspace::CheckStatus() {
        float energy_pct = (game_->GetEnergy() / (float)game_->GetShipSettings().InitialEnergy) * 100.0f;
        bool result = false;
        if ((game_->GetPlayer().status & 2) != 0) game_->Cloak(keys_);
        if ((game_->GetPlayer().status & 1) != 0) game_->Stealth();
        if (energy_pct == 100.0f) result = true;
        return result;
    }

    void Hyperspace::CreateHSBasePaths() {

        std::vector<Vector2f> base_entrance = { Vector2f(827, 339), Vector2f(811, 530), Vector2f(729, 893),
                                                Vector2f(444, 757), Vector2f(127, 848), Vector2f(268, 552), Vector2f(181, 330) };
        std::vector<Vector2f> flag_room = { Vector2f(826, 229), Vector2f(834, 540), Vector2f(745, 828),
                                            Vector2f(489, 832), Vector2f(292, 812), Vector2f(159, 571), Vector2f(205, 204) };

        std::vector<Vector2f> mines;

        for (std::size_t i = 0; i < base_entrance.size(); i++) {

            std::vector<Vector2f> base_path = GetPathfinder().FindPath(game_->GetMap(), mines, base_entrance[i], flag_room[i], game_->GetSettings().ShipSettings[6].GetRadius());
            base_path = GetPathfinder().SmoothPath(base_path, game_->GetMap(), game_->GetSettings().ShipSettings[6].GetRadius());

            hs_base_paths_.push_back(base_path);
        }
    }

    void Hyperspace::CheckCenterRegion() {
        in_center = regions_->IsConnected((MapCoord)game_->GetPosition(), MapCoord(512, 512));
    }




	namespace hs {

        /*monkey> no i dont have a list of the types
monkey> i think it's some kind of combined thing with weapon flags in it
monkey> im pretty sure type & 0x01 is whether or not it's bouncing
monkey> and the 0x8000 is the alternative bit for things like mines and multifire
monkey> i was using type & 0x8F00 to see if it was a mine and it worked decently well
monkey> 0x0F00 worked ok for seeing if its a bomb
monkey> and 0x00F0 for bullet, but i don't think it's exact*/

        behavior::ExecuteResult FreqWarpAttachNode::Execute(behavior::ExecuteContext& ctx) {
            auto& game = ctx.hs->GetGame();
            
            Flaggers flaggers = ctx.hs->FindFlaggers();

            bool team_anchor_in_safe = false;
            uint64_t target_id = 0;
            
            bool in_center = ctx.hs->GetRegions().IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));
            
            if (flaggers.team_anchor != nullptr) {
                team_anchor_in_safe = game.GetMap().GetTileId(flaggers.team_anchor->position) == kSafeTileId;
            }
            bool in_safe = game.GetMap().GetTileId(game.GetPosition()) == kSafeTileId;
            float energy_pct = (game.GetEnergy() / (float)game.GetShipSettings().InitialEnergy) * 100.0f;
            bool respawned = in_safe && in_center && energy_pct == 100.0f;
            
            uint64_t time = ctx.com->GetTime();
            uint64_t spam_check = ctx.blackboard.ValueOr<uint64_t>("SpamCheck", 0);
            uint64_t flash_check = ctx.blackboard.ValueOr<uint64_t>("FlashCoolDown", 0);
            bool no_spam = time > spam_check;
            bool flash_cooldown = time > flash_check;
            
            bool flagging = game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91;
            bool in_lanc = game.GetPlayer().ship == 6; bool in_spid = game.GetPlayer().ship == 2;
            bool anchor = flagging && in_lanc;
            
            //warp out if enemy anchor is in another base
            if (flaggers.enemy_anchor != nullptr) {

                bool not_connected = !ctx.hs->GetRegions().IsConnected((MapCoord)game.GetPosition(), (MapCoord)flaggers.enemy_anchor->position);

                if (ctx.hs->InHSBase(game.GetPosition()) && ctx.hs->InHSBase(flaggers.enemy_anchor->position) && not_connected) {
                    if (ctx.hs->CheckStatus()) game.Warp();
                    return behavior::ExecuteResult::Success;
                }
            }

                //checkstatus turns off stealth and cloak before warping
                if (!in_center && !flagging) {
                    if (ctx.hs->CheckStatus()) game.Warp();
                    return behavior::ExecuteResult::Success;
                }
                //join a flag team, spams too much without the timer
                if (flaggers.flaggers < 14 && !flagging && respawned && no_spam) {
                    if (ctx.hs->CheckStatus()) game.Flag();
                    ctx.blackboard.Set("SpamCheck", time + 200);
                }
                //switch to lanc if team doesnt have one
                if (flagging && flaggers.team_lancs == 0 && !in_lanc && respawned && no_spam) {
                    //try to remember what ship it was in
                    ctx.blackboard.Set<int>("last_ship", (int)game.GetPlayer().ship);
                    if (ctx.hs->CheckStatus()) game.SetShip(6);
                    ctx.blackboard.Set("SpamCheck", time + 200);
                }
                //if there is more than one lanc on the team, switch back to original ship
                if (flagging && flaggers.team_lancs > 1 && in_lanc && no_spam) {
                    if (flaggers.team_anchor != nullptr) {
                        
                        if (ctx.hs->InHSBase(flaggers.team_anchor->position) || !ctx.hs->InHSBase(game.GetPosition())) {
                            int ship = ctx.blackboard.ValueOr<int>("last_ship", 0);
                            if (ctx.hs->CheckStatus()) game.SetShip(ship);
                            ctx.blackboard.Set("SpamCheck", time + 200);
                            return behavior::ExecuteResult::Success;
                        }
                    }
                }

                //switch to spider if team doesnt have one
                if (flagging && flaggers.team_spiders == 0 && !in_spid && respawned && no_spam) {
                    //try to remember what ship it was in
                    ctx.blackboard.Set<int>("last_ship", (int)game.GetPlayer().ship);
                    if (ctx.hs->CheckStatus()) game.SetShip(2);
                    ctx.blackboard.Set("SpamCheck", time + 200);
                }
                //if there is more than two spiders on the team, switch back to original ship
                if (flagging && flaggers.team_spiders > 2 && in_spid && respawned && no_spam) {
                        int ship = ctx.blackboard.ValueOr<int>("last_ship", 0);
                        if (ctx.hs->CheckStatus()) game.SetShip(ship);
                        ctx.blackboard.Set("SpamCheck", time + 200);
                        return behavior::ExecuteResult::Success;      
                }

                //leave a flag team by spectating, will reenter ship on the next update
                if (flaggers.flaggers > 16 && flagging && !in_lanc && respawned && no_spam) {
                    int freq = ctx.com->FindOpenFreq(ctx.hs->GetFreqList(), 0);
                    if (ctx.hs->CheckStatus()) game.SetFreq(freq);// game.Spec();
                    ctx.blackboard.Set("SpamCheck", time + 200);
                }

                //if flagging try to attach
                if (flagging && !team_anchor_in_safe && !in_lanc && no_spam) {
                    uint64_t last_target_id = ctx.blackboard.ValueOr<uint64_t>("LastTarget", 0);
                    if (flaggers.team_anchor != nullptr) {
                        target_id = flaggers.team_anchor->id;
                    }
                    bool same_target = target_id == last_target_id;
                    //all ships will attach after respawning, all but lancs will attach if not in the same base
                    if (flaggers.team_anchor != nullptr) {
                        
                        bool not_connected = !ctx.hs->GetRegions().IsConnected((MapCoord)game.GetPosition(), (MapCoord)flaggers.team_anchor->position);

                        if (respawned || (not_connected && (same_target || flaggers.team_lancs < 2))) {
                            if (ctx.hs->CheckStatus()) game.Attach(flaggers.team_anchor->name);
                            ctx.blackboard.Set("SpamCheck", time + 200);
                            ctx.blackboard.Set("LastTarget", target_id);
                            return behavior::ExecuteResult::Success;
                        }
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
                game.Cloak(ctx.hs->GetKeys());
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
            auto& game = ctx.hs->GetGame();
            Flaggers flaggers = ctx.hs->FindFlaggers();
      
            const Player* target = nullptr;
            const Player& bot = ctx.hs->GetGame().GetPlayer();
            bool flagging = bot.frequency == 90 || bot.frequency == 91;
            bool anchor = (bot.frequency == 90 || bot.frequency == 91) && bot.ship == 6;
            
            Vector2f position = game.GetPosition();
         
            Vector2f resolution(1920, 1080);
            view_min_ = bot.position - resolution / 2.0f / 16.0f;
            view_max_ = bot.position + resolution / 2.0f / 16.0f;
            //if anchor and not in base switch to patrol mode
            if (anchor && !ctx.hs->InHSBase(game.GetPosition())) {
                return behavior::ExecuteResult::Failure;
            }
            //this loop checks every player and finds the closest one based on a cost formula
            //if this does not succeed then there is no target, it will go into patrol mode
            for (std::size_t i = 0; i < game.GetPlayers().size(); ++i) {
                const Player& player = game.GetPlayers()[i];

                if (!IsValidTarget(ctx, player)) continue;
                auto to_target = player.position - position;
                CastResult in_sight = RayCast(game.GetMap(), game.GetPosition(), Normalize(to_target), to_target.Length());

                //make players in line of sight high priority
                if (flagging && flaggers.team_anchor != nullptr) {
                    if (player.position.Distance(flaggers.team_anchor->position) < 10.0f) {
                        cost = CalculateCost(game, bot, player) / 100.0f;
                    }
                    else cost = CalculateCost(game, bot, player);
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
            const auto& game = ctx.hs->GetGame();
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

            if (!ctx.hs->GetRegions().IsConnected(bot_coord, target_coord)) {
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
            auto& game = ctx.hs->GetGame();

            bool anchor = (game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91) && game.GetPlayer().ship == 6;
            bool flagging = game.GetPlayer().frequency == 90 || game.GetPlayer().frequency == 91;

            Vector2f bot = game.GetPosition();
            enemy_ = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr)->position;

            std::vector<Vector2f> path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());

            //anchors dont engage enemies until they reach a base via the patrol node
            if (anchor) {

                flaggers_ = ctx.hs->FindFlaggers();
                std::vector<bool> in_number = ctx.hs->InHSBaseNumber(game.GetPosition());
                
                for (std::size_t i = 0; i < in_number.size(); i++) {
                    if (in_number[i]) {
                        //a set path built once and stored, path begin is entrance and end is flag room
                        base_path_ = ctx.hs->GetBasePaths(i);

                        //find pre calculated path node for the current base
                        FindClosestNodes(ctx);

                        if (DistanceToEnemy(ctx) < 50.0f) {
                            if (IsDefendingAnchor(ctx)) {
                                if (bot_node + 4 > base_path_.size()) {
                                    //the anchor will use a path calculated through base to move away from enemy
                                    path = ctx.hs->GetPathfinder().CreatePath(path, bot, base_path_.back(), game.GetShipSettings().GetRadius());
                                }
                                else path = ctx.hs->GetPathfinder().CreatePath(path, bot, base_path_[bot_node + 3], game.GetShipSettings().GetRadius());
                            }
                            else {
                                if (bot_node < 3) {
                                    //reading a lower index in the base_path means it will move towards the base entrance
                                    path = ctx.hs->GetPathfinder().CreatePath(path, bot, base_path_[0], game.GetShipSettings().GetRadius());
                                }
                                else path = ctx.hs->GetPathfinder().CreatePath(path, bot, base_path_[bot_node - 3], game.GetShipSettings().GetRadius());
                                
                            }
                        }
                        else {
                            path = ctx.hs->GetPathfinder().CreatePath(path, bot, enemy_, game.GetShipSettings().GetRadius());
                        }
                    }
                }
            }
            else if (game.GetPlayer().ship == 2 && flagging) {
                flaggers_ = ctx.hs->FindFlaggers();
                if (flaggers_.team_anchor != nullptr) {
                    bool connected = ctx.hs->GetRegions().IsConnected((MapCoord)game.GetPosition(), (MapCoord)flaggers_.team_anchor->position);
                    if (flaggers_.team_anchor->position.Distance(game.GetPosition()) > 15.0f && connected) {
                        path = ctx.hs->GetPathfinder().CreatePath(path, bot, flaggers_.team_anchor->position, game.GetShipSettings().GetRadius());
                    }
                }
                else path = ctx.hs->GetPathfinder().CreatePath(path, bot, enemy_, game.GetShipSettings().GetRadius());
            }
            else {
                path = ctx.hs->GetPathfinder().CreatePath(path, bot, enemy_, game.GetShipSettings().GetRadius());
            }
            
            ctx.blackboard.Set("path", path);
            return behavior::ExecuteResult::Success;
        }

        void PathToEnemyNode::FindClosestNodes(behavior::ExecuteContext& ctx) {
            auto& game = ctx.hs->GetGame();
            if (flaggers_.enemy_anchor == nullptr) return;

            float bot_closest_distance = std::numeric_limits<float>::max();
            float enemy_anchor_closest_distance = std::numeric_limits<float>::max();
            float enemy_closest_distance = std::numeric_limits<float>::max();

            //find closest path node
            for (std::size_t i = 0; i < base_path_.size(); i++) {
                float bot_distance_to_node = game.GetPosition().Distance(base_path_[i]);
                float enemy_distance_to_node = enemy_.Distance(base_path_[i]);
                float enemy_anchor_distance_to_node = flaggers_.enemy_anchor->position.Distance(base_path_[i]);
                
                if (bot_closest_distance > bot_distance_to_node) {
                    bot_node = i;
                    bot_closest_distance = bot_distance_to_node;
                }
                if (enemy_closest_distance > enemy_distance_to_node) {
                    enemy_node = i;
                    enemy_closest_distance = enemy_distance_to_node;
                }
                if (enemy_anchor_closest_distance > enemy_anchor_distance_to_node) {
                    enemy_anchor_node = i;
                    enemy_anchor_closest_distance = enemy_anchor_distance_to_node;
                }
            }
        }

        bool PathToEnemyNode::IsDefendingAnchor(behavior::ExecuteContext& ctx) {
            auto& game = ctx.hs->GetGame();
            if (flaggers_.enemy_anchor == nullptr) return true;

            //is defending anchor
            if (bot_node > enemy_anchor_node) return true;
            //if they are the same comapare distance to the next node closest to the flag room
            else if (bot_node == enemy_anchor_node) {

                if (game.GetPosition().Distance(base_path_[bot_node + 1]) < flaggers_.enemy_anchor->position.Distance(base_path_[bot_node + 1])) {
                    return true;
                }
                else return false;
            }

            //is not defending anchor
            return false;

        }

        float PathToEnemyNode::DistanceToEnemy(behavior::ExecuteContext& ctx) {
            auto& game = ctx.hs->GetGame();

            float distance = 0.0f;

            std::size_t start_node = 0;
            std::size_t end_node = 0;

            if (bot_node == enemy_node) {
                distance = game.GetPosition().Distance(enemy_);
                return distance;
            }

            //find what side of the nearest node the bot is on, add or subtract that distance
            if (game.GetPosition().Distance(base_path_[bot_node - 1]) < game.GetPosition().Distance(base_path_[bot_node + 1])) {
                if (bot_node < enemy_node) distance += game.GetPosition().Distance(base_path_[bot_node]);
                else if (bot_node > enemy_node) distance -= game.GetPosition().Distance(base_path_[bot_node]);
            }
            else {
                if (bot_node < enemy_node) distance -= game.GetPosition().Distance(base_path_[bot_node]);
                else if (bot_node > enemy_node) distance += game.GetPosition().Distance(base_path_[bot_node]);
            }

            //find what side of the nearest node the enemy is on and repeat
            if (enemy_.Distance(base_path_[enemy_node - 1]) < enemy_.Distance(base_path_[enemy_node + 1])) {
                if (bot_node < enemy_node) distance -= enemy_.Distance(base_path_[enemy_node]);
                else if (bot_node > enemy_node) distance += enemy_.Distance(base_path_[enemy_node]);
            }
            else {
                if (bot_node < enemy_node) distance += enemy_.Distance(base_path_[enemy_node]);
                else if (bot_node > enemy_node) distance -= enemy_.Distance(base_path_[enemy_node]);
            }

            //set lowest node number as the starting point

            //bot is closest to entrance
            if (bot_node < enemy_node) {
                start_node = bot_node;
                end_node = enemy_node;

                
            }
            else if (enemy_node < bot_node) {
                start_node = enemy_node;
                end_node = bot_node;
            }
            //calculate distance between each node point and add them together
            for (std::size_t i = start_node; i < end_node; i++) {
                distance += base_path_[i].Distance(base_path_[i + 1]);
            }

            return distance;
        }




        behavior::ExecuteResult PatrolNode::Execute(behavior::ExecuteContext& ctx) {

            auto& game = ctx.hs->GetGame();
            const Player& bot = game.GetPlayer();
            Vector2f from = game.GetPosition();
            std::vector<Vector2f> path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());

            auto nodes = ctx.blackboard.ValueOr("patrol_nodes", std::vector<Vector2f>());
            auto index = ctx.blackboard.ValueOr("patrol_index", 0);


            //this is where the anchor decides what base to move to
            bool flagging = (ctx.hs->GetGame().GetPlayer().frequency == 90 || ctx.hs->GetGame().GetPlayer().frequency == 91);

            //the base selector changes base every 10 mintues by adding one to gate
            int gate = ctx.hs->BaseSelector();
            //gate coords for each base
            std::vector<Vector2f> gates = { Vector2f(961, 62), Vector2f(961, 349), Vector2f(961, 961),
                                            Vector2f(512, 961), Vector2f(61, 960), Vector2f(62, 673), Vector2f(60, 62) };

            Vector2f center_gate_l = Vector2f(388, 396);
            Vector2f center_gate_r = Vector2f(570, 677);
            

            if (flagging) {

                Flaggers flaggers = ctx.hs->FindFlaggers();
                bool in_center = ctx.hs->GetRegions().IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));
               
                if (in_center) {
                    if (flaggers.enemy_anchor != nullptr) {
                        
                       bool enemy_in_1 = ctx.hs->GetRegions().IsConnected((MapCoord)flaggers.enemy_anchor->position, MapCoord(854, 358));
                       bool enemy_in_2 = ctx.hs->GetRegions().IsConnected((MapCoord)flaggers.enemy_anchor->position, MapCoord(823, 488));
                       bool enemy_in_3 = ctx.hs->GetRegions().IsConnected((MapCoord)flaggers.enemy_anchor->position, MapCoord(696, 817));

                        if (!ctx.hs->InHSBase(flaggers.enemy_anchor->position)) {
                            if (gate == 0 || gate == 1 || gate == 2) {
                                path = ctx.hs->GetPathfinder().CreatePath(path, from, center_gate_r, game.GetShipSettings().GetRadius());
                            }
                            else {
                                path = ctx.hs->GetPathfinder().CreatePath(path, from, center_gate_l, game.GetShipSettings().GetRadius());
                            }
                        }
                        else if (enemy_in_1 || enemy_in_2 || enemy_in_3) {
                            path = ctx.hs->GetPathfinder().CreatePath(path, from, center_gate_r, game.GetShipSettings().GetRadius());
                        }
                        else {
                            path = ctx.hs->GetPathfinder().CreatePath(path, from, center_gate_l, game.GetShipSettings().GetRadius());
                        }
                    }
                    else {
                        path = ctx.hs->GetPathfinder().CreatePath(path, from, center_gate_l, game.GetShipSettings().GetRadius());
                    }
                }
                //if bot is in tunnel
                if (ctx.hs->GetRegions().IsConnected((MapCoord)game.GetPosition(), MapCoord(27, 354))) {
                    if (flaggers.enemy_anchor != nullptr) {
                        
                        if (ctx.hs->InHSBase(flaggers.enemy_anchor->position)) {
                            std::vector<bool> in_number = ctx.hs->InHSBaseNumber(flaggers.enemy_anchor->position);
                            for (std::size_t i = 0; i < in_number.size(); i++) {
                                if (in_number[i]) path = ctx.hs->GetPathfinder().CreatePath(path, from, gates[i], game.GetShipSettings().GetRadius());
                            }
                        }
                        else path = ctx.hs->GetPathfinder().CreatePath(path, from, Vector2f(gates[gate]), game.GetShipSettings().GetRadius());
                    }
                    else path = ctx.hs->GetPathfinder().CreatePath(path, from, Vector2f(gates[gate]), game.GetShipSettings().GetRadius());
                }
                if (ctx.hs->InHSBase(game.GetPosition())) {
                    //camp is coords for position part way into base for bots to move to when theres no enemy
                    std::vector< Vector2f > camp = { Vector2f(855, 245), Vector2f(801, 621), Vector2f(773, 839),
                                                      Vector2f(554, 825), Vector2f(325, 877), Vector2f(208, 614), Vector2f(151, 253) };
                    std::vector<bool> in_number = ctx.hs->InHSBaseNumber(game.GetPosition());
                    for (std::size_t i = 0; i < in_number.size(); i++) {
                        if (in_number[i]) {
                            float dist_to_camp = bot.position.Distance(camp[i]);
                            path = ctx.hs->GetPathfinder().CreatePath(path, from, camp[i], game.GetShipSettings().GetRadius());
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

                path = ctx.hs->GetPathfinder().CreatePath(path, from, to, game.GetShipSettings().GetRadius());

            }

            ctx.blackboard.Set("path", path);

            return behavior::ExecuteResult::Success;
		}



        behavior::ExecuteResult FollowPathNode::Execute(behavior::ExecuteContext& ctx) {
            
            auto path = ctx.blackboard.ValueOr("path", std::vector<Vector2f>());

            size_t path_size = path.size();
            auto& game = ctx.hs->GetGame();

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

            ctx.hs->Move(current, 0.0f);

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

            const Player* target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
            if (!target_player) { return behavior::ExecuteResult::Failure; }

            auto& game = ctx.hs->GetGame();

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
                    bool in_center = ctx.hs->GetRegions().IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));
                    float energy_pct = ctx.hs->GetGame().GetEnergy() / (float)ctx.hs->GetGame().GetShipSettings().InitialEnergy;
                    if (energy_pct < 0.10f && !in_center && game.GetPlayer().bursts > 0) {
                        ctx.hs->GetGame().Burst(ctx.hs->GetKeys());
                    }
                }
            }
            else {
                const Player* target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
                if (target_player) {
                    float target_radius = game.GetSettings().ShipSettings[target_player->ship].GetRadius();
                    bool shoot = false;
                    bool bomb_hit = false;
                    Vector2f wall_pos;

                    if (BounceShot(game, target_player->position, target_player->velocity, target_radius, game.GetPlayer().GetHeading(), &bomb_hit, &wall_pos)) {
                        if (game.GetMap().GetTileId(game.GetPosition()) != marvin::kSafeTileId) {
                            if (bomb_hit) {
                                ctx.hs->GetKeys().Press(VK_TAB, ctx.com->GetTime(), 30);
                            }
                            else {
                                ctx.hs->GetKeys().Press(VK_CONTROL, ctx.com->GetTime(), 30);
                            }
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
            auto& game = ctx.hs->GetGame();

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
                if (CanShootGun(game, game.GetMap(), game.GetPosition(), solution)) {
                    return behavior::ExecuteResult::Success;
                }
            }
            return behavior::ExecuteResult::Failure;
        }




        behavior::ExecuteResult ShootEnemyNode::Execute(behavior::ExecuteContext& ctx) {
            const auto target_player = ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);
            const Player& target = *target_player; auto& game = ctx.hs->GetGame();
           

            uint64_t time = ctx.com->GetTime();
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
                ctx.hs->GetKeys().Press(VK_TAB, ctx.com->GetTime(), 50);
                //int chat_ = ctx.bot->GetGame().GetSelected();
                //std::string chat = std::to_string(chat_);
                //std::string chat = ctx.bot->GetGame().TickName();
                 //ctx.bot->GetGame().SendChatMessage(chat);
                ctx.blackboard.Set("BombCooldownExpire", time + bomb_delay);
            }
            else {
                ctx.hs->GetKeys().Press(VK_CONTROL, ctx.com->GetTime(), 50);
                ctx.blackboard.Set("GunExpire", time + gun_delay);
            }

            return behavior::ExecuteResult::Success;
        }

        


        behavior::ExecuteResult BundleShots::Execute(behavior::ExecuteContext& ctx) {
            
            const uint64_t cooldown = 50000000000000;
            const uint64_t duration = 1500;

            uint64_t bundle_expire = ctx.blackboard.ValueOr<uint64_t>("BundleCooldownExpire", 0);
            uint64_t current_time = ctx.com->GetTime();

            if (current_time < bundle_expire) return behavior::ExecuteResult::Failure;

            if (running_ && current_time >= start_time_ + duration) {
                float energy_pct =
                    ctx.hs->GetGame().GetEnergy() /
                    (float)ctx.hs->GetGame().GetShipSettings().InitialEnergy;
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
            ctx.hs->Move(target.position, 0.0f);
            ctx.hs->GetSteering().Face(target.position);

            ctx.hs->GetKeys().Press(VK_UP, ctx.com->GetTime(), 50);
            ctx.hs->GetKeys().Press(VK_CONTROL, ctx.com->GetTime(), 50);

            return behavior::ExecuteResult::Running;
        }

        bool BundleShots::ShouldActivate(behavior::ExecuteContext& ctx, const Player& target) {
            auto& game = ctx.hs->GetGame();
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
            auto& game = ctx.hs->GetGame();

            bool in_base = ctx.hs->GetRegions().IsConnected((MapCoord)game.GetPosition(), MapCoord(512, 512));

            Flaggers flaggers = ctx.hs->FindFlaggers();
            float energy_pct = (game.GetEnergy() / (float)game.GetShipSettings().InitialEnergy);

            Vector2f target_position = ctx.blackboard.ValueOr("shot_position", Vector2f());
            
            int ship = game.GetPlayer().ship;
            
            std::vector<float> ship_distance{ 5.0f, 10.0f, 5.0f, 15.0f, 5.0f, 10.0f, 5.0f, 5.0f };

            float hover_distance = 0.0f;

            if (ctx.hs->InHSBase(game.GetPosition())) {
                if (ship == 2) {
                    if (flaggers.team_anchor != nullptr) {
                        ctx.hs->Move(flaggers.team_anchor->position, 0.0f);
                        ctx.hs->GetSteering().Face(target_position);
                        return behavior::ExecuteResult::Success;
                    }
                    else ctx.hs->Move(target_position, 10.0f);
                    ctx.hs->GetSteering().Face(target_position);
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
                ctx.hs->GetGame().GetSettings().SpawnSettings[0].GetCoord();

            if (Vector2f(spawn.x, spawn.y)
                .DistanceSq(ctx.hs->GetGame().GetPosition()) < 35.0f * 35.0f) {
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


            if (energy_pct < 0.25f && !ctx.hs->InHSBase(game.GetPosition())) {
                Vector2f dodge;

                if (IsAimingAt(game, shooter, game.GetPlayer(), &dodge)) {
                    ctx.hs->Move(game.GetPosition() + dodge, 0.0f);
                    return behavior::ExecuteResult::Success;
                }
            }
            ctx.hs->Move(target_position, hover_distance);

            ctx.hs->GetSteering().Face(target_position);

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