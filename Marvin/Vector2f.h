#pragma once

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>
#include <algorithm>

namespace marvin {

    struct MapCoord;
    enum class Direction : short {West, East, North, NorthWest, NorthEast, South, SouthWest, SouthEast, None};

class Vector2f {
 public:
  union {
    struct {
      float x;
      float y;
    };

    float values[2];
  };

  Vector2f() : x(0), y(0) {}
  Vector2f(float x, float y) : x(x), y(y) {}

  Vector2f(const Vector2f& other) : x(other.x), y(other.y) {}
  Vector2f(const MapCoord& other);

  Vector2f& operator=(const Vector2f& other) {
    x = other.x;
    y = other.y;
    return *this;
  }

  Vector2f operator=(const MapCoord& other);

  bool operator==(const Vector2f& other) const noexcept {
    constexpr float epsilon = 0.000001f;
    return std::fabs(x - other.x) < epsilon && std::fabs(y - other.y) < epsilon;
  }

  bool operator!=(const Vector2f& other) const noexcept { return !(*this == other); }

  float operator[](std::size_t index) { return values[index]; }

  Vector2f operator-() const { return Vector2f(-x, -y); }

  inline Vector2f& operator+=(float v) noexcept {
    x += v;
    return *this;
  }

  inline Vector2f& operator-=(float v) noexcept {
    x -= v;
    return *this;
  }

  inline Vector2f& operator+=(const Vector2f& other) noexcept {
    x += other.x;
    y += other.y;
    return *this;
  }

  inline Vector2f& operator-=(const Vector2f& other) noexcept {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  inline Vector2f& operator*=(float v) noexcept {
    x *= v;
    y *= v;
    return *this;
  }

  inline Vector2f& operator/=(float v) {
    x /= v;
    y /= v;
    return *this;
  }

  inline Vector2f operator+(const Vector2f& other) const { return Vector2f(x + other.x, y + other.y); }

  inline Vector2f operator-(const Vector2f& other) const { return Vector2f(x - other.x, y - other.y); }

  Vector2f operator-(const MapCoord& other) const;

  inline Vector2f operator+(float v) const { return Vector2f(x + v, y + v); }

  inline Vector2f operator-(float v) const { return Vector2f(x - v, y - v); }

  inline Vector2f operator*(float v) const { return Vector2f(x * v, y * v); }

  inline Vector2f operator/(float v) const { return Vector2f(x / v, y / v); }

  inline float operator*(const Vector2f& other) const { return Dot(other); }

  inline float Length() const { return std::sqrt(x * x + y * y); }

  inline float LengthSq() const { return x * x + y * y; }

  inline float Distance(const Vector2f& other) const {
    Vector2f to_other = other - *this;
    return to_other.Length();
  }

  float Distance(const MapCoord& other) const;

  inline float DistanceSq(const Vector2f& other) const {
    Vector2f to_other = other - *this;
    return to_other.LengthSq();
  }

  inline void Normalize() {
    float len = Length();

    if (len > std::numeric_limits<float>::epsilon() * 2) {
      x /= len;
      y /= len;
    }
  }

  inline float Dot(const Vector2f& other) const noexcept { return x * other.x + y * other.y; }

  inline Vector2f& Truncate(float len) {
    if (LengthSq() > len * len) {
      Normalize();
      *this *= len;
    }

    return *this;
  }
};

inline Vector2f Reflect(const Vector2f& direction, const Vector2f& normal) {
  return direction - (normal * (2.0f * (direction.Dot(normal))));
}

inline float DotProduct(const Vector2f& v1, const Vector2f& v2) noexcept {
  return v1.x * v2.x + v1.y * v2.y;
}

inline Vector2f Normalize(const Vector2f& v) {
  Vector2f result = v;

  result.Normalize();

  return result;
}

inline Vector2f Perpendicular(const Vector2f& v) {
  return Vector2f(-v.y, v.x);
}

inline Vector2f Reverse(const Vector2f& v) {
  return v * -1.0f;
}

inline Direction GetDirection(const Vector2f& direction) {
  if (direction.x == -1.0f && direction.y == 0.0f) {
    return Direction::West;
  } else if (direction.x == 1.0f && direction.y == 0.0f) {
    return Direction::East;
  } else if (direction.x == 0.0f && direction.y == -1.0f) {
    return Direction::North;
  } else if (direction.x < 0.0f && direction.y < 0.0f) {
    return Direction::NorthWest;
  } else if (direction.x > 0.0f && direction.y < 0.0f) {
    return Direction::NorthEast;
  } else if (direction.x == 0.0f && direction.y == 1.0f) {
    return Direction::South;
  } else if (direction.x < 0.0f && direction.y > 0.0f) {
    return Direction::SouthWest;
  } else if (direction.x > 0.0f && direction.y > 0.0f) {
    return Direction::SouthEast;
  }
  return Direction::None;
}

inline Vector2f West(const Vector2f& position) {
  return Vector2f(position.x - 1, position.y);
}

inline Vector2f East(const Vector2f& position) {
  return Vector2f(position.x + 1, position.y);
}

inline Vector2f North(const Vector2f& position) {
  return Vector2f(position.x, position.y - 1);
}

inline Vector2f NorthWest(const Vector2f& position) {
  return Vector2f(position.x - 1, position.y - 1);
}

inline Vector2f NorthEast(const Vector2f& position) {
  return Vector2f(position.x + 1, position.y - 1);
}

inline Vector2f South(const Vector2f& position) {
  return Vector2f(position.x, position.y + 1);
}

inline Vector2f SouthWest(const Vector2f& position) {
  return Vector2f(position.x - 1, position.y + 1);
}

inline Vector2f SouthEast(const Vector2f& position) {
  return Vector2f(position.x + 1, position.y + 1);
}

inline Vector2f Rotate(const Vector2f& v, float rads) {
  float cosA = std::cos(rads);
  float sinA = std::sin(rads);

  return Vector2f(cosA * v.x - sinA * v.y, sinA * v.x + cosA * v.y);
}

inline bool operator<(const Vector2f& lhs, const Vector2f& rhs) noexcept {
  if (lhs.y < rhs.y) {
    return true;
  } else if (lhs.y > rhs.y) {
    return false;
  }

  return lhs.x < rhs.x;
}

inline float WrapMax(float x, float max) {
  return fmod(max + fmod(x, max), max);
}

inline float WrapMinMax(float x, float min, float max) {
  return min + WrapMax(x - min, max - min);
}

inline float WrapToPi(float rads) {
  return WrapMinMax(rads, -3.14159f, 3.14159f);
}



// the viewing area the bot could see if it were a player
inline bool InRect(Vector2f pos, Vector2f min_rect, Vector2f max_rect) {
  return ((pos.x >= min_rect.x && pos.y >= min_rect.y) && (pos.x <= max_rect.x && pos.y <= max_rect.y));
}

// probably checks if the target is dead
inline bool IsValidPosition(const Vector2f& position) {
  return position.x >= 0 && position.x < 1024 && position.y >= 0 && position.y < 1024;
}

}  // namespace marvin

inline std::ostream& operator<<(std::ostream& out, const marvin::Vector2f& v) {
  return out << "(" << v.x << ", " << v.y << ")";
}
