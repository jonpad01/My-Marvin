#pragma once

#include <cstdlib>
#include <cstdint>
#include <cmath>



namespace marvin {

    class Vector2f;

  struct MapCoord {
    uint16_t x;
    uint16_t y;

    MapCoord() : x(0), y(0) {}
    MapCoord(uint16_t x, uint16_t y) : x(x), y(y) {}
    MapCoord(Vector2f vec);

    bool operator==(const MapCoord& other) const { return x == other.x && y == other.y; }

    inline MapCoord operator-(const MapCoord& other) const { return MapCoord(x - other.x, y - other.y); }

    inline float Length() const { return (float)std::sqrt(x * x + y * y); }

   float Distance(const Vector2f& other) const;

   float Distance(const MapCoord& other) const;

  };


}
