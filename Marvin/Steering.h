#pragma once

#include "Vector2f.h"
#include "GameProxy.h"

namespace marvin {

struct Player;

class SteeringBehavior {
 public:
  SteeringBehavior(GameProxy& game, KeyController& keys);

  Vector2f GetSteering();
  float GetRotation();

  void Reset();

  void Seek(Vector2f target, float multiplier = 1.0f);
  void Flee(Vector2f target);
  void Arrive(Vector2f target, float deceleration);
  void AnchorSpeed(Vector2f target);
  void Stop(Vector2f target);
  void Pursue(const Player& enemy);
  void Face(Vector2f target);

  void Steer();

  //void AvoidWalls();

 private:

  GameProxy& game_;
  KeyController& keys_;

  Vector2f force_;
  float rotation_; 
};

}  // namespace marvin
