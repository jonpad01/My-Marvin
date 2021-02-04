#pragma once

#include "Vector2f.h"
#include "GameProxy.h"

namespace marvin {

class Bot;
struct Player;
//class GameProxy;

class SteeringBehavior {
 public:
  SteeringBehavior(std::shared_ptr<GameProxy> game);

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
  //void AvoidWalls();

 private:
 // Bot* bot_;
  std::shared_ptr<GameProxy> game_;
  Vector2f force_;
  float rotation_;
};

}  // namespace marvin
