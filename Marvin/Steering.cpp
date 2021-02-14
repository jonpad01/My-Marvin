#include "Steering.h"

#include "platform/platform.h"

#include <algorithm>
#include <cmath>

#include "Player.h"


namespace marvin {

    float GetCurrentSpeed(GameProxy& game) {
        return game.GetPlayer().velocity.Length();
    }

    // Context sensitive max speed to handle wormhole increases
    float GetMaxSpeed(GameProxy& game) {
        float speed = game.GetShipSettings().InitialSpeed / 10.0f / 16.0f;

        if (GetCurrentSpeed(game) > speed) {
            speed = std::abs(speed + game.GetShipSettings().GravityTopSpeed);
        }

        return speed;
    }


SteeringBehavior::SteeringBehavior(GameProxy& game, KeyController& keys) : game_(game), keys_(keys), rotation_(0.0f) {}

Vector2f SteeringBehavior::GetSteering() { return force_; }

float SteeringBehavior::GetRotation() { return rotation_; }

void SteeringBehavior::Reset() {
  force_ = Vector2f();
  rotation_ = 0.0f;
}

void SteeringBehavior::Seek(Vector2f target, float multiplier) {
  float speed = GetMaxSpeed(game_);
 
  Vector2f desired = Normalize(target - game_.GetPosition()) * speed * multiplier;

  force_ += desired - game_.GetPlayer().velocity;
}

void SteeringBehavior::Flee(Vector2f target) {

  float speed = GetMaxSpeed(game_);

  Vector2f desired = Normalize(game_.GetPosition() - target) * speed;

  force_ += desired - game_.GetPlayer().velocity;
}

void SteeringBehavior::Arrive(Vector2f target, float deceleration) {

    float max_speed = GetMaxSpeed(game_);

  Vector2f to_target = target - game_.GetPosition();
  float distance = to_target.Length();

  if (distance > 0) {
    float speed = distance / deceleration;

    speed = std::min(speed, max_speed);

    Vector2f desired = to_target * (speed / distance);

    force_ += desired - game_.GetPlayer().velocity;
  }
}
void SteeringBehavior::AnchorSpeed(Vector2f target) {

    float speed = GetMaxSpeed(game_);

    Vector2f desired = Normalize(target - game_.GetPosition()) * 5;

    force_ += desired - game_.GetPlayer().velocity;
}

void SteeringBehavior::Stop(Vector2f target) {

    float speed = GetMaxSpeed(game_);

    Vector2f desired = Normalize(target - game_.GetPosition()) * 5;

    force_ += desired - game_.GetPlayer().velocity;
}

void SteeringBehavior::Pursue(const Player& enemy) {

  const Player& player = game_.GetPlayer();

  float max_speed = GetMaxSpeed(game_);

  Vector2f to_enemy = enemy.position - game_.GetPosition();
  float dot = player.GetHeading().Dot(enemy.GetHeading());

  if (to_enemy.Dot(player.GetHeading()) > 0 && dot < -0.95f) {
    Seek(enemy.position);
  } else {
    float t = to_enemy.Length() / (max_speed + enemy.velocity.Length());

    Seek(enemy.position + enemy.velocity * t);
  }
}

void SteeringBehavior::Face(Vector2f target) {

  Vector2f to_target = target - game_.GetPosition();

  Vector2f heading = Rotate(game_.GetPlayer().GetHeading(), rotation_);
  float rotation =
      std::atan2(heading.y, heading.x) - std::atan2(to_target.y, to_target.x);

  rotation_ += WrapToPi(rotation);
}


void SteeringBehavior::Steer() {

    Vector2f force = GetSteering();
    float rotation = GetRotation();
    //bool afterburners = GetAfterburnerState();

    Vector2f heading = game_.GetPlayer().GetHeading();
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
        if (behind) { keys_.Press(VK_DOWN); }
        else { keys_.Press(VK_UP); }
    }

    //above 0.996 and bot is constantly correcting creating a wobble
    if (heading.Dot(steering_direction) < 0.996f) {  //1.0f

        //ensure that only one arrow key is pressed at any given time
        keys_.Set(VK_RIGHT, clockwise);
        keys_.Set(VK_LEFT, !clockwise);
    }
}

//void SteeringBehavior::AvoidWalls() { auto& game = bot_->GetGame(); }
//void SteeringBehavior::AvoidWalls() { auto& game = bot_->GetGame(); }

}  // namespace marvin
