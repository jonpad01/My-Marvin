#include "MapCoord.h"
#include "Vector2f.h"

namespace marvin {

MapCoord::MapCoord(Vector2f vec) : x((uint16_t)vec.x), y((uint16_t)vec.y) {}

float MapCoord::Distance(const MapCoord& other) const {
  MapCoord to_other = *this - other;
  return to_other.Length();
}

float MapCoord::Distance(const Vector2f& other) const {
  Vector2f to_other = other - *this;
  return to_other.Length();
}

}  // namespace marvin