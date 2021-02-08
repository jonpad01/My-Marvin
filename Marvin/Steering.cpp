#include "Steering.h"

#include <algorithm>
#include <cmath>

//#include "Bot.h"
//#include "Devastation.h"
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


SteeringBehavior::SteeringBehavior(GameProxy& game) : game_(game), rotation_(0.0f) {}

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

//void SteeringBehavior::AvoidWalls() { auto& game = bot_->GetGame(); }
//void SteeringBehavior::AvoidWalls() { auto& game = bot_->GetGame(); }

}  // namespace marvin
