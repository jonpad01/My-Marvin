#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

#include <cstdint>
#include <string>

#include "Vector2f.h"

namespace marvin {

struct BallData {
    Vector2f position, velocity, last_activity;
    uint32_t inactive_timer, last_holder_id;
    bool held;
};

struct Player {
  std::string name;
  std::string squad;
  std::string wins;
  bool dead, active;

  int32_t energy;
  uint16_t id;
  uint16_t ship;
  int32_t bounty;

  Vector2f position;
  Vector2f velocity;

  uint16_t frequency;
  uint16_t discrete_rotation;

  uint8_t status;

  Vector2f GetHeading() const {
    const float kToRads = (static_cast<float>(M_PI) / 180.0f);
    float rads = (((40 - (discrete_rotation + 30)) % 40) * 9.0f) * kToRads;
    float x = std::cos(rads);
    float y = -std::sin(rads);

    return Vector2f(x, y);
  }
};

}  // namespace marvin
