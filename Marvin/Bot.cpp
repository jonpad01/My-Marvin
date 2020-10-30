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
#include "Hyperspace.h"
#include "Devastation.h"
#include "ExtremeGames.h"
#include "GalaxySports.h"
#include "Hockey.h"
#include "PowerBall.h"


namespace marvin {
    
//#if 0
    
//#endif
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

    float PathingNode::PathLength(behavior::ExecuteContext& ctx, Vector2f from, Vector2f to) {
        auto& game = ctx.bot->GetGame();
        std::vector< Vector2f > mines;
        Path path = ctx.bot->GetPathfinder().FindPath(game.GetMap(), mines, from, to, game.GetSettings().ShipSettings[6].GetRadius());
        //Path path = CreatePath(ctx, "length", from, to, game.GetSettings().ShipSettings[6].GetRadius());
        float length = from.Distance(path[0]);
        
        if (path.size() == 0) {
            return length = from.Distance(to);
        }

        if (path.size() > 1) {
            for (std::size_t i = 0; i < (path.size() - 1); i++) {
                Vector2f point_1 = path[i];
                Vector2f point_2 = path[i + 1];
                length += point_1.Distance(point_2);
            }
        }
        return length;
    }

    Path PathingNode::CreatePath(behavior::ExecuteContext& ctx, const std::string& pathname, Vector2f from, Vector2f to, float radius) {
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
                CastResult side1 =
                    RayCast(game.GetMap(), pos + side * radius, direction, distance);
                CastResult side2 =
                    RayCast(game.GetMap(), pos - side * radius, direction, distance);

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

void Bot::Devastation() {

    auto freq_warp_attach = std::make_unique<deva::FreqWarpAttachNode>();
    auto find_enemy = std::make_unique<deva::FindEnemyNode>();
    auto looking_at_enemy = std::make_unique<deva::LookingAtEnemyNode>();
    auto target_in_los = std::make_unique<deva::InLineOfSightNode>(
        [](marvin::behavior::ExecuteContext& ctx) {
            const Vector2f* result = nullptr;

            const Player* target_player =
                ctx.blackboard.ValueOr<const Player*>("target_player", nullptr);

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

Bot::Bot(std::unique_ptr<GameProxy> game) : game_(std::move(game)), steering_(this) {
  auto processor = std::make_unique<path::NodeProcessor>(*game_);
  
  last_ship_change_ = 0;
  last_base_change_ = 0;
  ship_ = game_->GetPlayer().ship;
  

  if (ship_ > 7) {
    ship_ = 0;
  }
  
  pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor));
  regions_ = RegionRegistry::Create(game_->GetMap());

  if (game_->Zone() == "hyperspace") {
      Hyperspace();
      CreateHSBasePaths();
      behavior_ctx_.blackboard.Set("patrol_nodes", std::vector<Vector2f>({ Vector2f(585, 540), Vector2f(400, 570) }));
      behavior_ctx_.blackboard.Set("patrol_index", 0);
  }
  else if (game_->Zone() == "devastation") {
      Devastation();
      FindDevaSafeTiles();
      CreateDevaBasePaths();
  }
  else if (game_->Zone() == "extreme games") ExtremeGames();
  else if (game_->Zone() == "galaxy sports") GalaxySports();
  else if (game_->Zone() == "hockey") HockeyZone();
  else if (game_->Zone() == "powerball") PowerBall();

  behavior_ = std::make_unique<behavior::BehaviorEngine>(behavior_nodes_.back().get());

}
//the bot update is the first step in the flow, called from main.cpp, from here it calls the behavior tree
void Bot::Update(float dt) {
    keys_.ReleaseAll();
    game_->Update(dt);
    
    uint64_t timestamp = GetTime();
    uint64_t ship_cooldown = 10000;
    if (game_->Zone() == "devastation") ship_cooldown = 10800000;

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

    if (game_->Zone() == "hyperspace") {
        CheckCenterRegion();
    }

    if (game_->Zone() == "devastation") {
        has_wall_shot = false;
        SortDevaPlayers();
        CheckCenterRegion();
        //LookForWallShot(Vector2f(0, 0));

        //check if bot is in a base
        if (!in_center && game_->GetPlayer().active) {

            std::size_t i = behavior_ctx_.blackboard.ValueOr<std::size_t>("LastBase", 0);
            bool check_again = behavior_ctx_.blackboard.ValueOr<bool>("CheckAgain", false);
            //check if bot is in a new base
            if (!regions_->IsConnected((MapCoord)game_->GetPosition(), MapCoord(deva_base_paths[i][0])) || check_again) {
                //find the new base and set the index, this index is used by other members so it must be set first
                SetDevaBaseRegion();
                //determine team and enemy safe, if it cant then keep trying
                if (!SetDevaCurrentSafes()) behavior_ctx_.blackboard.Set("CheckAgain", true);
                else { behavior_ctx_.blackboard.Set("CheckAgain", false); }
                
                behavior_ctx_.blackboard.Set("LastBase", index);
            }
        }
    }


  steering_.Reset();

  behavior_ctx_.bot = this;
  behavior_ctx_.dt = dt;

  behavior_->Update(behavior_ctx_);

  Steer();
}

void Bot::Steer() {
    Vector2f center = GetWindowCenter();
    float debug_y = 100.0f;
  Vector2f force = steering_.GetSteering();
  float rotation = steering_.GetRotation();

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
          if (behind) {
              keys_.Press(VK_DOWN);
          }
          else {
              keys_.Press(VK_UP);
          }
      }

      if (heading.Dot(steering_direction) < 1.0f) {
          keys_.Set(VK_RIGHT, clockwise);
          keys_.Set(VK_LEFT, !clockwise);
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
      if (flaggers.enemy_anchor != nullptr) {
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

Vector2f Bot::FindPowerBallGoal() {

    Vector2f goal;

    std::vector<Vector2f> arenas { Vector2f(512, 133), Vector2f(512, 224), Vector2f(512, 504), Vector2f(512, 775) };
    std::vector<Vector2f> goal_0 { Vector2f(484, 137), Vector2f(398, 263), Vector2f(326, 512), Vector2f(435, 780) };
    std::vector<Vector2f> goal_1 { Vector2f(540, 137), Vector2f(625, 263), Vector2f(697, 512), Vector2f(587, 780) };

        for (std::size_t i = 0; i < arenas.size(); i++) {

            if (GetRegions().IsConnected((MapCoord)game_->GetPosition(), arenas[i])) {
                if (game_->GetPlayer().frequency == 00) {
                    goal = goal_1[i];
                }
                else {
                    goal = goal_0[i];
                }
            }
        }   
    return goal;
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
    float closest_distance = std::numeric_limits<float>::max();

    for (std::size_t i = 0; i < deva_base_paths[index].size(); i++) {

        float distance_to_node = position.Distance(deva_base_paths[index][i]);

        if (closest_distance > distance_to_node) {
            node = i;
            closest_distance = distance_to_node;
        }
    }
    return node;
}

float Bot::PathLength(Vector2f point_1, Vector2f point_2) {
    //returns the index to the closest path node
    std::size_t node_1 = FindClosestPathNode(point_1);
    std::size_t node_2 = FindClosestPathNode(point_2);

    float distance = 0.0f;
    std::size_t start_node = 0;
    std::size_t end_node = 0;
    Vector2f start_point;
    Vector2f end_point;

    //sort the start position by lowest index in the vector, so the loop can count up from the start
    if (node_1 < node_2) {
        start_node = node_1;
        start_point = point_1;
        end_node = node_2;
        end_point = point_2;

    }
    else if (node_1 > node_2) {
        start_node = node_2;
        start_point = point_2;
        end_node = node_1;
        end_point = point_1;
    }
    else if (node_1 == node_2) {
        return point_1.Distance(point_2);
    }

    //start by finding the distance to the path node ahead of the start and behind the end node
    
    float start_node_distance = deva_base_paths[index][start_node].Distance(deva_base_paths[index][start_node + 1]);
    float start_point_distance = start_point.Distance(deva_base_paths[index][start_node + 1]);

    //compare distance to the next node up to the players distance to the next node up, if the players distance is greater then its on the other side of the node
    if (start_point_distance > start_node_distance) {
        distance += start_point.Distance(deva_base_paths[index][start_node]);
    }
    else {
        distance -= start_point.Distance(deva_base_paths[index][start_node]);
    }

    float end_node_distance = deva_base_paths[index][end_node].Distance(deva_base_paths[index][end_node - 1]);
    float end_point_distance = end_point.Distance(deva_base_paths[index][end_node - 1]);

    if (end_point_distance > end_node_distance) {
        distance += end_point.Distance(deva_base_paths[index][end_node]);
    }
    else {
        distance -= end_point.Distance(deva_base_paths[index][end_node]);
    }

    //then add distance to each node point
    for (std::size_t i = start_node; i < end_node; i++) {
        distance += deva_base_paths[index][i].Distance(deva_base_paths[index][i + 1]);
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

void Bot::SortDevaPlayers() {

    const Player& bot = game_->GetPlayer();

    freq_list.clear();
    freq_list.resize(100, 0);
    deva_team.clear();
    deva_enemy_team.clear();
    deva_duelers.clear();
    last_bot_standing = true;

    //find team sizes and push team mates into a vector
    for (std::size_t i = 0; i < game_->GetPlayers().size(); i++) {
        const Player& player = game_->GetPlayers()[i];

        if (player.frequency < 100) freq_list[player.frequency]++;

        if ((player.frequency == 00 || player.frequency == 01) && (player.id != bot.id)) {

            deva_duelers.push_back(game_->GetPlayers()[i]);

            if (player.frequency == bot.frequency) {
                deva_team.push_back(game_->GetPlayers()[i]);
            }
            else {
                deva_enemy_team.push_back(game_->GetPlayers()[i]);
            }
        }
    }

    if (deva_team.size() == 0) {
        last_bot_standing = false;
    }
    else {
        //check if team is in base
        for (std::size_t i = 0; i < deva_team.size(); i++) {
            const Player& player = deva_team[i];

            if (!regions_->IsConnected((MapCoord)player.position, MapCoord(512, 512)) && player.active) {
                last_bot_standing = false;
                break;
            }
        }
    }
}

void Bot::SetDevaBaseRegion() {

    index = 0;

    for (std::size_t i = 0; i < base_safe_1.size(); i++) {

        if (regions_->IsConnected((MapCoord)game_->GetPosition(), MapCoord(base_safe_1[i]))) {
            index = i;
            break;
        }
    }
}

bool Bot::SetDevaCurrentSafes() {
 
    //this member should only update when the bot enters a new base

    team_safe;
    enemy_safe;

    Vector2f safe_1(base_safe_1[index].x, base_safe_1[index].y);
    Vector2f safe_2(base_safe_2[index].x, base_safe_2[index].y);

    //if the bot is waped in at the game start, it can determine safe by distance
    if (safe_1.Distance(game_->GetPosition()) < 4.0f) {
        team_safe = safe_1;
        enemy_safe = safe_2;
        return true;
    }
    else if (safe_2.Distance(game_->GetPosition()) < 4.0f) {
        team_safe = safe_2;
        enemy_safe = safe_1;
        return true;
    }

    //if the bot is attaching into a new base then it wont be close to a safe zone, find the closest player to each safe zone

    bool team_is_closest_to_1 = false;
    bool team_is_closest_to_2 = false;

    float closest_to_1 = std::numeric_limits<float>::max();
    float closest_to_2 = std::numeric_limits<float>::max();

    for (std::size_t i = 0; i < game_->GetPlayers().size(); i++) {
        const Player& player = game_->GetPlayers()[i];

        if (player.frequency != 00 && player.frequency != 01) continue;
        if (!regions_->IsConnected((MapCoord)game_->GetPosition(), MapCoord(512, 512)) && player.active) {
           
            float distance_to_1 = PathLength(player.position, safe_1);
            float distance_to_2 = PathLength(player.position, safe_2);

            if (distance_to_1 < closest_to_1) {
                if (player.frequency == game_->GetPlayer().frequency) {
                    team_is_closest_to_1 = true;
                }
                else team_is_closest_to_1 = false;
                closest_to_1 = distance_to_1;
            }
            if (distance_to_2 < closest_to_2) {
                if (player.frequency == game_->GetPlayer().frequency) {
                    team_is_closest_to_2 = true;
                }
                else team_is_closest_to_2 = false;
                closest_to_2 = distance_to_2;
            } 
        }
    }

    if (team_is_closest_to_1 && !team_is_closest_to_2) {
        team_safe = safe_1;
        enemy_safe = safe_2;
    }
    else if (!team_is_closest_to_1 && team_is_closest_to_2) {
        team_safe = safe_2;
        enemy_safe = safe_1;
    }
    else return false;

    return true;

}

void Bot::FindDevaSafeTiles() {

    std::vector<MapCoord> random_tiles;

    base_safe_1;
    base_safe_2;

    //a vector of vectors, each vector contains the safe tiles from the same base
    std::vector<std::vector<MapCoord>> base_tiles;
 
    //grab all safe tiles and store into a vector
    for (uint16_t y = 0; y < 1024; ++y) {
        for (uint16_t x = 0; x < 1024; ++x) {
            Vector2f coord(x, y);

            if (game_->GetMap().GetTileId(coord) == kSafeTileId) {
                //ignore center safe
                if (regions_->IsConnected((MapCoord)coord, MapCoord(512, 512))) continue;
                if (regions_->IsConnected((MapCoord)coord, MapCoord(606, 459))) continue;

                MapCoord current(x, y);

                const MapCoord northwest(current.x - 1, current.y - 1);
                const MapCoord southwest(current.x - 1, current.y + 1);
                const MapCoord northeast(current.x + 1, current.y - 1);
                const MapCoord southeast(current.x + 1, current.y + 1);
                const MapCoord west(current.x - 1, current.y);
                const MapCoord east(current.x + 1, current.y);
                const MapCoord north(current.x, current.y - 1);
                const MapCoord south(current.x, current.y + 1);
                //skip tiles next to walls
                if (game_->GetMap().IsSolid(west.x, west.y) || game_->GetMap().IsSolid(east.x, east.y) || game_->GetMap().IsSolid(north.x, north.y) || game_->GetMap().IsSolid(south.x, south.y) ||
                    game_->GetMap().IsSolid(northwest.x, northwest.y) || game_->GetMap().IsSolid(northeast.x, northeast.y) || game_->GetMap().IsSolid(southwest.x, southwest.y) ||
                    game_->GetMap().IsSolid(southeast.x, southeast.y)) {
                    continue;
                }

                random_tiles.push_back(coord);
            }
        }
    }

    //separate tiles by connected bases
    for (std::size_t i = 0; i < random_tiles.size(); i++) {

        std::vector<MapCoord> base_i;
        base_i.push_back(random_tiles[i]);

        //compare the current tile to each other tile in the vector
        for (std::size_t j = i + 1; j < random_tiles.size(); j++) {

            if (random_tiles[i] == random_tiles[j]) continue;
            //if connected store that with the other tile
            if (regions_->IsConnected((MapCoord)random_tiles[i], (MapCoord)random_tiles[j])) {

                base_i.push_back(random_tiles[j]);
                //then delete it from the random tiles so it wont end up as an extra base
                random_tiles.erase(random_tiles.begin() + j);
                //and set the index back since the vector just shrank by one value
                j--;
            }
        }
        //add this base vector of safe tiles to a vector of bases
        base_tiles.push_back(base_i);
    }

    //take the first tile, match it to a tile with distance greater than 4, and store both, these are the safe coords for each base
    for (std::size_t i = 0; i < base_tiles.size(); i++) {
        
        MapCoord side_1 = base_tiles[i][0];
        MapCoord side_2(0, 0);

        Vector2f current(base_tiles[i][0].x, base_tiles[i][0].y);
        
        for (std::size_t j = 1; j < base_tiles[i].size(); j++) {

            Vector2f next(base_tiles[i][j].x, base_tiles[i][j].y);

            if (current.Distance(next) > 4.0f) {
              
                side_2 = base_tiles[i][j];
                break;
            }   
        }
        base_safe_1.push_back(side_1);
        base_safe_2.push_back(side_2);
    }
}

void Bot::CreateDevaBasePaths() {

    deva_base_paths;
    std::vector<Vector2f> mines;

    for (std::size_t i = 0; i < base_safe_1.size(); i++) {

        Vector2f safe_1(base_safe_1[i].x, base_safe_1[i].y);
        Vector2f safe_2(base_safe_2[i].x, base_safe_2[i].y);

        float radius = game_->GetSettings().ShipSettings[2].GetRadius();

        Path base_path = GetPathfinder().FindPath(game_->GetMap(), mines, safe_1, safe_2, radius);
        base_path = GetPathfinder().SmoothPath(base_path, game_->GetMap(), radius);

        deva_base_paths.push_back(base_path);
    }
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
    //int bounces = 9;

    Vector2f direction = desired_direction;
    Vector2f position = game_->GetPosition();
    Vector2f center = GetWindowCenter();
    
    float speed_adjust = (game_->GetPlayer().velocity * game_->GetPlayer().GetHeading());


    //float travel = ((proj_speed + speed_adjust) / 100.0f) * alive_time;
    float travel = (proj_speed + speed_adjust) * (alive_time / 100.0f);
    float max_travel = travel; 

    //std::string text = std::to_string(speed_adjust);
    //RenderText(text.c_str(), GetWindowCenter() - Vector2f(0, 160), RGB(100, 100, 100), RenderText_Centered);

    

    //RenderLine(center, center + (direction * bullet_travel_ * 15), RGB(100, 100, 100));

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
                        std::string text = "Box Intersected";
                        RenderText(text.c_str(), GetWindowCenter() - Vector2f(0, 160), RGB(100, 100, 100), RenderText_Centered);

                        CastResult target_line = GunCast(game_->GetMap(), position, seek_direction, to_seek.Length());

                        if (!target_line.hit) {

                            std::string text = "SHOOT";
                            RenderText(text.c_str(), GetWindowCenter() - Vector2f(0, 140), RGB(100, 100, 100), RenderText_Centered);
                            RenderLine(center, center + (direction * to_target.Length() * 16), RGB(100, 100, 100));
                            RenderLine(center, center + (seek_direction * to_seek.Length() * 16), RGB(120, 0, 120));
                            return true;
                        }
                    }
                }
            }
            
            CastResult wall_line = GunCast(game_->GetMap(), position, direction, travel);

            if (i == 0) { 
                wall_shot = wall_line.position;
                //RenderLine(center, center + (seek_direction * to_seek.Length() * 16), RGB(100, 100, 100)); 
            }

            if (wall_line.hit) {
                RenderLine(center, center + (direction * wall_line.distance * 16), RGB(100, 100, 100));

                center = center + (direction * wall_line.distance * 16);

                direction = direction - (wall_line.normal * (2.0f * direction.Dot(wall_line.normal)));

                std::string text = std::to_string(wall_line.normal.x) + ", " + std::to_string(wall_line.normal.y);
                RenderText(text.c_str(), GetWindowCenter() - Vector2f(0, 120), RGB(100, 100, 100), RenderText_Centered);

                position = wall_line.position;

                travel -= wall_line.distance;
            }
            else {
                RenderLine(center, center + (direction * travel * 16), RGB(100, 100, 100));
                return false;
            }
        }
        return false;
}

}  // namespace marvin
