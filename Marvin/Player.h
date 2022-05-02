#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

#include <cstdint>
#include <string>

#include "Vector2f.h"

namespace marvin {



struct BallData {
  BallData() : position(0, 0), velocity(0, 0), last_activity(0, 0), inactive_timer(0), last_holder_id(0), held(false) {}
  Vector2f position, velocity, last_activity;
  uint32_t inactive_timer, last_holder_id;
  bool held;
};

struct Player {
  std::string name;
  std::string squad;
  std::string wins;
  bool dead, active;

  bool multifire_status;
  bool multifire_capable;

  int32_t energy;
  uint16_t id;
  uint16_t attach_id;
  uint16_t ship;
  int32_t bounty;
  int32_t flags;
  uint32_t emp_ticks;

  Vector2f position;
  Vector2f velocity;

  uint16_t frequency;
  uint16_t discrete_rotation;

  uint32_t repels;
  uint32_t bursts;
  uint32_t decoys;
  uint32_t thors;
  uint32_t bricks;
  uint32_t rockets;
  uint32_t portals;

  uint8_t status;

  Vector2f GetHeading() const {
    const float kToRads = (static_cast<float>(M_PI) / 180.0f);
    float rads = (((40 - (discrete_rotation + 30)) % 40) * 9.0f) * kToRads;
    float x = std::cos(rads);
    float y = -std::sin(rads);

    return Vector2f(x, y);
  }

  Vector2f MultiFireDirection(uint16_t angle, bool leftside) const {
    const float kToRads = (static_cast<float>(M_PI) / 180.0f);

    float rads = 0.0f;

    float headingrads = (((40 - (discrete_rotation + 30)) % 40) * 9.0f) * kToRads;
    float multirads = ((float)angle / 111.1111111f) * kToRads;

    if (leftside) {
      rads = headingrads + multirads;
    } else {
      rads = headingrads - multirads;
    }

    float x = std::cos(rads);
    float y = -std::sin(rads);

    return Vector2f(x, y);
  }

  Vector2f ConvertToHeading(uint16_t rotation) const {
    const float kToRads = (static_cast<float>(M_PI) / 180.0f);
    float rads = (((40 - (rotation + 30)) % 40) * 9.0f) * kToRads;
    float x = std::cos(rads);
    float y = -std::sin(rads);

    return Vector2f(x, y);
  }
  float ConvertToRads(uint16_t rotation) const {
    const float kToRads = (static_cast<float>(M_PI) / 180.0f);
    float rads = (((40 - (rotation + 30)) % 40) * 9.0f) * kToRads;

    return rads;
  }
};

}  // namespace marvin
