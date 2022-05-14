#pragma once

#include "Vector2f.h"
//#include "Bot.h"

namespace marvin {

class Map;
class Bot;


enum class RayBarrier { Solid, Edge };

struct CastResult {
  CastResult() : hit(false), distance(0.0f), position(Vector2f()), normal(Vector2f()) {}
  bool hit;
  float distance;
  Vector2f position;
  Vector2f normal;
};

bool LineBoxIntersect(Vector2f point, Vector2f direction, Vector2f box_pos, Vector2f box_extent, float* dist,
                      Vector2f* norm);
bool RayBoxIntersect(Vector2f origin, Vector2f direction, Vector2f lb, Vector2f rt, float* dist,
                     Vector2f* norm);
bool FloatingRayBoxIntersect(Vector2f origin, Vector2f direction, Vector2f box_pos, float box_size, float* dist,
                          Vector2f* norm);
bool TiledRayBoxIntersect(Vector2f origin, Vector2f direction, Vector2f box_pos, Vector2f box_extent, float* dist,
                         Vector2f* norm);
float BoxPointDistance(Vector2f box_pos, Vector2f box_extent, Vector2f point);

CastResult SolidRayCast(Bot& bot, Vector2f from, Vector2f to);
CastResult EdgeRayCast(Bot& bot, Vector2f from, Vector2f to);


CastResult RayCast(Bot& bot, RayBarrier barrier, Vector2f from, Vector2f direction, float max_length);

bool DiameterEdgeRayCastHit(Bot& bot, Vector2f from, Vector2f to, float radius);
bool RadiusEdgeRayCastHit(Bot& bot, Vector2f from, Vector2f to, float radius);
bool DiameterRayCastHit(Bot& bot, Vector2f from, Vector2f to, float radius);
bool RadiusRayCastHit(Bot& bot, Vector2f from, Vector2f to, float radius);

}  // namespace marvin
