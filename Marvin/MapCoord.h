#pragma once

#include <cstdlib>
#include <cstdint>



namespace marvin {

    class Vector2f;

  struct MapCoord {
    uint16_t x;
    uint16_t y;

    MapCoord() : x(0), y(0) {}
    MapCoord(uint16_t x, uint16_t y) : x(x), y(y) {}
    MapCoord(Vector2f vec);

    bool operator==(const MapCoord& other) const { return x == other.x && y == other.y; }
  };


}
