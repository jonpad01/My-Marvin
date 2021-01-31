#pragma once

#include <cstdint>

#include "../Hash.h"

namespace marvin {
namespace path {

struct NodePoint {
  uint16_t x;
  uint16_t y;

  NodePoint() : x(0), y(0) {}
  NodePoint(uint16_t x, uint16_t y) : x(x), y(y) {}

  bool operator==(const NodePoint& other) const {
    return x == other.x && y == other.y;
  }
};

struct Node {
  NodePoint point;
  Node* parent;
  float g;
  float h;
  float f;
  float weight;
  float previous_weight;
  uint32_t rotations;
  bool closed;
  bool openset;

  Node()
      : closed(false),
      openset(false),
      parent(nullptr),
      g(0.0f),
      h(0.0f),
      f(0.0f),
      weight(1.0f),
      previous_weight(1.0f),
      rotations(0) {}
};

}  // namespace path
}  // namespace marvin

MAKE_HASHABLE(marvin::path::NodePoint, t.x, t.y);
