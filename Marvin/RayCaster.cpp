

#include <algorithm>

#include "Bot.h"
#include "Debug.h"
#include "Map.h"
#include "RegionRegistry.h"
#include "RayCaster.h"

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
  if (TiledRayBoxIntersect(point, direction, box_pos, box_extent, dist, norm)) {
    return true;
  }

  return TiledRayBoxIntersect(point, -direction, box_pos, box_extent, dist, norm);
}

bool TiledRayBoxIntersect(Vector2f origin, Vector2f direction, Vector2f box_pos, Vector2f box_extent, float* dist,
                      Vector2f* norm) {
  Vector2f lb = box_pos + Vector2f(0, box_extent.y);
  Vector2f rt = box_pos + Vector2f(box_extent.x, 0);
  if (RayBoxIntersect(origin, direction, lb, rt, dist, norm)) {
    return true;
  }
  return false;
}

bool FloatingRayBoxIntersect(Vector2f origin, Vector2f direction, Vector2f box_pos, float box_size, float* dist,
                      Vector2f* norm) {
  Vector2f lb = box_pos + Vector2f(-box_size, box_size);
  Vector2f rt = box_pos + Vector2f(box_size, -box_size);
  if (RayBoxIntersect(origin, direction, lb, rt, dist, norm)) {
    return true;
  }
  return false;
}

bool RayBoxIntersect(Vector2f origin, Vector2f direction, Vector2f lb, Vector2f rt, float* dist,
                     Vector2f* norm) {
  Vector2f recip(1.0f / direction.x, 1.0f / direction.y);

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

bool IsBarrier(Bot& bot, const Vector2f& position, RayBarrier barrier) {
  auto& map = bot.GetGame().GetMap();
  auto& registry = bot.GetRegions();

  switch (barrier) {
    case RayBarrier::Solid: {
      if (map.IsSolid(position)) {
        return true;
      }
    } break;
    case RayBarrier::Edge: {
      if (registry.IsEdge(position)) {
        return true;
      }
    } break;
  }
  return false;
}

CastResult RayCast(Bot& bot, RayBarrier barrier, Vector2f from, Vector2f direction, float max_length) {
  auto& map = bot.GetGame().GetMap();
  Vector2f vMapSize = {1024.0f, 1024.0f};

  CastResult result;

  // this can result in division by 0.0f infinity which means that the direction is parallel with the axis and will never intersect.
  // in this case the ray caster will only step in one direction.
  Vector2f vRayUnitStepSize = Vector2f(abs(1.0f / direction.x), abs(1.0f / direction.y));

  Vector2f vMapCheck = Vector2f(std::floor(from.x), std::floor(from.y));
  Vector2f vRayLength1D, vStep, reflection;

  float fDistance = 0.0f;

  bool cornered = false, bTileFound = false;

  // fix for when stepping out of a bottom right corner both the first Y and the X steps are solid tiles
  // this is a logic error if the raycaster were to start inside a solid position, but this should be
  // safe since the raycaster is not intended to start inside of a wall.

  if ((int)from.x == from.x && (int)from.y == from.y) {
    if (vRayUnitStepSize.x == vRayUnitStepSize.y) {
      if (direction.x < 0.0f && direction.y < 0.0f) {
        if (map.IsSolid((unsigned short)vMapCheck.x - 1, (unsigned short)vMapCheck.y) &&
            map.IsSolid((unsigned short)vMapCheck.x, (unsigned short)vMapCheck.y - 1)) {
          cornered = true;
        }
      }
    }
  }

  // this decides how long the first step should be from the starting point
  // and which direction to step in.
  // a 0 value means the starting point is perfectly aligned with a wall tile
  if (direction.x < 0) {
    vStep.x = -1.0f;
    vRayLength1D.x = (from.x - float(vMapCheck.x)) * vRayUnitStepSize.x;
  } else {
    vStep.x = 1.0f;
    vRayLength1D.x = (float(vMapCheck.x + 1) - from.x) * vRayUnitStepSize.x;
  }

  if (direction.y < 0) {
    vStep.y = -1.0f;
    vRayLength1D.y = (from.y - float(vMapCheck.y)) * vRayUnitStepSize.y;
  } else {
    vStep.y = 1.0f;
    vRayLength1D.y = (float(vMapCheck.y + 1) - from.y) * vRayUnitStepSize.y;
  }

  // Perform "Walk" until collision or range check
  while (!bTileFound && fDistance < max_length) {
    // Walk along shortest path
    if (vRayLength1D.x < vRayLength1D.y) {
      vMapCheck.x += vStep.x;
      fDistance = vRayLength1D.x;
      vRayLength1D.x += vRayUnitStepSize.x;
      reflection = Vector2f(-1, 1);
    } else {
      vMapCheck.y += vStep.y;
      fDistance = vRayLength1D.y;
      vRayLength1D.y += vRayUnitStepSize.y;
      reflection = Vector2f(1, -1);
    }

    // Test tile at new test point
    if (vMapCheck.x >= 0 && vMapCheck.x < vMapSize.x && vMapCheck.y >= 0 && vMapCheck.y < vMapSize.y) {
      //if (map.IsSolid((unsigned short)vMapCheck.x, (unsigned short)vMapCheck.y)) {
      if (IsBarrier(bot, vMapCheck, barrier)) {
        bool skipFirstCheck = cornered && fDistance == 0.0f;

        if (!skipFirstCheck) {
          bTileFound = true;
        }
      }
    }
  }

  if (bTileFound) {

    result.hit = true;
    result.distance = fDistance;
    result.position = from + direction * fDistance;

    /* special case handling for reflections off of a corner tile when the intersection has a 0 decimal
     (the intersected position landed exactly on the tiles map coordinate)

     the raycaster has to run twice to make 2 dicerection changes and reflect out of a corner.
     the rayboxintersect and the reflection when stepping both get this wrong on the 2nd step

       The first step calculates a floored corner, feeding that back into the 2nd step
       results in a reflection in an unintended direction.

       The following below looks for this specific situation and reverses the direction so the raycaster
       can avoid the 2nd step that it doesnt handle correctly.
       */

    // if the calculated intersection has a 0 decimal (floored)
    if ((int)result.position.x == result.position.x && (int)result.position.y == result.position.y) {
      /* when pointing into a lower right corner it finds the bottom left tile to be solid,
         it then calculates a floored position and reflects Y upward, when fed back into the raycaster
         it says that the vraylength for Y is 0 and X is 1, takes a 0 distance step and says that the Y
         step is solid, and reflects Y back down.
        */
      if (direction.x > 0 && direction.y > 0) {
        if (map.IsSolid((unsigned short)result.position.x - 1, (unsigned short)result.position.y) &&
            map.IsSolid((unsigned short)result.position.x, (unsigned short)result.position.y - 1)) {
          reflection = Vector2f(-1, -1);
        }
      }

      if (direction.x < 0 && direction.y > 0) {
        if (map.IsSolid((unsigned short)result.position.x - 1, (unsigned short)result.position.y - 1) &&
            map.IsSolid((unsigned short)result.position.x, (unsigned short)result.position.y)) {
          reflection = Vector2f(-1, -1);
        }
      }

      if (direction.x > 0 && direction.y < 0) {
        if (map.IsSolid((unsigned short)result.position.x - 1, (unsigned short)result.position.y - 1) &&
            map.IsSolid((unsigned short)result.position.x, (unsigned short)result.position.y)) {
          reflection = Vector2f(-1, -1);
        }
      }

      /* when pointing into a upper left corner it finds the bottom left corner tile to be solid,
         returns a floored position and reflects to the right? */
      if (direction.x < 0 && direction.y < 0) {
        if (map.IsSolid((unsigned short)result.position.x - 1, (unsigned short)result.position.y) &&
            map.IsSolid((unsigned short)result.position.x, (unsigned short)result.position.y - 1)) {
          reflection = Vector2f(-1, -1);
        }
      }
    }
    result.normal = reflection;
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
// return hit if any cast line hits a solid tile
bool DiameterRayCastHit(Bot& bot, Vector2f from, Vector2f to, float radius) {
  bool result = false;

  Vector2f to_target = to - from;
  Vector2f direction = Normalize(to_target);
  Vector2f side = Perpendicular(direction);

  CastResult center = RayCast(bot, RayBarrier::Solid, from, direction, to_target.Length());
  CastResult side1 = RayCast(bot, RayBarrier::Solid, from + side * radius, direction, to_target.Length());
  CastResult side2 = RayCast(bot, RayBarrier::Solid, from - side * radius, direction, to_target.Length());

  if (center.hit || side1.hit || side2.hit) {
    result = true;
  }
  return result;
}

/* 
Return false if only leftside or rightside is a hit.  Good for line of sight checks where a single raycast can trigger line of
sight through holes the ship cant path through, and a diameter raycast makes the ship slower to respond when
meeting an enemy right around a corner
*/
bool RadiusRayCastHit(Bot& bot, Vector2f from, Vector2f to, float radius) {
  bool result = false;

  Vector2f to_target = to - from;
  Vector2f direction = Normalize(to_target);
  Vector2f side = Perpendicular(direction);

  CastResult center = RayCast(bot, RayBarrier::Solid, from, direction, to_target.Length());
  CastResult side1 = RayCast(bot, RayBarrier::Solid, from + side * radius, direction, to_target.Length());
  CastResult side2 = RayCast(bot, RayBarrier::Solid, from - side * radius, direction, to_target.Length());

  if (center.hit) {
    result = true;
  } else if (side1.hit && side2.hit) {
  result = true;
  }
  return result;
}

// call the raycaster for common use, stops on solid tiles
CastResult SolidRayCast(Bot& bot, Vector2f from, Vector2f to) {
  Vector2f to_target = to - from;
  CastResult result = RayCast(bot, RayBarrier::Solid, from, Normalize(to_target), to_target.Length());

  return result;
}

// call the raycaster for region use, stops on edges calculated by the region registry (useful for bases)
CastResult EdgeRayCast(Bot& bot, const RegionRegistry& registry, Vector2f from, Vector2f to) {
  Vector2f to_target = to - from;
  CastResult result = RayCast(bot, RayBarrier::Edge, from, Normalize(to_target), to_target.Length());

  return result;
}

}  // namespace marvin
