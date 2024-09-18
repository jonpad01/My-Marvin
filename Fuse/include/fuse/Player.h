#pragma once

#define _USE_MATH_DEFINES
#include <fuse/Math.h>
#include <fuse/Types.h>
#include <math.h>

#include <string>

namespace fuse {

struct Player {
  uint16_t id;
  uint16_t ship;

  Vector2f position;
  Vector2f velocity;

  uint16_t frequency;
  uint16_t discrete_rotation;

  int32_t energy;
  uint8_t status;

  int32_t bounty;

  u32 respawn_time;
  u8 ping;

  FUSE_EXPORT const std::string& GetName() const { return name; }
  FUSE_EXPORT void SetName(const std::string& name) {
    this->name = name;
  }

  FUSE_EXPORT Vector2f GetHeading() const {
    const float kToRads = (static_cast<float>(M_PI) / 180.0f);
    float rads = (((40 - (discrete_rotation + 30)) % 40) * 9.0f) * kToRads;
    float x = cosf(rads);
    float y = -sinf(rads);

    return Vector2f(x, y);
  }

 private:
  std::string name;
};

}  // namespace fuse
