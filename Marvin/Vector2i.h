#pragma once

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace marvin {

class Vector2i {
 public:
  union {
    struct {
      int x;
      int y;
    };

    int values[2];
  };

  Vector2i() : x(0), y(0) {}
  Vector2i(int x, int y) : x(x), y(y) {}

  Vector2i(const Vector2i& other) : x(other.x), y(other.y) {}

  Vector2i& operator=(const Vector2i& other) {
    x = other.x;
    y = other.y;
    return *this;
  }

  bool operator==(const Vector2i& other) const noexcept { return x == other.x && y == other.y; }

  bool operator!=(const Vector2i& other) const noexcept { return !(*this == other); }

  int operator[](std::size_t index) { return values[index]; }

  Vector2i operator-() const { return Vector2i(-x, -y); }

  inline Vector2i& operator+=(int v) noexcept {
    x += v;
    return *this;
  }

  inline Vector2i& operator-=(int v) noexcept {
    x -= v;
    return *this;
  }

  inline Vector2i& operator+=(const Vector2i& other) noexcept {
    x += other.x;
    y += other.y;
    return *this;
  }

  inline Vector2i& operator-=(const Vector2i& other) noexcept {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  inline Vector2i& operator*=(int v) noexcept {
    x *= v;
    y *= v;
    return *this;
  }

  inline Vector2i& operator/=(int v) {
    x /= v;
    y /= v;
    return *this;
  }

  inline Vector2i operator+(const Vector2i& other) const { return Vector2i(x + other.x, y + other.y); }

  inline Vector2i operator-(const Vector2i& other) const { return Vector2i(x - other.x, y - other.y); }

  inline Vector2i operator+(int v) const { return Vector2i(x + v, y + v); }

  inline Vector2i operator-(int v) const { return Vector2i(x - v, y - v); }

  inline Vector2i operator*(int v) const { return Vector2i(x * v, y * v); }

  inline Vector2i operator/(int v) const { return Vector2i(x / v, y / v); }
};

inline bool IsDiagonal(Vector2i v) {
  return v.x != 0 && v.y != 0;
}

inline Vector2i Perpendicular(Vector2i v) {
  // return Vector2i(-v.y, v.x);
  Vector2i result;
  if (v.x == -1) {
    result.y = 1;
    if (v.y == -1) {
      result.x = -1;
    }
    if (v.y == 1) {
      result.x = 1;
    }
    return result;
  }

  if (v.x == 1) {
    result.y = 1;
    if (v.y == -1) {
      result.x = 1;
    }
    if (v.y == 1) {
      result.x = -1;
    }
    return result;
  }

  if (v.y == -1) {
    result.x = 1;
    if (v.x == -1) {
      result.y = -1;
    }
    if (v.x == 1) {
      result.y = 1;
    }
    return result;
  }

  if (v.y == 1) {
    result.x = 1;
    if (v.x == -1) {
      result.y = 1;
    }
    if (v.x == 1) {
      result.y = -1;
    }
    return result;
  }

  return result;
}
}  // namespace marvin