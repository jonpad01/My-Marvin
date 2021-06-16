#include "Steering.h"

#include <algorithm>
#include <cmath>

#include "Debug.h"
#include "Player.h"
#include "RayCaster.h"
#include "platform/platform.h"

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

Vector2f SteeringBehavior::GetSteering() {
  return force_;
}

float SteeringBehavior::GetRotation() {
  return rotation_;
}

void SteeringBehavior::Reset() {
  force_ = Vector2f();
  rotation_ = 0.0f;
}

void SteeringBehavior::Seek(Vector2f target, float multiplier) {
  float speed = (game_.GetMaxSpeed() + 5.0f);

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

  float rotation = std::atan2(heading.y, heading.x) - std::atan2(to_target.y, to_target.x);

  rotation_ += WrapToPi(rotation);
}

void SteeringBehavior::Steer(bool backwards) {
  Vector2f force = GetSteering();
  float rotation = GetRotation();
  // bool afterburners = GetAfterburnerState();

  Vector2f heading = game_.GetPlayer().GetHeading();
  if (backwards) {
    heading = Reverse(heading);
  }

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

  if (backwards) {
    if (behind) {
      behind = false;
    } else {
      behind = true;
    }
  }

  if (has_force) {
    if (behind) {
      keys_.Press(VK_DOWN);
    } else {
      keys_.Press(VK_UP);
    }
  }

  // above 0.996 and bot is constantly correcting creating a wobble
  if (heading.Dot(steering_direction) < 0.996f) {  // 1.0f

    // ensure that only one arrow key is pressed at any given time
    keys_.Set(VK_RIGHT, clockwise);
    keys_.Set(VK_LEFT, !clockwise);
  }

#if 0

    Vector2f center = GetWindowCenter();

    if (has_force) {
        Vector2f force_direction = Normalize(force);
        float force_percent = force.Length() / 22.0f;
        RenderLine(center, center + (force_direction * 100 * force_percent), RGB(255, 255, 0)); //yellow
    }
    
    RenderLine(center, center + (heading * 100), RGB(255, 0, 0)); //red
    RenderLine(center, center + (perp * 100), RGB(100, 0, 100));  //purple
    RenderLine(center, center + (rotate_target * 85), RGB(0, 0, 255)); //blue
    RenderLine(center, center + (steering_direction * 75), RGB(0, 255, 0)); //green
#endif
}

void SteeringBehavior::AvoidWalls(float max_look_ahead) {
  constexpr float kDegToRad = 3.14159f / 180.0f;
  constexpr size_t kFeelerCount = 29;

  static_assert(kFeelerCount & 1, "Feeler count must be odd");

  Vector2f feelers[kFeelerCount];

  feelers[0] = Normalize(game_.GetPlayer().velocity);

  for (size_t i = 1; i < kFeelerCount; i += 2) {
    feelers[i] = Rotate(feelers[0], kDegToRad * (90.0f / kFeelerCount) * i);
    feelers[i + 1] = Rotate(feelers[0], -kDegToRad * (90.0f / kFeelerCount) * i);
  }

  float speed = game_.GetPlayer().velocity.Length();
  float max_speed = game_.GetMaxSpeed();
  float look_ahead = max_look_ahead * (speed / max_speed);

  size_t force_count = 0;
  Vector2f force;

  for (size_t i = 0; i < kFeelerCount; ++i) {
    float intensity = feelers[i].Dot(Normalize(game_.GetPlayer().velocity));
    float check_distance = look_ahead * intensity;
    Vector2f check = feelers[i] * intensity;
    CastResult result = RayCast(game_.GetMap(), game_.GetPlayer().position, Normalize(feelers[i]), check_distance);
    COLORREF color = RGB(100, 0, 0);

    if (result.hit) {
      float multiplier = ((check_distance - result.distance) / check_distance) * 1.5f;

      force += Normalize(feelers[i]) * -Normalize(feelers[i]).Dot(feelers[0]) * multiplier * max_speed;

      ++force_count;
    } else {
      result.distance = check_distance;
      color = RGB(0, 100, 0);
    }
#if 0
        RenderWorldLine(game_.GetPosition(), game_.GetPosition(),
            game_.GetPosition() + Normalize(feelers[i]) * result.distance, color);
#endif
  }

  if (force_count > 0) {
    force_ += force * (1.0f / force_count);
  }
}

}  // namespace marvin
