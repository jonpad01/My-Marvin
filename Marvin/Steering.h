#pragma once

#include "GameProxy.h"

namespace marvin {

struct Player;
class Bot;
class Vector2f;

class SteeringBehavior {
 public:
  SteeringBehavior();

  void Reset();

  void Seek(Bot& bot, Vector2f target, float multiplier = 1.0f);
  void Flee(Bot& bot, Vector2f target);
  void Arrive(Bot& bot, Vector2f target, float deceleration);
  void AnchorSpeed(Bot& bot, Vector2f target);
  void Stop(Bot& bot, Vector2f target);
  void Pursue(Bot& bot, const Player& enemy);
  void Face(Bot& bot, Vector2f target);

  void Steer(Bot& bot, bool backwards);

  void AvoidWalls(Bot& bot, float max_look_ahead);

 private:

  Vector2f force_;
  float rotation_;
};

}  // namespace marvin
