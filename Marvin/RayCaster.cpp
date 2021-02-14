#include "RayCaster.h"

#include "Map.h"
#include "Debug.h"
#include <algorithm>

namespace marvin {

float BoxPointDistance(Vector2f box_pos, Vector2f box_extent, Vector2f p) {
  Vector2f bmin = box_pos;
  Vector2f bmax = box_pos + box_extent;

  float dx = std::max(std::max(bmin.x - p.x, 0.0f), p.x - bmax.x);
  float dy = std::max(std::max(bmin.y - p.y, 0.0f), p.y - bmax.y);

  return std::sqrt(dx * dx + dy * dy);
}

bool LineBoxIntersect(Vector2f point, Vector2f direction, Vector2f box_pos, Vector2f box_extent, float* dist, Vector2f* norm) {
  if (RayBoxIntersect(point, direction, box_pos, box_extent, dist, norm)) {
    return true;
  }

  return RayBoxIntersect(point, -direction, box_pos, box_extent, dist, norm);
}

bool RayBoxIntersect(Vector2f origin, Vector2f direction, Vector2f box_pos, Vector2f box_extent, float* dist, Vector2f* norm) {

    if (direction.x == 0.0f) { direction.x = 0.00001f; }
    if (direction.y == 0.0f) { direction.y = 0.00001f;  }

  Vector2f recip(1.0f / direction.x, 1.0f / direction.y);
  Vector2f lb = box_pos + Vector2f(0, box_extent.y);
  Vector2f rt = box_pos + Vector2f(box_extent.x, 0);

  float t1 = (float)((lb.x - origin.x) * recip.x);
  float t2 = (float)((rt.x - origin.x) * recip.x);
  float t3 = (float)((lb.y - origin.y) * recip.y);
  float t4 = (float)((rt.y - origin.y) * recip.y);

  using std::min;
  using std::max;

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
        *norm = Vector2f(0, -1);
      } else if (t == t4) {
        *norm = Vector2f(0, 1);
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

    CastResult result;

    if (isinf(max_length) || isnan(max_length)) { return result; }

  static const Vector2f kDirections[] = { Vector2f(0, 0), Vector2f(1, 0), Vector2f(-1, 0), Vector2f(0, 1), Vector2f(0, -1), Vector2f(-1, -1), Vector2f(-1, 1), Vector2f(1, -1), Vector2f(1, 1) };

  
  float closest_distance = std::numeric_limits<float>::max();
  Vector2f closest_normal;

  result.distance = max_length;
  
  for (float i = 0; i < max_length; ++i) {
    Vector2f current = from + direction * i; //step into the direction

    current = Vector2f(std::floor(current.x), std::floor(current.y)); //convert current to a map tile

    for (Vector2f check_direction : kDirections) {
      Vector2f check = current + check_direction; //checks 4 tiles around current?

      if (!map.IsSolid((unsigned short)check.x, (unsigned short)check.y)) continue; //this tile isnt a wall

      float dist;
      Vector2f normal;

      //wall tiles are checked and returns distance + norm
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
    }
  }

  return result;
}

}
