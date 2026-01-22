#include "Steering.h"

#include <algorithm>
#include <cmath>

#include "Bot.h"
#include "Debug.h"
#include "Player.h"
#include "RayCaster.h"
#include "platform/platform.h"
#include "Vector2f.h"

namespace marvin {

  static float SignedAngle(const Vector2f& from, const Vector2f& to) {
    float dot = from.Dot(to);
    float cross = from.x * to.y - from.y * to.x;
    return std::atan2(cross, dot); // radians (-pi .. +pi)
  }

float GetCurrentSpeed(GameProxy& game) {
  return game.GetPlayer().velocity.Length();
}

void SteeringBehavior::Reset() {
  velocityDelta_ = Vector2f();
  rotation_.reset();
}

void SteeringBehavior::Seek(Bot& bot, Vector2f target, float multiplier) {
  auto& game = bot.GetGame();
  float speed = game.GetMaxSpeed();

  Vector2f desired = Normalize(target - game.GetPosition()) * speed * multiplier;

  velocityDelta_ += desired - game.GetPlayer().velocity;
}

void SteeringBehavior::Flee(Bot& bot, Vector2f target) {
  auto& game = bot.GetGame();
  float speed = game.GetMaxSpeed();

  Vector2f desired = Normalize(game.GetPosition() - target) * speed;

  velocityDelta_ += desired - game.GetPlayer().velocity;
}

void SteeringBehavior::Arrive(Bot& bot, Vector2f target, Vector2f target_velocity) {
  auto& game = bot.GetGame();

 // float turn_rate_rad = (game.GetRotation() / 400.0f) * 2.0f * M_PI;
  Vector2f vel = game.GetPlayer().velocity;
  float thrust = game.GetThrust();
  float max_speed = game.GetMaxSpeed() + 1.0f; // fix for jitter when hitting top speed


 // Vector2f vel_dir = Normalize(vel);
 // Vector2f brake_dir = -vel_dir;

  //float angle_to_brake = std::abs(SignedAngle(brake_dir, game.GetPlayer().GetHeading()));
 // float time_to_rotate = angle_to_brake / turn_rate_rad;
 // float distance_during_turn = vel.Length() * time_to_rotate;



  Vector2f to_target = target - game.GetPosition();
  float distance = to_target.Length();
  Vector2f relative_vel = vel - target_velocity;

 // float closing_speed = relative_vel.Dot(Normalize(to_target));
 // float braking_distance = (closing_speed * closing_speed) / (2.0f * thrust);
  //float total_braking_distance = braking_distance + distance_during_turn;

  //float effective_distance = std::max(0.0f, distance - distance_during_turn);

  //float desired_closing_speed = std::sqrt(2.0f * thrust * effective_distance);


  float desired_closing_speed = (std::sqrt(std::max(0.0f, 2.0f * thrust * distance)));
  desired_closing_speed = std::min(desired_closing_speed, max_speed);

  Vector2f desired_relative_vel = Normalize(to_target) * desired_closing_speed;
  Vector2f desired_world_vel = desired_relative_vel + target_velocity;

  velocityDelta_ += desired_world_vel - vel;

  //Vector2f accel = desired_world_vel - vel;

 // float accel_len = accel.Length();
 // float max_accel = thrust; 

 // if (accel_len > max_accel)
  //  accel = accel * (max_accel / accel_len);

  //force_ += accel;
  RenderWorldBox(game.GetPosition(), target, 0.875f);
}

void SteeringBehavior::AnchorSpeed(Bot& bot, Vector2f target) {
  auto& game = bot.GetGame();
  float speed = game.GetMaxSpeed();

  Vector2f desired = Normalize(target - game.GetPosition()) * 5;

  velocityDelta_ += desired - game.GetPlayer().velocity;
}

void SteeringBehavior::Stop(Bot& bot, Vector2f target) {
  auto& game = bot.GetGame();
  float speed = game.GetMaxSpeed();

  Vector2f desired = Normalize(target - game.GetPosition()) * 5;

  velocityDelta_ += desired - game.GetPlayer().velocity;
}

void SteeringBehavior::Pursue(Bot& bot, const Player& enemy) {
  auto& game = bot.GetGame();
  const Player& player = game.GetPlayer();

  float max_speed = game.GetMaxSpeed();

  Vector2f to_enemy = enemy.position - game.GetPosition();
  float dot = player.GetHeading().Dot(enemy.GetHeading());

  if (to_enemy.Dot(player.GetHeading()) > 0 && dot < -0.95f) {
    Seek(bot, enemy.position);
  } else {
    float t = to_enemy.Length() / (max_speed + enemy.velocity.Length());

    Seek(bot, enemy.position + enemy.velocity * t);
  }
}

void SteeringBehavior::Face(Bot& bot, Vector2f target) {
  auto& game = bot.GetGame();
  Vector2f to_target = target - game.GetPosition();

  Vector2f heading = Rotate(game.GetPlayer().GetHeading(), rotation_.value_or(0.0f));

  float rotation = std::atan2(heading.y, heading.x) - std::atan2(to_target.y, to_target.x);

  rotation_.emplace(rotation_.value_or(0.0f) + WrapToPi(rotation));
}


#if 0 

void SteeringBehavior::Steer(Bot& bot, bool backwards) {
  auto& game = bot.GetGame();
  auto& bb = bot.GetBlackboard();
  auto& keys = bot.GetKeys();
  // bool afterburners = GetAfterburnerState();
  
  float free_play = 80.0f / 81.0f;

  CombatDifficulty difficulty = bb.ValueOr<CombatDifficulty>(BBKey::CombatDifficulty, CombatDifficulty::Normal);

  if (difficulty == CombatDifficulty::Nerf) {
    free_play = 0.97f;
  }

  Vector2f heading = game.GetPlayer().GetHeading();
  if (backwards) {
    heading = Reverse(heading);
  }

  // Start out by trying to move in the direction that the bot is facing.
  Vector2f steering_direction = heading;

  bool has_force = force_.LengthSq() > 0.0f;

  // If the steering system calculated any movement force, then set the movement
  // direction to it.
  if (has_force) {
    steering_direction = marvin::Normalize(force_);
  }

  // Rotate toward the movement direction.
  Vector2f rotate_target = steering_direction;

  // If the steering system calculated any rotation then rotate from the heading
  // to desired orientation.
  if (rotation_ != 0.0f) {
    //rotate_target = Rotate(heading, -rotation_);  // changed
    Vector2f rotDir = Rotate(heading, -rotation_);
    rotate_target = Normalize(rotDir + steering_direction);
  }

  if (!has_force) {
    steering_direction = rotate_target;
  }
  Vector2f perp = marvin::Perpendicular(heading);
  //bool behind = force_.Dot(heading) < 0;  // changed
  bool behind = steering_direction.Dot(heading) < 0;
  // This is whether or not the steering direction is pointing to the left of
  // the ship.
  bool leftside = steering_direction.Dot(perp) < 0;

#if 0 
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
#endif

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
      keys.Press(VK_DOWN);
    } else {
      keys.Press(VK_UP);
    }
  }

  // above 0.996 and bot is constantly correcting creating a wobble
  if (heading.Dot(steering_direction) < free_play) {  // 1.0f

    // ensure that only one arrow key is pressed at any given time
    keys.Set(VK_RIGHT, clockwise);
    keys.Set(VK_LEFT, !clockwise);
  }

#if DEBUG_RENDER_STEERING

    Vector2f center = GetWindowCenter();

    if (has_force) {
        Vector2f force_direction = Normalize(force_);
        float force_percent = force_.Length() / 22.0f;
        RenderLine(center, center + (force_direction * 100 * force_percent), RGB(255, 255, 0)); //yellow
    }
    
   // RenderLine(center, center + (heading * 100), RGB(255, 0, 0)); //red
  //  RenderLine(center, center + (perp * 100), RGB(100, 0, 100));  //purple
    RenderLine(center, center + (rotate_target * 85), RGB(0, 0, 255)); //blue
    RenderLine(center, center + (steering_direction * 75), RGB(0, 255, 0)); //green
#endif
}

#endif




void SteeringBehavior::Steer(Bot& bot, float dt, SteeringOverride override) {
  auto& game = bot.GetGame();
  auto& bb = bot.GetBlackboard();
  auto& keys = bot.GetKeys();

  constexpr float DEG_TO_RAD = (float)M_PI / 180.0f;
  constexpr float HALF_PI = (float)M_PI / 2.0f;

  // continuum has 40 possible rotation headings
  // each possible rotation point is about 9 degress
  float deadzone = DEG_TO_RAD * (360.0f / 40.0f) * 0.5f;

  float rotation_deg_per_sec = game.GetRotation() * 0.9f;
  float rotation_rad_per_sec = rotation_deg_per_sec * DEG_TO_RAD;
  float max_turn_this_frame = rotation_rad_per_sec * dt;

  Vector2f heading = game.GetPlayer().GetHeading();
  float thrust = game.GetThrust();
  float max_speed = game.GetMaxSpeed();

  bool has_force = velocityDelta_.LengthSq() > 0.0f;

  // Desired movement direction  
  Vector2f desired_dir = has_force ? Normalize(velocityDelta_) : heading;
  Vector2f desired_heading = desired_dir;


  // ---- SIGNED ANGLE CONTROLLER ----

  // signed angle expects y coord to be reversed and im too lazy to learn how it works
  float angle = -SignedAngle(heading, desired_heading);
  float abs_angle = std::abs(angle);

  // if desired heading is behind the ship just fly in reverse rather than waste time rotating
  bool invert = (abs_angle > HALF_PI && override == SteeringOverride::None) || override == SteeringOverride::Reverse;

  // if the delta is low then the ship should have time to return to forward facing
  // delta only gets that low when the ship is hitting top speed or stopping on a dime
  if (invert && velocityDelta_.Length() > 1.5f) {
    angle = -angle;
    abs_angle = (float)M_PI - abs_angle;
  }

  // the bot was told to face something
  if (rotation_.has_value()) {
    deadzone *= 0.5f;
    angle = rotation_.value();
    abs_angle = std::abs(angle);
  }
  else if (!has_force) {
    return; // nothing to do
  }

  // ---- ROTATION ----

  if (abs_angle > deadzone) {
    if (angle > 0.0f) {
      keys.Set(VK_LEFT, true);
      keys.Set(VK_RIGHT, false);
    }
    else {
      keys.Set(VK_RIGHT, true);
      keys.Set(VK_LEFT, false);
    }
  }

  // ---- THRUST ----
  if (!has_force) return;

  bool can_face_target = abs_angle < (max_turn_this_frame * 2.0f) || abs_angle <= deadzone;

  // the force is perpendicular-ish to the heading
  // this can probably be based on the ships max turn as well instead of arbitrary radians
  bool force_perp = abs_angle > 1.0f && abs_angle < 2.14f;

  // can the ship correct its momentum?
  angle = -SignedAngle(heading, Normalize(game.GetPlayer().velocity));
  abs_angle = std::abs(angle);

  bool velocity_perp = abs_angle > 1.25f && abs_angle < 1.89f && game.GetPlayer().velocity.LengthSq() > 1.0f;

  if (velocity_perp) {
    g_RenderState.RenderDebugText("     PERP");
  }

  // ship needs more time to rotate
  if (!can_face_target) {
    return;
    //if (!force_perp || velocity_perp) return;
    //desired_dir = -Normalize(game.GetPlayer().velocity);
  }

  //if ((!can_face_target && !force_perp) || velocity_perp) return;

  // if the desired heading is nearly perp, try to kill the ships momentum 
  if (force_perp) {
    //desired_dir = -Normalize(game.GetPlayer().velocity);
  }

  bool behind = heading.Dot(desired_dir) < 0.0f;

  if (behind) {
    keys.Press(VK_DOWN);
  }
  else {
    keys.Press(VK_UP);
  }


#if DEBUG_RENDER_STEERING
  Vector2f center = GetWindowCenter();
  RenderLine(center, center + heading * 80, RGB(255, 0, 0)); // heading (red)
  RenderLine(center, center + desired_dir * 80, RGB(0, 255, 0)); // desired (green)
  RenderLine(center, center + force_ * 3.0f, RGB(255, 255, 0)); //yellow
  g_RenderState.RenderDebugText("     abs_angle: %07.4f", abs_angle);
#endif
}




void SteeringBehavior::AvoidWalls(Bot& bot, float max_look_ahead) {
  auto& game = bot.GetGame();
  constexpr float kDegToRad = 3.14159f / 180.0f;
  constexpr size_t kFeelerCount = 29;

  static_assert(kFeelerCount & 1, "Feeler count must be odd");

  Vector2f feelers[kFeelerCount];

  feelers[0] = Normalize(game.GetPlayer().velocity);

  for (size_t i = 1; i < kFeelerCount; i += 2) {
    feelers[i] = Rotate(feelers[0], kDegToRad * (90.0f / kFeelerCount) * i);
    feelers[i + 1] = Rotate(feelers[0], -kDegToRad * (90.0f / kFeelerCount) * i);
  }

  float speed = game.GetPlayer().velocity.Length();
  float max_speed = game.GetMaxSpeed();
  float look_ahead = max_look_ahead * (speed / max_speed);

  size_t force_count = 0;
  Vector2f force;

  for (size_t i = 0; i < kFeelerCount; ++i) {
    float intensity = feelers[i].Dot(Normalize(game.GetPlayer().velocity));
    float check_distance = look_ahead * intensity;
    Vector2f check = feelers[i] * intensity;
    CastResult result = RayCast(bot, RayBarrier::Solid, game.GetPlayer().position, Normalize(feelers[i]), check_distance);
    COLORREF color = RGB(100, 0, 0);

    if (result.hit) {
      float multiplier = ((check_distance - result.distance) / check_distance) * 1.5f;

      force += Normalize(feelers[i]) * -Normalize(feelers[i]).Dot(feelers[0]) * multiplier * max_speed;

      ++force_count;
    } else {
      result.distance = check_distance;
      color = RGB(0, 100, 0);
    }
#if DEBUG_RENDER_STEERING
        RenderWorldLine(game.GetPosition(), game.GetPosition(),
            game.GetPosition() + Normalize(feelers[i]) * result.distance, color);
#endif
  }

  if (force_count > 0) {
    velocityDelta_ += force * (1.0f / force_count);
  }
}

}  // namespace marvin
