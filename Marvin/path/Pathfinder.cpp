#include "Pathfinder.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_set>

#include "../Bot.h"
#include "../Debug.h"
#include "../RayCaster.h"

extern std::unique_ptr<marvin::Bot> bot;

/* when creating map weights, a single tile with a weight of 10 equals 10 tiles with a weight of 1,
or 5 tiles with a weight of 2, and so on. If a tile has a weight of 9, the pathfinder will need to step into 
9 tiles with a weight of 1 before stepping into that tile. */

namespace marvin {

    

Vector2f LastLOSNode(const Map& map, std::size_t index, bool count_down, std::vector<Vector2f> path, float radius) {
  Vector2f position;

  if (path.empty()) {
    return position;
  }

  if (count_down) {
    // position = path.back();

    for (std::size_t i = index; i > 0; i--) {
      Vector2f current = path[i];

      if (!RadiusRayCastHit(map, path[index], current, radius)) {
        position = current;
      } else {
        break;
      }
    }
  } else {
    //  position = path.front();

    for (std::size_t i = index; i < path.size(); i++) {
      Vector2f current = path[i];

      if (!RadiusRayCastHit(map, path[index], current, radius)) {
        position = current;
      } else {
        break;
      }
    }
  }

  return position;
}

float PathLength(std::vector<Vector2f> path, Vector2f pos1, Vector2f pos2) {
  float path_distance = 0.0f;

  if (path.empty()) {
    return path_distance;
  }

  std::size_t index1 = FindPathIndex(path, pos1);
  std::size_t index2 = FindPathIndex(path, pos2);

  if (index1 == index2) {
    return pos1.Distance(pos2);
  }

  std::size_t start = std::min(index1, index2);
  std::size_t end = std::max(index1, index2);

  for (std::size_t i = start; i < end; i++) {
    path_distance += path[i].Distance(path[i + 1]);
  }

  return path_distance;
}

std::size_t FindPathIndex(std::vector<Vector2f> path, Vector2f position) {
  std::size_t path_index = 0;
  float closest_distance = std::numeric_limits<float>::max();

  if (path.empty()) {
    return path_index;
  }

  for (std::size_t i = 0; i < path.size(); i++) {
    float distance = position.DistanceSq(path[i]);

    if (closest_distance > distance) {
      path_index = i;
      closest_distance = distance;
    }
  }

  return path_index;
}

namespace path {

    void Pathfinder::DebugUpdate(const Vector2f& position) {   
        for (float y = -5.00; y <= 5.0f; y++) {
        for (float x = -5.0f; x <= 5.0f; x++) {
          Node* node = this->processor_->GetNode(NodePoint(uint16_t(position.x + x), uint16_t(position.y + y)));
          if (node->can_occupy) {
            Vector2f check(std::floor(position.x) + x, std::floor(position.y) + y);
            RenderWorldLine(position, check, check + Vector2f(1, 1), RGB(255, 100, 100));
            RenderWorldLine(position, check + Vector2f(0, 1), check + Vector2f(1, 0), RGB(255, 100, 100));
          }
        }
      }
    }

bool IsPassablePath(const Map& map, Vector2f from, Vector2f to, float radius) {
  const Vector2f direction = Normalize(to - from);
  const Vector2f side = Perpendicular(direction) * radius;
  const float distance = from.Distance(to);

  CastResult cast_center = RayCast(map, from, direction, distance);
  CastResult cast_side1 = RayCast(map, from + side, direction, distance);
  CastResult cast_side2 = RayCast(map, from - side, direction, distance);

  return !cast_center.hit && !cast_side1.hit && !cast_side2.hit;
}

NodePoint ToNodePoint(const Vector2f v, float radius, const Map& map) {
  NodePoint np;

  np.x = (uint16_t)v.x;
  np.y = (uint16_t)v.y;

  Vector2f check_pos(np.x, np.y);

  // in case the start or end point isnt on a pathable node, attmept to find one nearby
  if (!map.CanPathOn(check_pos, radius + 0.5f)) {
    int check_diameter = (int)((radius + 0.5f) * 2) + 1;

    // start in the center and search outward
    for (int i = 1; i < check_diameter; i++) {
      for (int y = -i; y <= i; y++) {
        for (int x = -i; x <= i; x++) {
          check_pos = Vector2f(np.x + (float)x, np.y + (float)y);
          if (map.CanPathOn(check_pos, radius)) {
            np.y += y;
            np.x += x;
            return np;
          }
        }
      }
    }
  }

  return np;
}

inline float Euclidean(NodeProcessor& processor, const Node* from, const Node* to) {
  NodePoint from_p = processor.GetPoint(from);
  NodePoint to_p = processor.GetPoint(to);

  float dx = static_cast<float>(from_p.x - to_p.x);
  float dy = static_cast<float>(from_p.y - to_p.y);

  return sqrt(dx * dx + dy * dy);
}

Pathfinder::Pathfinder(std::unique_ptr<NodeProcessor> processor, RegionRegistry& regions)
    : processor_(std::move(processor)), regions_(regions) {}

std::vector<Vector2f> Pathfinder::FindPath(const Map& map, const std::vector<Vector2f>& mines, const Vector2f& from,
                                           const Vector2f& to, float radius) {
  std::vector<Vector2f> path;

  // Clear the touched nodes before pathfinding.
  for (Node* node : touched_nodes_) {
    // Setting the flag to zero causes GetNode to reset the node on next fetch.
    node->flags = 0;
  }
  touched_nodes_.clear();

  Node* start = processor_->GetNode(ToNodePoint(from, radius, map));
  Node* goal = processor_->GetNode(ToNodePoint(to, radius, map));

  if (start == nullptr || goal == nullptr) {
    return path;
  }

  NodePoint start_p = processor_->GetPoint(start);
  NodePoint goal_p = processor_->GetPoint(goal);

  if (!regions_.IsConnected(MapCoord(start_p.x, start_p.y), MapCoord(goal_p.x, goal_p.y))) {
    return path;
  }

  // clear vector then add start node
  openset_.Clear();
  openset_.Push(start);

  touched_nodes_.insert(start);
  touched_nodes_.insert(goal);

  // at the start there is only one node here, the start node
  while (!openset_.Empty()) {
    // grab front item then delete it
    Node* node = openset_.Pop();

    touched_nodes_.insert(node);

    // this is the only way to break the pathfinder
    if (node == goal) {
      break;
    }

    node->flags |= NodeFlag_Closed;

    // returns neighbor nodes that are not solid
    NodeConnections connections = processor_->FindEdges(mines, node, start, goal);

    for (std::size_t i = 0; i < connections.count; ++i) {
      Node* edge = connections.neighbors[i];

      touched_nodes_.insert(edge);

      float cost = node->g + edge->weight * Euclidean(*processor_, node, edge);

      if ((edge->flags & NodeFlag_Closed) && cost < edge->g) {
        edge->flags &= ~NodeFlag_Closed;
      }

      float h = Euclidean(*processor_, edge, goal);

      if (!(edge->flags & NodeFlag_Openset) || cost + h < edge->f) {
        edge->g = cost;
        edge->f = edge->g + h;
        edge->parent = node;

        edge->flags |= NodeFlag_Openset;

        openset_.Push(edge);
      }
    }
  }

  if (goal->parent) {
    path.push_back(Vector2f(start_p.x + 0.5f, start_p.y + 0.5f));
  }

  // Construct path backwards from goal node
  std::vector<NodePoint> points;
  Node* current = goal;

  while (current != nullptr && current != start) {
    NodePoint p = processor_->GetPoint(current);
    points.push_back(p);
    current = current->parent;
  }

  // Reverse and store as vector
  for (std::size_t i = 0; i < points.size(); ++i) {
    std::size_t index = points.size() - i - 1;
    Vector2f pos(points[index].x + 0.5f, points[index].y + 0.5f);

    path.push_back(pos);
  }

  return path;
}

std::vector<Vector2f> Pathfinder::SmoothPath(const std::vector<Vector2f>& path, const Map& map, float ship_radius) {
  std::vector<Vector2f> result = path;
  return result;


 for (std::size_t i = 0; i < result.size(); i++) {
    std::size_t next = i + 1;

    if (next == result.size() - 1) {
      return result;
    }

    bool hit = RadiusRayCastHit(map, result[i], result[next], 3.0f);

    if (!hit) {
      result.erase(result.begin() + next);
      i--;
    }
 }

  return result;
}



std::vector<Vector2f> Pathfinder::CreatePath(std::vector<Vector2f> path, Vector2f from, Vector2f to, float radius) {
  bool build = true;

  std::vector<Vector2f> new_path = path;

  if (!new_path.empty()) {
    // Check if the current destination is the same as the requested one.
    if (new_path.back().DistanceSq(to) < 3 * 3) {
      Vector2f pos = processor_->GetGame().GetPosition();
      Vector2f next = new_path.front();
      Vector2f direction = Normalize(next - pos);
      Vector2f side = Perpendicular(direction);
      float radius = processor_->GetGame().GetShipSettings().GetRadius();

      float distance = next.Distance(pos);

      // Rebuild the path if the bot isn't in line of sight of its next node.
      CastResult center = RayCast(processor_->GetGame().GetMap(), pos, direction, distance);
      CastResult side1 = RayCast(processor_->GetGame().GetMap(), pos + side * radius, direction, distance);
      CastResult side2 = RayCast(processor_->GetGame().GetMap(), pos - side * radius, direction, distance);

      if (!center.hit && !side1.hit && !side2.hit) {
        build = false;
      }
    }
  }

  if (build) {
    std::vector<Vector2f> mines;
    new_path.clear();
    //#if 0
    for (Weapon* weapon : processor_->GetGame().GetWeapons()) {
      const Player* weapon_player = processor_->GetGame().GetPlayerById(weapon->GetPlayerId());
      if (weapon_player == nullptr) continue;
      if (weapon_player->frequency == processor_->GetGame().GetPlayer().frequency) continue;
      if (weapon->IsMine()) mines.push_back(weapon->GetPosition());
    }
    //#endif
    new_path = FindPath(processor_->GetGame().GetMap(), mines, from, to, radius);
   // new_path = SmoothPath(new_path, processor_->GetGame().GetMap(), radius);
  }

  return new_path;
}

float Pathfinder::GetWallDistance(const Map& map, u16 x, u16 y, u16 radius) {
  float closest_sq = std::numeric_limits<float>::max();

  for (i16 offset_y = -radius; offset_y <= radius; ++offset_y) {
    for (i16 offset_x = -radius; offset_x <= radius; ++offset_x) {
      u16 check_x = x + offset_x;
      u16 check_y = y + offset_y;

      if (map.IsSolid(check_x, check_y)) {
        float dist_sq = (float)(offset_x * offset_x + offset_y * offset_y);

        if (dist_sq < closest_sq) {
          closest_sq = dist_sq;
        }
      }
    }
  }
  return sqrt(closest_sq);
}

void Pathfinder::CreateMapWeights(const Map& map) {

  for (u16 y = 0; y < 1024; ++y) {
    for (u16 x = 0; x < 1024; ++x) {
      if (map.IsSolid(x, y)) continue;

      Node* node = this->processor_->GetNode(NodePoint(x, y));

      // Search width is double this number (for 8, searches a 16 x 16 square).
      int close_distance = 8;

      /* Causes exponentianl weight increase as the path gets closer to wall tiles.  
      Known Issue: 3 tile gaps and 4 tile gaps will carry the same weight since each tiles closest wall is 2 tiles away.*/

      float distance = GetWallDistance(map, x, y, close_distance);

      // Nodes are initialized with a weight of 1.0f, so never calculate when the distance is greater or equal
      // because the result will be less than 1.0f.
      if (distance < close_distance) {
        float weight = 8.0f / distance;
        //paths directly next to a wall will be a last resort, 1 tile from wall very unlikely
        node->weight = (float)std::pow(weight, 4.0);
        node->previous_weight = node->weight;
      }
    }
  }
}

void Pathfinder::SetPathableNodes(const Map& map, float radius) {
  for (u16 y = 0; y < 1024; ++y) {
    for (u16 x = 0; x < 1024; ++x) {

      if (map.IsSolid(x, y)) continue;

      Node* node = this->processor_->GetNode(NodePoint(x, y));

      if (map.CanPathOn(Vector2f(x, y), radius)) {
        node->is_pathable = true;
      }
      if (map.CanOccupy(Vector2f(x, y), radius)) {
        node->can_occupy = true;
      }
    }
  }
}

}  // namespace path
}  // namespace marvin
