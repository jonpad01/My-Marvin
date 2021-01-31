#include "Bot.h"

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

#include "Hyperspace.h"
#include "ExtremeGames.h"
#include "GalaxySports.h"
#include "Hockey.h"
#include "PowerBall.h"


namespace marvin {
    
    void RenderWorldLine(Vector2f screenCenterWorldPosition, Vector2f from, Vector2f to) {
        Vector2f center = GetWindowCenter();

        Vector2f diff = to - from;
        from = (from - screenCenterWorldPosition) * 16.0f;
        to = from + (diff * 16.0f);

        RenderLine(center + from, center + to, RGB(200, 0, 0));
    }

    void RenderPath(GameProxy& game, behavior::Blackboard& blackboard) {
        std::vector<Vector2f> path = blackboard.ValueOr("path", std::vector<Vector2f>());
        const Player& player = game.GetPlayer();

        if (path.empty()) return;

        for (std::size_t i = 0; i < path.size() - 1; ++i) {
            Vector2f current = path[i];
            Vector2f next = path[i + 1];

            if (i == 0) {
                RenderWorldLine(player.position, player.position, current);
            }

            RenderWorldLine(player.position, current, next);
        }
    }

    //the viewing area the bot could see if it were a player
    bool InRect(marvin::Vector2f pos, marvin::Vector2f min_rect,
        marvin::Vector2f max_rect) {
        return ((pos.x >= min_rect.x && pos.y >= min_rect.y) &&
            (pos.x <= max_rect.x && pos.y <= max_rect.y));
    }
    //probably checks if the target is dead
    bool IsValidPosition(marvin::Vector2f position) {
        return position.x >= 0 && position.x < 1024 && position.y >= 0 &&
            position.y < 1024;
    }

    

    marvin::Vector2f CalculateShot(const marvin::Vector2f& pShooter, const marvin::Vector2f& pTarget, const marvin::Vector2f& vShooter, const marvin::Vector2f& vTarget, float sProjectile) {
        marvin::Vector2f totarget = pTarget - pShooter;
        marvin::Vector2f v = vTarget - vShooter;

        float a = v.Dot(v) - sProjectile * sProjectile;
        float b = 2 * v.Dot(totarget);
        float c = totarget.Dot(totarget);

        marvin::Vector2f solution;

        float disc = (b * b) - 4 * a * c;
        float t = -1.0;

        if (disc >= 0.0) {
            float t1 = (-b + std::sqrt(disc)) / (2 * a);
            float t2 = (-b - std::sqrt(disc)) / (2 * a);
            if (t1 < t2 && t1 >= 0)
                t = t1;
            else
                t = t2;
        }

        solution = pTarget + (v * t);

        return solution;
    }
   
    //Path PathingNode::CreatePath(behavior::ExecuteContext& ctx, const std::string& pathname, Vector2f from, Vector2f to, float radius) {
    Path Bot::CreatePath(behavior::ExecuteContext& ctx, const std::string& pathname, Vector2f from, Vector2f to, float radius) {
        bool build = true;
        auto& game = ctx.bot->GetGame();


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
            path = ctx.bot->GetPathfinder().FindPath(game.GetMap(), mines, from, to, radius);
            path = ctx.bot->GetPathfinder().SmoothPath(path, game.GetMap(), radius);
        }

        return path;
    }

void Bot::Hyperspace() {
    
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
    
}

void Bot::GalaxySports() {

    auto freq_warp_attach = std::make_unique<gs::FreqWarpAttachNode>();
    auto find_enemy = std::make_unique<gs::FindEnemyNode>();
    auto looking_at_enemy = std::make_unique<gs::LookingAtEnemyNode>();
    auto target_in_los = std::make_unique<gs::InLineOfSightNode>(
        [](marvin::behavior::ExecuteContext& ctx) {
            const Vector2f* result = nullptr;

            const Player* target_player =
                ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            if (target_player) {
                result = &target_player->position;
            }
            return result;
        });

    auto shoot_enemy = std::make_unique<gs::ShootEnemyNode>();
    auto path_to_enemy = std::make_unique<gs::PathToEnemyNode>();
    auto move_to_enemy = std::make_unique<gs::MoveToEnemyNode>();
    auto follow_path = std::make_unique<gs::FollowPathNode>();
    auto patrol = std::make_unique<gs::PatrolNode>();
 

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
}

void Bot::ExtremeGames() {

    auto freq_warp_attach = std::make_unique<eg::FreqWarpAttachNode>();
    auto find_enemy = std::make_unique<eg::FindEnemyNode>();
    auto aim_with_gun = std::make_unique<eg::AimWithGunNode>();
    auto aim_with_bomb = std::make_unique<eg::AimWithBombNode>();
    auto target_in_los = std::make_unique<eg::InLineOfSightNode>(
        [](marvin::behavior::ExecuteContext& ctx) {
            const Vector2f* result = nullptr;

            const Player* target_player =
                ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            if (target_player) {
                result = &target_player->position;
            }
            return result;
        });

    auto shoot_gun = std::make_unique<eg::ShootGunNode>();
    auto shoot_bomb = std::make_unique<eg::ShootBombNode>();
    auto path_to_enemy = std::make_unique<eg::PathToEnemyNode>();
    auto move_to_enemy = std::make_unique<eg::MoveToEnemyNode>();
    auto follow_path = std::make_unique<eg::FollowPathNode>();
    auto patrol = std::make_unique<eg::PatrolNode>();


    auto move_method_selector = std::make_unique<behavior::SelectorNode>(move_to_enemy.get());
    auto gun_sequence = std::make_unique<behavior::SequenceNode>(aim_with_gun.get(), shoot_gun.get());
    auto bomb_sequence = std::make_unique<behavior::SequenceNode>(aim_with_bomb.get(), shoot_bomb.get());
    auto bomb_gun_sequence = std::make_unique<behavior::SelectorNode>(gun_sequence.get(), bomb_sequence.get());
    auto parallel_shoot_enemy = std::make_unique<behavior::ParallelNode>(bomb_gun_sequence.get(), move_method_selector.get());
    auto los_shoot_conditional = std::make_unique<behavior::SequenceNode>(target_in_los.get(), parallel_shoot_enemy.get());
    auto enemy_path_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy.get(), follow_path.get());
    auto patrol_path_sequence = std::make_unique<behavior::SequenceNode>(patrol.get(), follow_path.get());
    auto path_or_shoot_selector = std::make_unique<behavior::SelectorNode>(los_shoot_conditional.get(), enemy_path_sequence.get());
    auto handle_enemy = std::make_unique<behavior::SequenceNode>(find_enemy.get(), path_or_shoot_selector.get());
    auto root_selector = std::make_unique<behavior::SelectorNode>(freq_warp_attach.get(), handle_enemy.get(), patrol_path_sequence.get());

    behavior_nodes_.push_back(std::move(freq_warp_attach));
    behavior_nodes_.push_back(std::move(find_enemy));
    behavior_nodes_.push_back(std::move(aim_with_gun));
    behavior_nodes_.push_back(std::move(aim_with_bomb));
    behavior_nodes_.push_back(std::move(target_in_los));
    behavior_nodes_.push_back(std::move(shoot_gun));
    behavior_nodes_.push_back(std::move(shoot_bomb));
    behavior_nodes_.push_back(std::move(path_to_enemy));
    behavior_nodes_.push_back(std::move(move_to_enemy));
    behavior_nodes_.push_back(std::move(follow_path));
    behavior_nodes_.push_back(std::move(patrol));

    behavior_nodes_.push_back(std::move(move_method_selector));
    behavior_nodes_.push_back(std::move(bomb_gun_sequence));
    behavior_nodes_.push_back(std::move(bomb_sequence));
    behavior_nodes_.push_back(std::move(gun_sequence));
    behavior_nodes_.push_back(std::move(parallel_shoot_enemy));
    behavior_nodes_.push_back(std::move(los_shoot_conditional));
    behavior_nodes_.push_back(std::move(enemy_path_sequence));
    behavior_nodes_.push_back(std::move(patrol_path_sequence));
    behavior_nodes_.push_back(std::move(path_or_shoot_selector));
    behavior_nodes_.push_back(std::move(handle_enemy));
    behavior_nodes_.push_back(std::move(root_selector));
}


void Bot::HockeyZone() {

    auto freq_warp_attach = std::make_unique<hz::FreqWarpAttachNode>();
    auto find_enemy = std::make_unique<hz::FindEnemyNode>();
    auto looking_at_enemy = std::make_unique<hz::LookingAtEnemyNode>();
    auto target_in_los = std::make_unique<hz::InLineOfSightNode>(
        [](marvin::behavior::ExecuteContext& ctx) {
            const Vector2f* result = nullptr;

            const Player* target_player =
                ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            if (target_player) {
                result = &target_player->position;
            }
            return result;
        });

    auto shoot_enemy = std::make_unique<hz::ShootEnemyNode>();
    auto path_to_enemy = std::make_unique<hz::PathToEnemyNode>();
    auto move_to_enemy = std::make_unique<hz::MoveToEnemyNode>();
    auto follow_path = std::make_unique<hz::FollowPathNode>();
    auto patrol = std::make_unique<hz::PatrolNode>();


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
}

void Bot::PowerBall() {

    auto freq_warp_attach = std::make_unique<pb::FreqWarpAttachNode>();
    auto find_enemy = std::make_unique<pb::FindEnemyNode>();
    auto aim_with_gun = std::make_unique<pb::AimWithGunNode>();
    auto aim_with_bomb = std::make_unique<pb::AimWithBombNode>();
    auto target_in_los = std::make_unique<pb::InLineOfSightNode>(
        [](marvin::behavior::ExecuteContext& ctx) {
            const Vector2f* result = nullptr;

            const Player* target_player =
                ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

            if (target_player) {
                result = &target_player->position;
            }
            return result;
        });

    auto shoot_gun = std::make_unique<pb::ShootGunNode>();
    auto shoot_bomb = std::make_unique<pb::ShootBombNode>();
    auto path_to_enemy = std::make_unique<pb::PathToEnemyNode>();
    auto move_to_enemy = std::make_unique<pb::MoveToEnemyNode>();
    auto follow_path = std::make_unique<pb::FollowPathNode>();
    auto patrol = std::make_unique<pb::PatrolNode>();


    auto move_method_selector = std::make_unique<behavior::SelectorNode>(move_to_enemy.get());
    auto gun_sequence = std::make_unique<behavior::SequenceNode>(aim_with_gun.get(), shoot_gun.get());
    auto bomb_sequence = std::make_unique<behavior::SequenceNode>(aim_with_bomb.get(), shoot_bomb.get());
    auto bomb_gun_sequence = std::make_unique<behavior::SelectorNode>(gun_sequence.get(), bomb_sequence.get());
    auto parallel_shoot_enemy = std::make_unique<behavior::ParallelNode>(bomb_gun_sequence.get(), move_method_selector.get());
    auto los_shoot_conditional = std::make_unique<behavior::SequenceNode>(target_in_los.get(), parallel_shoot_enemy.get());
    auto enemy_path_sequence = std::make_unique<behavior::SequenceNode>(path_to_enemy.get(), follow_path.get());
    auto patrol_path_sequence = std::make_unique<behavior::SequenceNode>(patrol.get(), follow_path.get());
    auto path_or_shoot_selector = std::make_unique<behavior::SelectorNode>(los_shoot_conditional.get(), enemy_path_sequence.get());
    auto handle_enemy = std::make_unique<behavior::SequenceNode>(find_enemy.get(), path_or_shoot_selector.get());
    auto root_selector = std::make_unique<behavior::SelectorNode>(freq_warp_attach.get(), handle_enemy.get(), patrol_path_sequence.get());

    behavior_nodes_.push_back(std::move(freq_warp_attach));
    behavior_nodes_.push_back(std::move(find_enemy));
    behavior_nodes_.push_back(std::move(aim_with_gun));
    behavior_nodes_.push_back(std::move(aim_with_bomb));
    behavior_nodes_.push_back(std::move(target_in_los));
    behavior_nodes_.push_back(std::move(shoot_gun));
    behavior_nodes_.push_back(std::move(shoot_bomb));
    behavior_nodes_.push_back(std::move(path_to_enemy));
    behavior_nodes_.push_back(std::move(move_to_enemy));
    behavior_nodes_.push_back(std::move(follow_path));
    behavior_nodes_.push_back(std::move(patrol));

    behavior_nodes_.push_back(std::move(move_method_selector));
    behavior_nodes_.push_back(std::move(bomb_gun_sequence));
    behavior_nodes_.push_back(std::move(bomb_sequence));
    behavior_nodes_.push_back(std::move(gun_sequence));
    behavior_nodes_.push_back(std::move(parallel_shoot_enemy));
    behavior_nodes_.push_back(std::move(los_shoot_conditional));
    behavior_nodes_.push_back(std::move(enemy_path_sequence));
    behavior_nodes_.push_back(std::move(patrol_path_sequence));
    behavior_nodes_.push_back(std::move(path_or_shoot_selector));
    behavior_nodes_.push_back(std::move(handle_enemy));
    behavior_nodes_.push_back(std::move(root_selector));
}

//Bot::Bot(std::shared_ptr<GameProxy> game) : game_(std::move(game)), steering_(this) {
Bot::Bot(std::shared_ptr<marvin::GameProxy> game, std::shared_ptr<marvin::GameProxy> game2) : game_(std::move(game)), steering_(std::move(game2)), keys_(GetTime()) {
 
  auto processor = std::make_unique<path::NodeProcessor>(*game_);
  
  last_ship_change_ = 0;
  last_base_change_ = 0;
  ship_ = game_->GetPlayer().ship;
  

  if (ship_ > 7) {
    ship_ = 0;
  }
  
  pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor));
  regions_ = RegionRegistry::Create(game_->GetMap());
  pathfinder_->CreateMapWeights(game_->GetMap());

  if (game_->GetServerFolder() == "zones\\SSCE Hyperspace") {
      Hyperspace();
      CreateHSBasePaths();
      behavior_ctx_.blackboard.Set("patrol_nodes", std::vector<Vector2f>({ Vector2f(585, 540), Vector2f(400, 570) }));
      behavior_ctx_.blackboard.Set("patrol_index", 0);
  }

  else if (game_->GetServerFolder() == "zones\\SSCU Extreme Games") ExtremeGames();
  else if (game_->GetServerFolder() == "zones\\SSCJ Galaxy Sports") GalaxySports();
  else if (game_->GetServerFolder() == "zones\\SSCE HockeyFootball Zone") HockeyZone();
  else if (game_->GetServerFolder() == "zones\\SSCJ PowerBall") {
      PowerBall();
      FindPowerBallGoal();
      powerball_arena_ = game_->GetMapFile();
  }

  behavior_ = std::make_unique<behavior::BehaviorEngine>(behavior_nodes_.back().get());
  behavior_ctx_.bot = this;
}

//the bot update is the first step in the flow, called from main.cpp, from here it calls the behavior tree
void Bot::Update(float dt) {
    keys_.ReleaseAll();
    game_->Update(dt);

    has_wall_shot = false;
    
    uint64_t timestamp = GetTime();
    uint64_t ship_cooldown = 10000;

    //check chat for disconected message or spec message in eg and terminate continuum
    std::string name = game_->GetName();
    std::string disconnected = "WARNING: ";
    std::string eg_specced = "[ " + name + " ]";
    std::size_t len = 4 + name.size();

    std::vector<std::string> chat = game_->GetChat(0);

    for (std::size_t i = 0; i < chat.size(); i++) {
        if (chat[i].compare(0, 9, disconnected) == 0 || chat[i].compare(0, len, eg_specced) == 0) {
            exit(5);
        }
    }

    //then check if specced for lag
    if (game_->GetPlayer().ship > 7) {
        uint64_t timestamp = GetTime();
        if (timestamp - last_ship_change_ > ship_cooldown) {
            if (game_->SetShip(ship_)) {
                last_ship_change_ = timestamp;
            }
        }
        return;
    }

    /*these are some calculations that are needed in multiple behavior nodes, they are private members that 
    create and modify private variables once, and when a behavior node needs that information it calls a public member */

    if (game_->GetServerFolder() == "zones\\SSCE Hyperspace") {
        CheckCenterRegion();
    }

    if (game_->GetServerFolder() == "zones\\SSCJ PowerBall") {
        //look at alive time to see if bot has changed arena, then reset pointers for the map
        if (powerball_arena_ != game_->GetMapFile()) {

            powerball_arena_ = game_->GetMapFile();

            auto processor = std::make_unique<path::NodeProcessor>(*game_);
            pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor));

            behavior_ctx_.blackboard.Set("path", Path());
            regions_ = RegionRegistry::Create(game_->GetMap());
            pathfinder_->CreateMapWeights(game_->GetMap());
            
            //set new goals
            FindPowerBallGoal();
        }  
        if (!regions_->IsConnected((MapCoord)game_->GetPosition(), (MapCoord)powerball_goal_)) {
            FindPowerBallGoal();
        }
    }

  steering_.Reset();
 
  
  behavior_ctx_.dt = dt;

  behavior_->Update(behavior_ctx_);

#if DEBUG_RENDER
 // RenderPath(GetGame(), behavior_ctx_.blackboard);
#endif

  Steer();

}

void Bot::Steer() {
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

          if (behind) { keys_.Press(VK_DOWN, GetTime(), 30); }
          else { keys_.Press(VK_UP, GetTime(), 30); }

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
          //keys_.Set(VK_RIGHT, clockwise, GetTime());
          //keys_.Set(VK_LEFT, !clockwise, GetTime());
      }

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
}

void Bot::Move(const Vector2f& target, float target_distance) {
  const Player& bot_player = game_->GetPlayer();
  float distance = bot_player.position.Distance(target);
  Vector2f heading = game_->GetPlayer().GetHeading();
  
  Vector2f target_position = behavior_ctx_.blackboard.ValueOr("target_position", Vector2f());
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

uint64_t Bot::GetTime() const {
    return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
}

void Bot::FindPowerBallGoal() {

    powerball_goal_;
    powerball_goal_path_;

    float closest_distance = std::numeric_limits<float>::max();
    int alive_time = game_->GetSettings().BombAliveTime;

    //player is in original
    if (game_->GetMapFile() == "powerlg.lvl") {
        if (game_->GetPlayer().frequency == 00) {
            powerball_goal_ = Vector2f(945, 512);

            std::vector<Vector2f> goal1_paths{ Vector2f(945, 495), Vector2f(926, 512), Vector2f(962, 511), Vector2f(944, 528) };

            for (std::size_t i = 0; i < goal1_paths.size(); i++) {
                float distance = goal1_paths[i].Distance(game_->GetPosition());
                if (distance < closest_distance) {
                    powerball_goal_path_ = goal1_paths[i];
                    closest_distance = distance;
                }
            }
        }
        else {
            powerball_goal_ = Vector2f(72, 510);

            std::vector<Vector2f> goal0_paths{ Vector2f(89, 510), Vector2f(72, 526), Vector2f(55, 509), Vector2f(73, 492) };

            for (std::size_t i = 0; i < goal0_paths.size(); i++) {
                float distance = goal0_paths[i].Distance(game_->GetPosition());
                if (distance < closest_distance) {
                    powerball_goal_path_ = goal0_paths[i];
                    closest_distance = distance;
                }
            }
        }
    }
    //player is in pub
    else if (game_->GetMapFile() == "combo_pub.lvl") {

        std::vector<Vector2f> arenas{ Vector2f(512, 133), Vector2f(512, 224), Vector2f(512, 504), Vector2f(512, 775) };
        std::vector<Vector2f> goal_0{ Vector2f(484, 137), Vector2f(398, 263), Vector2f(326, 512), Vector2f(435, 780) };
        std::vector<Vector2f> goal_1{ Vector2f(540, 137), Vector2f(625, 263), Vector2f(697, 512), Vector2f(587, 780) };

        for (std::size_t i = 0; i < arenas.size(); i++) {

            if (GetRegions().IsConnected((MapCoord)game_->GetPosition(), arenas[i])) {
                if (game_->GetPlayer().frequency == 00) {
                    powerball_goal_ = goal_1[i];
                    powerball_goal_path_ = powerball_goal_;
                }
                else {
                    powerball_goal_ = goal_0[i];
                    powerball_goal_path_ = powerball_goal_;
                }
            }
        }
    }
}

void Bot::CheckHSBaseRegions(Vector2f position) {

   hs_in_1 = regions_->IsConnected((MapCoord)position, MapCoord(854, 358));
   hs_in_2 = regions_->IsConnected((MapCoord)position, MapCoord(823, 488));
   hs_in_3 = regions_->IsConnected((MapCoord)position, MapCoord(696, 817));
   hs_in_4 = regions_->IsConnected((MapCoord)position, MapCoord(587, 776));
   hs_in_5 = regions_->IsConnected((MapCoord)position, MapCoord(125, 877));
   hs_in_6 = regions_->IsConnected((MapCoord)position, MapCoord(254, 493));
   hs_in_7 = regions_->IsConnected((MapCoord)position, MapCoord(154, 358));
   hs_in_8 = regions_->IsConnected((MapCoord)position, MapCoord(620, 250));
}

bool Bot::InHSBase(Vector2f position) {
    CheckHSBaseRegions(position);
    return hs_in_1 || hs_in_2 || hs_in_3 || hs_in_4 || hs_in_5 || hs_in_6 || hs_in_7;
}

std::vector<bool> Bot::InHSBaseNumber(Vector2f position) {
    CheckHSBaseRegions(position);
    std::vector<bool> in = { hs_in_1, hs_in_2, hs_in_3, hs_in_4, hs_in_5, hs_in_6, hs_in_7 };
    return in;
}

int Bot::BaseSelector() {
    uint64_t timestamp = GetTime();
    bool base_cooldown = timestamp - last_base_change_ > 200000;
    int base = behavior_ctx_.blackboard.ValueOr<int>("base", 5);
    if (base_cooldown) {
        base++;
        behavior_ctx_.blackboard.Set<int>("base", base);
        last_base_change_ = timestamp;
    }
    
    if (base > 6) {
        base = 0;
        behavior_ctx_.blackboard.Set<int>("base", base);
    }
    return base;
}
Flaggers Bot::FindFlaggers() {

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


bool Bot::CheckStatus() {
    float energy_pct = (game_->GetEnergy() / (float)game_->GetShipSettings().InitialEnergy) * 100.0f;
    bool result = false;
    if ((game_->GetPlayer().status & 2) != 0) game_->Cloak(keys_);
    if ((game_->GetPlayer().status & 1) != 0) game_->Stealth();
    if (energy_pct == 100.0f) result = true;
    return result;
}

void Bot::CreateHSBasePaths() {

    std::vector<Vector2f> base_entrance = { Vector2f(827, 339), Vector2f(811, 530), Vector2f(729, 893),
                                            Vector2f(444, 757), Vector2f(127, 848), Vector2f(268, 552), Vector2f(181, 330) };
    std::vector<Vector2f> flag_room = { Vector2f(826, 229), Vector2f(834, 540), Vector2f(745, 828),
                                        Vector2f(489, 832), Vector2f(292, 812), Vector2f(159, 571), Vector2f(205, 204) };

    std::vector<Vector2f> mines;

    for (std::size_t i = 0; i < base_entrance.size(); i++) {
        
        Path base_path = GetPathfinder().FindPath(game_->GetMap(), mines, base_entrance[i], flag_room[i], game_->GetSettings().ShipSettings[6].GetRadius());
        base_path = GetPathfinder().SmoothPath(base_path, game_->GetMap(), game_->GetSettings().ShipSettings[6].GetRadius());
        
        hs_base_paths_.push_back(base_path);
    }
}

void Bot::CheckCenterRegion() {
    in_center = regions_->IsConnected((MapCoord)game_->GetPosition(), MapCoord(512, 512));
}

std::size_t Bot::FindClosestPathNode(Vector2f position) {
    //this can probably be used for both deva and hs base paths

    std::size_t node = 0;
 
    return node;
}

float Bot::PathLength(Vector2f point_1, Vector2f point_2) {
    //returns the index to the closest path node
    //change this, feed path as argument and use distance
    std::size_t node1 = FindClosestPathNode(point_1);
    std::size_t node2 = FindClosestPathNode(point_2);

    RenderText(std::to_string(node1), GetWindowCenter() - Vector2f(200, 200), RGB(100, 100, 100), RenderText_Centered);
    RenderText(std::to_string(node2), GetWindowCenter() - Vector2f(200, 100), RGB(100, 100, 100), RenderText_Centered);

    float distance = 0.0f;

    if (node1 > node2) {
        distance = (float)(node1 - node2);
    }
    else {
        distance = (float)(node2 - node1);
    }
   
    return distance;
}

int Bot::FindOpenFreq() {

    int open_freq = 0;
    
    for (std::size_t i = 2; i < freq_list.size(); i++) {
        if (freq_list[i] == 0) {
            open_freq = i;
            break;
        }
    }
    return open_freq;
}

bool Bot::CanShootGun(const Map& map, const Player& bot_player, const Player& target) {

    float bullet_speed = game_->GetSettings().ShipSettings[game_->GetPlayer().ship].BulletSpeed / 10.0f / 16.0f;
    float speed_adjust = (game_->GetPlayer().velocity * game_->GetPlayer().GetHeading());

    float bullet_travel = ((bullet_speed + speed_adjust) / 100.0f) * game_->GetSettings().BulletAliveTime;

    if (bot_player.position.Distance(target.position) > bullet_travel) return false;
    if (map.GetTileId(bot_player.position) == marvin::kSafeTileId) return false;

    return true;
}

bool Bot::CanShootBomb(const Map& map, const Player& bot_player, const Player& target) {

    float bomb_speed = game_->GetSettings().ShipSettings[game_->GetPlayer().ship].BombSpeed / 10.0f / 16.0f;
    float speed_adjust = (game_->GetPlayer().velocity * game_->GetPlayer().GetHeading());

    float bomb_travel = ((bomb_speed + speed_adjust) / 100.0f) * game_->GetSettings().BombAliveTime;

    if (bot_player.position.Distance(target.position) > bomb_travel) return false;
    if (map.GetTileId(bot_player.position) == marvin::kSafeTileId) return false;

    return true;
}

void Bot::LookForWallShot(Vector2f target_pos, Vector2f target_vel, float proj_speed, int alive_time, uint8_t bounces) {

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

bool Bot::CalculateWallShot(Vector2f target_pos, Vector2f target_vel, Vector2f desired_direction, float proj_speed, int alive_time, uint8_t bounces) {

    Vector2f direction = desired_direction;
    Vector2f position = game_->GetPosition();
    Vector2f center = GetWindowCenter();
    Vector2f center_adjust = GetWindowCenter();
    float wall_debug_y = 300.0f;

   // RenderText("Bot::CalculateWallShot", center - Vector2f(wall_debug_y, 300), RGB(100, 100, 100), RenderText_Centered);
    
    float speed_adjust = (game_->GetPlayer().velocity * game_->GetPlayer().GetHeading());
    float travel = (proj_speed + speed_adjust) * (alive_time / 100.0f);
    float max_travel = travel; 

   // RenderText("Speed Adjust: " + std::to_string(speed_adjust), center - Vector2f(wall_debug_y, 280), RGB(100, 100, 100), RenderText_Centered);
   // RenderText("Bullet Travel: " + std::to_string(travel), center - Vector2f(wall_debug_y, 260), RGB(100, 100, 100), RenderText_Centered);

        for (int i = 0; i < bounces + 1; ++i) {

            Vector2f to_target = target_pos - position;
            Vector2f target_direction = Normalize(target_pos - position);

            //adjust the calculate shot function for the time it takes the bullet to reach the position its calculating from
            float seek_speed = ((proj_speed + speed_adjust)) - (max_travel - travel);

            //this shot is calculated from the wall the bullet is bouncing off of, bullet speed may not be accurate
            Vector2f seek_position = CalculateShot(position, target_pos, Vector2f(0, 0), target_vel, proj_speed + speed_adjust);

            Vector2f to_seek = seek_position - position;
            Vector2f seek_direction = Normalize(to_seek);

           // RenderLine(center, center + (seek_direction * to_seek.Length() * 16), RGB(120, 120, 120));

            if (target_pos.Distance(seek_position) > travel) {
               // RenderLine(center, center + (direction * travel * 16), RGB(100, 100, 100));
                //return false;
            }

            //RenderLine(center + box_min, center + box_min + (target_direction * test_radius * 16), RGB(100, 100, 100));

            if (i != 0) {

                float test_radius = 6.0f;
                Vector2f box_min = seek_position - Vector2f(test_radius, test_radius);
                Vector2f box_extent(test_radius * 1.2f, test_radius * 1.2f);
                float dist;
                Vector2f norm;
                
                if (RayBoxIntersect(position, direction, box_min, box_extent, &dist, &norm)) {
                    if (dist < travel) {
                      //  RenderText("Box Intersected", center - Vector2f(wall_debug_y, 240), RGB(100, 100, 100), RenderText_Centered);

                        CastResult target_line = RayCast(game_->GetMap(), position, seek_direction, to_seek.Length());

                        if (!target_line.hit) {
                           // RenderText("SHOOT", center - Vector2f(wall_debug_y, 220), RGB(100, 100, 100), RenderText_Centered);

                           // RenderLine(center_adjust, center_adjust + (direction * to_target.Length() * 16), RGB(100, 100, 100));
                          //  RenderLine(center_adjust, center_adjust + (seek_direction * to_seek.Length() * 16), RGB(200, 200, 200));
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

}  // namespace marvin
