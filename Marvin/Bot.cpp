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

namespace marvin {


    Bot::Bot(std::unique_ptr<marvin::GameProxy> game) : game_(std::move(game)), keys_(time_.GetTime()), steering_(*game_, keys_) {

        auto processor = std::make_unique<path::NodeProcessor>(*game_);

        last_ship_change_ = 0;
     
        ship_ = game_->GetPlayer().ship;


        if (ship_ > 7) {
            ship_ = 0;
        }

        pathfinder_ = std::make_unique<path::Pathfinder>(std::move(processor));
        regions_ = RegionRegistry::Create(game_->GetMap());
        pathfinder_->CreateMapWeights(game_->GetMap());

        //needs behavior tree

        behavior_ = std::make_unique<behavior::BehaviorEngine>(behavior_nodes_.back().get());
        ctx_.bot = this;
    }

//the bot update is the first step in the flow, called from main.cpp, from here it calls the behavior tree
    void Bot::Update(float dt) {
        keys_.ReleaseAll();
        game_->Update(dt);

        uint64_t timestamp = time_.GetTime();
        uint64_t ship_cooldown = 10000;

        //check chat for disconected message or spec message in eg and terminate continuum
        std::string name = game_->GetName();
        std::string disconnected = "WARNING: ";


        Chat chat = game_->GetChat();

        if (chat.type == 0) {
            if (chat.message.compare(0, 9, disconnected) == 0) {
                exit(5);
            }
        }


        //then check if specced for lag
        if (game_->GetPlayer().ship > 7) {
            uint64_t timestamp = time_.GetTime();
            if (timestamp - last_ship_change_ > ship_cooldown) {
                if (game_->SetShip(ship_)) {
                    last_ship_change_ = timestamp;
                }
            }
            return;
        }

        steering_.Reset();


        ctx_.dt = dt;

        behavior_->Update(ctx_);

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

            if (behind) { keys_.Press(VK_DOWN); }
            else { keys_.Press(VK_UP); }

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

        Vector2f target_position = ctx_.blackboard.ValueOr("target_position", Vector2f());
        float dot = heading.Dot(Normalize(target_position - game_->GetPosition()));
  
  

        //seek makes the bot hunt the target hard
        //arrive makes the bot slow down as it gets closer
        //this works with steering.Face in MoveToEnemyNode
        //there is a small gap where the bot wont do anything at all
        if (distance > target_distance) {
           
            steering_.Seek(target);
        }

        else if (distance <= target_distance) {

            Vector2f to_target = target - bot_player.position;

            steering_.Seek(target - Normalize(to_target) * target_distance);
        }
    }
}  // namespace marvin
