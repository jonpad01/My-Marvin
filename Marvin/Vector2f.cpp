#include "Vector2f.h"
#include "MapCoord.h"

namespace marvin {

Vector2f::Vector2f(const MapCoord& other) : x(float(other.x)), y(float(other.y)) {}

Vector2f Vector2f::operator=(const MapCoord& other) {
  x = float(other.x);
  y = float(other.y);
  return *this;
}

Vector2f Vector2f::operator-(const MapCoord& other) const {
  return Vector2f(x - float(other.x), y - float(other.y));
}

float Vector2f::Distance(const MapCoord& other) const {
  Vector2f to_other = *this - other;
  return to_other.Length();
}

}  // namespace marvin