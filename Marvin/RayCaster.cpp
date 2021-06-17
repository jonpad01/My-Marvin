#include "RayCaster.h"

#include <algorithm>

#include "Debug.h"
#include "Map.h"

namespace marvin {

float BoxPointDistance(Vector2f box_pos, Vector2f box_extent, Vector2f p) {
  Vector2f bmin = box_pos;
  Vector2f bmax = box_pos + box_extent;

  float dx = std::max(std::max(bmin.x - p.x, 0.0f), p.x - bmax.x);
  float dy = std::max(std::max(bmin.y - p.y, 0.0f), p.y - bmax.y);

  return std::sqrt(dx * dx + dy * dy);
}

bool LineBoxIntersect(Vector2f point, Vector2f direction, Vector2f box_pos, Vector2f box_extent, float* dist,
                      Vector2f* norm) {
  if (RayBoxIntersect(point, direction, box_pos, box_extent, dist, norm)) {
    return true;
  }

  return RayBoxIntersect(point, -direction, box_pos, box_extent, dist, norm);
}

bool RayBoxIntersect(Vector2f origin, Vector2f direction, Vector2f box_pos, Vector2f box_extent, float* dist,
                     Vector2f* norm) {
  Vector2f recip(1.0f / direction.x, 1.0f / direction.y);
  Vector2f lb = box_pos + Vector2f(0, box_extent.y);
  Vector2f rt = box_pos + Vector2f(box_extent.x, 0);

  float t1 = (float)((lb.x - origin.x) * recip.x);
  float t2 = (float)((rt.x - origin.x) * recip.x);
  float t3 = (float)((lb.y - origin.y) * recip.y);
  float t4 = (float)((rt.y - origin.y) * recip.y);

  using std::max;
  using std::min;

  float tmin = max(min(t1, t2), min(t3, t4));
  float tmax = min(max(t1, t2), max(t3, t4));

  bool intersected = false;
  float t;

  if (tmax < 0.0f) {
    t = tmax;
  } else if (tmin > tmax) {
    t = tmax;
  } else {
    intersected = true;
    t = tmin;

    if (norm) {
      if (t == t1) {
        *norm = Vector2f(-1, 0);
      } else if (t == t2) {
        *norm = Vector2f(1, 0);
      } else if (t == t3) {
        *norm = Vector2f(0, 1);
      } else if (t == t4) {
        *norm = Vector2f(0, -1);
      } else {
        *norm = Vector2f(0, 0);
      }
    }
  }

  if (dist) {
    *dist = t;
  }

  return intersected;
}

CastResult RayCast(const Map& map, Vector2f from, Vector2f direction, float max_length) {
  Vector2f vMapSize = {1024.0f, 1024.0f};

  CastResult result;

  float xStepSize = std::sqrt(1 + (direction.y / direction.x) * (direction.y / direction.x));
  float yStepSize = std::sqrt(1 + (direction.x / direction.y) * (direction.x / direction.y));

  Vector2f vMapCheck = Vector2f(std::floor(from.x), std::floor(from.y));
  Vector2f vRayLength1D;

  Vector2f vStep;

  if (direction.x < 0) {
    vStep.x = -1.0f;
    vRayLength1D.x = (from.x - float(vMapCheck.x)) * xStepSize;
  } else {
    vStep.x = 1.0f;
    vRayLength1D.x = (float(vMapCheck.x + 1) - from.x) * xStepSize;
  }

  if (direction.y < 0) {
    vStep.y = -1.0f;
    vRayLength1D.y = (from.y - float(vMapCheck.y)) * yStepSize;
  } else {
    vStep.y = 1.0f;
    vRayLength1D.y = (float(vMapCheck.y + 1) - from.y) * yStepSize;
  }

  // Perform "Walk" until collision or range check
  bool bTileFound = false;
  float fDistance = 0.0f;

  while (!bTileFound && fDistance < max_length) {
    // Walk along shortest path
    if (vRayLength1D.x < vRayLength1D.y) {
      vMapCheck.x += vStep.x;
      fDistance = vRayLength1D.x;
      vRayLength1D.x += xStepSize;
    } else {
      vMapCheck.y += vStep.y;
      fDistance = vRayLength1D.y;
      vRayLength1D.y += yStepSize;
    }

    // Test tile at new test point
    if (vMapCheck.x >= 0 && vMapCheck.x < vMapSize.x && vMapCheck.y >= 0 && vMapCheck.y < vMapSize.y) {
      if (map.IsSolid((unsigned short)vMapCheck.x, (unsigned short)vMapCheck.y)) {
        bTileFound = true;
      }
    }
  }

  // Calculate intersection location
  Vector2f vIntersection;
  if (bTileFound) {
    vIntersection = from + direction * fDistance;
    float dist;

    result.hit = true;
    result.distance = fDistance;
    result.position = vIntersection;

    // wall tiles are checked and returns distance + norm
    RayBoxIntersect(from, direction, vMapCheck, Vector2f(1.0f, 1.0f), &dist, &result.normal);
  }

#if 0

  if (isinf(max_length) || isnan(max_length)) {
    return result;
  }

  static const Vector2f kDirections[] = {Vector2f(0, 0),  Vector2f(1, 0),  Vector2f(-1, 0),
                                         Vector2f(0, 1),  Vector2f(0, -1), Vector2f(1, 1),
                                         Vector2f(-1, 1), Vector2f(1, -1), Vector2f(-1, -1)};

  float closest_distance = std::numeric_limits<float>::max();
  Vector2f closest_normal;

  result.distance = max_length;

  for (float i = 0; i < max_length; ++i) {
    Vector2f current = from + direction * i;  // step into the direction

    current = Vector2f(std::floor(current.x), std::floor(current.y));  // convert current to a map tile

    for (Vector2f check_direction : kDirections) {
      Vector2f check = current + check_direction;  // checks 4 tiles around current?

      if (!map.IsSolid((unsigned short)check.x, (unsigned short)check.y)) continue;  // this tile isnt a wall

      float dist;
      Vector2f normal;

      // wall tiles are checked and returns distance + norm
      if (RayBoxIntersect(from, direction, check, Vector2f(1, 1), &dist, &normal)) {
        if (dist < closest_distance && dist >= 0.0f) {
          closest_distance = dist;
          closest_normal = normal;
        }
      }
    }

    if (closest_distance < max_length) {
      result.hit = true;
      result.normal = closest_normal;
      result.distance = closest_distance;
      result.position = from + direction * closest_distance;
      return result;
    }
  }

#endif

  return result;
}

bool RadiusRayCastHit(const Map& map, Vector2f from, Vector2f to, float radius) {
  bool result = true;

  Vector2f to_target = to - from;
  CastResult center = RayCast(map, from, Normalize(to_target), to_target.Length());

  if (!center.hit) {
    Vector2f direction = Normalize(to_target);
    Vector2f side = Perpendicular(direction);

    CastResult side1 = RayCast(map, from + side * radius, direction, to_target.Length());

    if (!side1.hit) {
      CastResult side2 = RayCast(map, from - side * radius, direction, to_target.Length());

      if (!side2.hit) {
        result = false;
      }
    }
  }
  return result;
}

}  // namespace marvin
