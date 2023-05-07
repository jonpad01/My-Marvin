#include "MapCoord.h"
#include "Vector2f.h"

namespace marvin {

	MapCoord::MapCoord(Vector2f vec) : x((uint16_t)vec.x), y((uint16_t)vec.y) {}

}