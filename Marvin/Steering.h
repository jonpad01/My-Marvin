#pragma once

#include <optional>
#include "GameProxy.h"

namespace marvin {

struct Player;
class Bot;
class Vector2f;

class SteeringBehavior {
 public:

  void Reset();

  void Seek(Bot& bot, Vector2f target, float multiplier = 1.0f);
  void Flee(Bot& bot, Vector2f target);

  void Arrive(Bot& bot, Vector2f target, Vector2f target_velocity = { 0.0f, 0.0f });

  void AnchorSpeed(Bot& bot, Vector2f target);
  void Stop(Bot& bot, Vector2f target);
  void Pursue(Bot& bot, const Player& enemy);
  void Face(Bot& bot, Vector2f target);

  void Steer(Bot& bot, float dt, SteeringOverride override);   

  void AvoidWalls(Bot& bot, float max_look_ahead);

 private:

  Vector2f velocityDelta_;
  std::optional<float> rotation_;
};

}  // namespace marvin
