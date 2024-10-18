#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

#include <cstdint>
#include <string>

#include "Vector2f.h"
//#include "GameProxy.h"

namespace marvin {

// current status of bots ship
struct ShipFlightStatus {
  uint32_t max_energy;
  uint32_t recharge;
  uint32_t thrust;
  uint32_t speed;
  uint32_t rotation;
  uint32_t shrapnel;
};

// field of booleans
// if the ship is capable of using this status
struct ShipCapability {
  uint8_t stealth : 1;
  uint8_t cloak : 1;
  uint8_t xradar : 1;
  uint8_t antiwarp : 1;
  uint8_t multifire : 1;
  uint8_t bouncing_bullets : 1;
  uint8_t proximity : 1;
  uint8_t padding : 1;
};


struct BallData {
  BallData() : position(0, 0), velocity(0, 0), last_activity(0, 0), inactive_timer(0), last_holder_id(0), held(false) {}
  Vector2f position, velocity, last_activity;
  uint32_t inactive_timer, last_holder_id;
  bool held;
};



struct Player {

  void Update(const Player& o) { 
    name = o.name;
    generation_id = o.generation_id;
    multifire_status = o.multifire_status;
    dead = o.dead;
    energy = o.energy;
    id = o.id;
    attach_id = o.attach_id;
    ship = o.ship;
    bounty = o.bounty;
    flags = o.flags;
    emp_ticks = o.emp_ticks;

    position = o.position;
    velocity = o.velocity;

    frequency = o.frequency;
    discrete_rotation = o.discrete_rotation;

    repels = o.repels;
    bursts = o.bursts;
    decoys = o.decoys;
    thors = o.thors;
    bricks = o.bricks;
    rockets = o.rockets;
    portals = o.portals;

    capability = o.capability;
    flight_status = o.flight_status;

    // bitfield
    status = o.status;
  }

  std::string name;
  int generation_id;

  bool dead;
//  bool active;

  bool multifire_status;
 // bool multifire_capable;

  int32_t energy;
  uint16_t id;
  uint16_t attach_id;
  uint16_t ship; // 0 = warbird, 8 = spectator 
  uint16_t previous_ship = 9999;
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

  // 0 = disabled, 1 = red, 2 = yellow, 3 = blue
  uint32_t gun_level;
  uint32_t bomb_level;

  ShipCapability capability;
  ShipFlightStatus flight_status;

  // bitfield
  uint8_t status;
  //ShipToggleStatus toggle_status;

  // input positive and negative multifire angle to ge multifire heading
  Vector2f GetHeading(float multifire_angle = 0.0f) const {
    const float kToRads = (static_cast<float>(M_PI) / 180.0f);
    float rads = (((40 - (discrete_rotation + 30)) % 40) * 9.0f) * kToRads;

    rads += multifire_angle * kToRads;

    float x = std::cos(rads);
    float y = -std::sin(rads);

    return Vector2f(x, y);
  }

  float GetRadians() const {
    const float kToRads = (static_cast<float>(M_PI) / 180.0f);
    return (((40 - (discrete_rotation + 30)) % 40) * 9.0f) * kToRads;
  }
};

struct BotPlayer {
  std::string name;
 

  bool dead;
  // bool active;

  bool multifire_status;
 // bool multifire_capable;

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

  uint32_t gun_level;
  uint32_t bomb_level;

  ShipCapability capability;
  ShipFlightStatus flight_status;
  //ShipToggleStatus toggle_status;

  // bitfield
  uint8_t status;

  BotPlayer() { 
   //   capability = {};
  }

  // input positive and negative multifire angle to ge multifire heading
  Vector2f GetHeading(float multifire_angle = 0.0f) const {
    const float kToRads = (static_cast<float>(M_PI) / 180.0f);
    float rads = GetRadians();

    rads += multifire_angle * kToRads;

    float x = std::cos(rads);
    float y = -std::sin(rads);

    return Vector2f(x, y);
  }

  float GetRadians() const {
    const float kToRads = (static_cast<float>(M_PI) / 180.0f);
    return (((40 - (discrete_rotation + 30)) % 40) * 9.0f) * kToRads;
  }

  const BotPlayer& operator=(const Player& other) {
    name = other.name;
    dead = other.dead;
    energy = other.energy;
    id = other.id;
    ship = other.ship;
    bounty = other.bounty;
    flags = other.flags;
    position = other.position;
    velocity = other.velocity;
    frequency = other.frequency;
    discrete_rotation = other.discrete_rotation;
    status = other.status;
   // toggle_status = other.toggle_status;
  }

};



}  // namespace marvin
