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


namespace marvin {
   
#if 0
    bool WallShot(behavior::ExecuteContext& ctx, const marvin::Player& bot_player) {
        int bounces = 2;
        Vector2f position = bot_player.position;
        Vector2f initial_heading = bot_player.GetHeading() * (ctx.bot->GetGame().GetShipSettings().BombSpeed / 16.0f / 10.0f);
        Vector2f direction = Normalize(initial_heading + bot_player.velocity);
        for (int i = 0; i < bounces + 1; ++i) {
            CastResult result = RayCast(ctx.bot->GetGame().GetMap(), position, direction, 1000.0f);

            if (result.hit) {
                //RenderWorldLine(player, position, result.position);
                //RenderRadarLine(player, position, result.position, game_->GetSettings().MapZoomFactor);

                direction = direction - (result.normal * (2.0f * direction.Dot(result.normal)));

                position = result.position;
            }
            else {
                //RenderWorldLine(player, position, position + direction * result.distance);
                break;
            }
        }
    }
#endif
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

    bool CanShoot(const marvin::Map& map, const marvin::Player& bot_player,
        const marvin::Player& target) {
        if (bot_player.position.DistanceSq(target.position) > 60 * 60) return false;
        if (map.GetTileId(bot_player.position) == marvin::kSafeTileId) return false;

        return true;
    }

    marvin::Vector2f CalculateShot(const marvin::Vector2f& pShooter,
        const marvin::Vector2f& pTarget,
        const marvin::Vector2f& vShooter,
        const marvin::Vector2f& vTarget,
        float sProjectile) {
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
        Path path = ctx.bot->GetPathfinder().FindPath(mines, from, to, game.GetSettings().ShipSettings[6].GetRadius());
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
            path = ctx.bot->GetPathfinder().FindPath(mines, from, to, radius);
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

void Bot::ExtremeGames() {

    auto freq_warp_attach = std::make_unique<eg::FreqWarpAttachNode>();
    auto find_enemy = std::make_unique<eg::FindEnemyNode>();
    auto looking_at_enemy = std::make_unique<eg::LookingAtEnemyNode>();
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

    auto shoot_enemy = std::make_unique<eg::ShootEnemyNode>();
    auto path_to_enemy = std::make_unique<eg::PathToEnemyNode>();
    auto move_to_enemy = std::make_unique<eg::MoveToEnemyNode>();
    auto follow_path = std::make_unique<eg::FollowPathNode>();
    auto patrol = std::make_unique<eg::PatrolNode>();
 

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

Bot::Bot(std::unique_ptr<GameProxy> game) : game_(std::move(game)), steering_(this) {
  auto processor = std::make_unique<path::NodeProcessor>(*game_);
  
  last_ship_change_ = 0;
  last_base_change_ = 0;
  ship_ = game_->GetPlayer().ship;
  

  if (ship_ > 7) {
    ship_ = 0;
  }
  
  pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor));

  if (game_->Zone() == "hyperspace") {
      Hyperspace();
      behavior_ctx_.blackboard.Set("patrol_nodes", std::vector<Vector2f>({ Vector2f(585, 540), Vector2f(400, 570) }));
      behavior_ctx_.blackboard.Set("patrol_index", 0);
  }
  else if (game_->Zone() == "devastation") Devastation();
  else if (game_->Zone() == "extreme games") ExtremeGames();

  behavior_ = std::make_unique<behavior::BehaviorEngine>(behavior_nodes_.back().get());

  regions_ = RegionRegistry::Create(game_->GetMap());

}
//the bot update is the first step in the flow, called from main.cpp, from here it calls the behavior tree
void Bot::Update(float dt) {
    keys_.ReleaseAll();
    game_->Update(dt);
    bool in_hs = game_->Zone() == "hyperspace";
    bool in_deva = game_->Zone() == "devastation";
    bool in_eg = game_->Zone() == "extreme games";
    uint64_t timestamp = GetTime();
    uint64_t ship_cooldown = 10000;
    if (in_deva) ship_cooldown = 10800000;

    if (game_->GetPlayer().ship > 7) {
        uint64_t timestamp = GetTime();
        if (timestamp - last_ship_change_ > ship_cooldown) {
            if (game_->SetShip(ship_)) {
                last_ship_change_ = timestamp;
            }
        }
        return;
    }

  steering_.Reset();

  behavior_ctx_.bot = this;
  behavior_ctx_.dt = dt;

  behavior_->Update(behavior_ctx_);

  Steer();
}

void Bot::Steer() {
    Vector2f center = GetWindowCenter();
    float debug_y = 100;
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
          RenderText("adjusting", center - Vector2f(0, debug_y), RGB(100, 100, 100),
              RenderText_Centered);
          debug_y -= 20;
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
          float force_percent =
              force.Length() /
              (GetGame().GetShipSettings().MaximumSpeed / 16.0f / 16.0f);
          RenderLine(center, center + (force_direction * 100 * force_percent),
              RGB(255, 255, 0));
      }

      RenderLine(center, center + (heading * 100), RGB(255, 0, 0));
      RenderLine(center, center + (perp * 100), RGB(100, 0, 100));
      RenderLine(center, center + (rotate_target * 85), RGB(0, 0, 255));
      RenderLine(center, center + (steering_direction * 75), RGB(0, 255, 0));
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
  const Player& enemy = flaggers.enemy_anchor;
  Area enemy_is = InArea(enemy.position);
  Area bot_is = InArea(bot_player.position);
  //seek makes the bot hunt the target hard
  //arrive makes the bot slow down as it gets closer
  //this works with steering.Face in MoveToEnemyNode
  //there is a small gap where the bot wont do anything at all
  if (distance > target_distance) {
      if (bot_is.in_base && enemy_is.connected && (anchor || game_->GetPlayer().ship == 2)) {
              steering_.AnchorSpeed(target); 
      }
      else steering_.Seek(target);
  }
  
  else if (distance <= target_distance) {
      
      Vector2f to_target = target - bot_player.position;

      steering_.Seek(target - Normalize(to_target) * target_distance);
  }
}

uint64_t Bot::GetTime() const {
    return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now())
        .time_since_epoch()
      .count();
}

Area Bot::InArea(Vector2f position) {
    Area result = { 0 };
    Vector2f position_ = game_->GetPlayer().position;

    result.in_center = regions_->IsConnected((MapCoord)position, MapCoord(512, 512));
    result.in_tunnel = regions_->IsConnected((MapCoord)position, MapCoord(27, 354));
    result.in_1 = regions_->IsConnected((MapCoord)position, MapCoord(854, 358));
    result.in_2 = regions_->IsConnected((MapCoord)position, MapCoord(823, 488));
    result.in_3 = regions_->IsConnected((MapCoord)position, MapCoord(696, 817));
    result.in_4 = regions_->IsConnected((MapCoord)position, MapCoord(587, 776));
    result.in_5 = regions_->IsConnected((MapCoord)position, MapCoord(125, 877));
    result.in_6 = regions_->IsConnected((MapCoord)position, MapCoord(254, 493));
    result.in_7 = regions_->IsConnected((MapCoord)position, MapCoord(154, 358));
    result.in_8 = regions_->IsConnected((MapCoord)position, MapCoord(620, 250));
    result.in_base = result.in_1 || result.in_2 || result.in_3 || result.in_4 || result.in_5 || result.in_6 || result.in_7;
    result.connected = regions_->IsConnected((MapCoord)position, (MapCoord)position_);
    //should be used on enemy targets
    result.in_diff = !regions_->IsConnected((MapCoord)position, (MapCoord)position_) && result.in_base;
    //stores first 7 results in a vector
    result.in = { result.in_1, result.in_2, result.in_3, result.in_4, result.in_5, result.in_6, result.in_7 };

    return result;
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
    std::vector < Player > players = game_->GetPlayers();
    const Player& bot = game_->GetPlayer();
    Vector2f position = game_->GetPlayer().position;
    std::vector < Player > enemy_anchors, team_anchors;
    Flaggers result;
    result.flaggers = 0; result.team_lancs = 0; result.team_spiders = 0;
    
    //grab all flaggers in lancs and sort by team then store in a vector
    for (std::size_t i = 0; i < players.size(); ++i) {
       const Player& player = players[i];
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
                if (enemy_team) enemy_anchors.push_back(players[i]);
                if (same_team) team_anchors.push_back(players[i]);
            }
        }
    }
   
    //loops through lancs and rejects any not in base, unless it reaches the last one
    for (std::size_t i = 0; i < enemy_anchors.size(); i++) {
        const Player& player = enemy_anchors[i];
        Area player_is = InArea(player.position);
        if (!player_is.in_base && i != enemy_anchors.size() - 1) {
            continue;
        }
        else {
            result.enemy_anchor = enemy_anchors[i];
        }
    }
    for (std::size_t i = 0; i < team_anchors.size(); i++) {
        const Player& player = team_anchors[i];
        Area player_is = InArea(player.position);
        if (!player_is.in_base && i != team_anchors.size() - 1) {
            continue;
        }
        else {
            result.team_anchor = team_anchors[i];
        }
    }
    return result;
}
Duelers Bot::FindDuelers() {
    std::vector < Player > players = game_->GetPlayers();
    const Player& bot = game_->GetPlayer();
    
    Duelers result = { 0 };

    //find team sizes and push team mates into a vector
    for (std::size_t i = 0; i < players.size(); i++) {
        const Player& player = players[i];
        Area player_is = InArea(player.position);
        if (player.frequency == 00) result.freq_0s++;
        if (player.frequency == 01) result.freq_1s++;
        bool dueling = player.frequency == 00 || player.frequency == 01;
        bool not_bot = player.id != bot.id;
        if (dueling && not_bot) {
            if (player.frequency == bot.frequency) {
                if (!player_is.in_center) result.team_in_base = 1;
                result.team.push_back(players[i]);
            }
            else if (player.frequency != bot.frequency) {
                if (!player_is.in_center) result.enemy_in_base = 1;
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
int Bot::FindOpenFreq() {
    std::vector < Player > players = game_->GetPlayers();
    std::vector < int > freqs(100, 0);
    int open_freq = 0;
    for (std::size_t i = 0; i < players.size(); i++) {
        const Player& player = players[i];
        int freq = player.frequency;
        if (freq > 1 && freq < 100) {
            freqs[freq]++;
        }
    }
    for (std::size_t i = 2; i < freqs.size(); i++) {
        if (freqs[i] == 0) {
            open_freq = i;
            break;
        }
    }
    return open_freq;
}

}  // namespace marvin
