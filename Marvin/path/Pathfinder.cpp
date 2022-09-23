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

std::vector<Vector2f> Pathfinder::SmoothPath(Bot& bot, const std::vector<Vector2f>& path, float ship_radius) {
  std::vector<Vector2f> result = path;
  return result;


 for (std::size_t i = 0; i < result.size(); i++) {
    std::size_t next = i + 1;

    if (next == result.size() - 1) {
      return result;
    }

    bool hit = DiameterRayCastHit(bot, result[i], result[next], 3.0f);

    if (!hit) {
      result.erase(result.begin() + next);
      i--;
    }
 }

  return result;
}



std::vector<Vector2f> Pathfinder::CreatePath(Bot& bot, Vector2f from, Vector2f to, float radius) {
  bool build = true;

  if (!path_.empty()) {
    // Check if the current destination is the same as the requested one.
    if (path_.back().DistanceSq(to) < 3 * 3) {
      Vector2f pos = processor_->GetGame().GetPosition();
      Vector2f next = path_.front();
      Vector2f direction = Normalize(next - pos);
      Vector2f side = Perpendicular(direction);
      float radius = processor_->GetGame().GetShipSettings().GetRadius();

      float distance = next.Distance(pos);


      // Rebuild the path if the bot isn't in line of sight of its next node.
      if (!DiameterRayCastHit(bot, pos, next, radius)) {
       build = false;
      }
    }
  }

  if (build) {
    std::vector<Vector2f> mines;
    path_.clear();
    //#if 0
    for (Weapon* weapon : processor_->GetGame().GetWeapons()) {
      const Player* weapon_player = processor_->GetGame().GetPlayerById(weapon->GetPlayerId());
      if (weapon_player == nullptr) continue;
      if (weapon_player->frequency == processor_->GetGame().GetPlayer().frequency) continue;
      if (weapon->IsMine()) mines.push_back(weapon->GetPosition());
    }
    //#endif
    path_ = FindPath(processor_->GetGame().GetMap(), mines, from, to, radius);
  }

  return path_;
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

  // Use breadth first search to find the nearest node index.
 // use region registry to search through solid tiles that are not connected to the regions barrier
 // to get a more accurate result
size_t PathNodeSearch::FindNearestNodeBFS(const Vector2f& start) {

  // this may happen when targets die
  if (path.empty() || !IsValidPosition(start)) {
    return 0;
  }

  auto& registry = bot.GetRegions();

  MapCoord start_coord = start;

  queue.Clear();
  queue.Push(VisitState(start_coord, 0.0f));

  visited.reset();
  visited[start_coord.y * 1024 + start_coord.x] = 1;

  // Loop over full neighbor set to improve traversable tile lookups.
  // This isn't done on every iteration for performance. It would have to check a bunch of tiles that were already
  // visited.
  for (i16 y = -1; y <= 1; ++y) {
    for (i16 x = -1; x <= 1; ++x) {
      if (x == 0 && y == 0) continue;

      MapCoord check(start_coord.x + x, start_coord.y + y);

      if (!IsValidPosition(Vector2f(check.x, check.y))) continue;

      if (!visited[check.y * 1024 + check.x] && !registry.IsEdge(check)) {
        queue.Push(VisitState(check, 1.0f));
        visited[check.y * 1024 + check.x] = true;
      }
    }
  }

  while (!queue.Empty()) {
    // Grab the next tile from the queue
    VisitState* next = queue.Pop();

    if (next == nullptr) break;

    VisitState state = *next;
    const MapCoord& coord = state.coord;

    if (state.distance > search_range) continue;

    // Check if the current tile is within the path set and return that index if it is.
    if (path_set[coord.y * 1024 + coord.x]) {
      for (size_t i = 0; i < path.size(); ++i) {
        MapCoord check = path[i];

        if (check == coord) {
#if DEBUG_RENDER_PATHNODESEARCH
          Vector2f debug_pos = Vector2f(check.x, check.y);
          RenderWorldBox(bot.GetGame().GetPosition(), debug_pos - Vector2f(1, 1), debug_pos + Vector2f(1, 1),
                         RGB(0, 255, 0));
#endif
          return i;
        }
      }

      // Something broke if it got here. The coord in the path set should always be found.
      break;
    }

    const MapCoord west(coord.x - 1, coord.y);
    const MapCoord east(coord.x + 1, coord.y);
    const MapCoord north(coord.x, coord.y - 1);
    const MapCoord south(coord.x, coord.y + 1);

    // Check if each neighbor tile was visited and push it into the queue if it wasn't.

    if (IsValidPosition(Vector2f(west.x, west.y)) && !visited[west.y * 1024 + west.x] && !registry.IsEdge(west)) {
      queue.Push(VisitState(west, state.distance + 1.0f));
      visited[west.y * 1024 + west.x] = true;
    }

    if (IsValidPosition(Vector2f(east.x, east.y)) && !visited[east.y * 1024 + east.x] && !registry.IsEdge(east)) {
      queue.Push(VisitState(east, state.distance + 1.0f));
      visited[east.y * 1024 + east.x] = true;
    }

    if (IsValidPosition(Vector2f(north.x, north.y)) && !visited[north.y * 1024 + north.x] && !registry.IsEdge(north)) {
      queue.Push(VisitState(north, state.distance + 1.0f));
      visited[north.y * 1024 + north.x] = true;
    }

    if (IsValidPosition(Vector2f(south.x, south.y)) && !visited[south.y * 1024 + south.x] && !registry.IsEdge(south)) {
      queue.Push(VisitState(south, state.distance + 1.0f));
      visited[south.y * 1024 + south.x] = true;
    }
  }

  // This method failed, so get nearest node by distance.
  return FindNearestNodeByDistance(start);
}

std::size_t PathNodeSearch::FindNearestNodeByDistance(const Vector2f& position) const {
  float closest_distance_sq = std::numeric_limits<float>::max();
  std::size_t path_index = 0;

  for (std::size_t i = 0; i < path.size(); i++) {
    float distance_sq = position.DistanceSq(path[i]);

    if (closest_distance_sq > distance_sq) {
      path_index = i;
      closest_distance_sq = distance_sq;
    }
  }

  return path_index;
}
// Use the player position and path index to calculate the last path node the bot is 
// still in line of sight of.  Use edge raycast to ignore solids that arent a part 
// of the basees barrier.
Vector2f PathNodeSearch::LastLOSNode(Bot& bot, Vector2f position, std::size_t index, float radius, bool count_down) {
  // this function should never be used on an empty path so this return is useless if it ever happens
    if (path.empty()) return position;
    
  // return this if the loop fails its pretest (probably in a corner where it can't see any path nodes)
  Vector2f final_pos = path[index];
  // count_down is used to determine which direction to look in
  if (count_down) {
    for (std::size_t i = index; i > 0; i--) {
      Vector2f current = path[i];

      if (!RadiusEdgeRayCastHit(bot, position, current, radius)) {
        final_pos = current;
      } else {
        break;
      }
    }
  } else {
    for (std::size_t i = index; i < path.size(); i++) {
      Vector2f current = path[i];

      if (!RadiusEdgeRayCastHit(bot, position, current, radius)) {
        final_pos = current;
      } else {
        break;
      }
    }
  }
#if DEBUG_RENDER_PATHNODESEARCH
  RenderWorldBox(bot.GetGame().GetPosition(), final_pos - Vector2f(1, 1), final_pos + Vector2f(1, 1), RGB(0, 0, 255));
#endif
  return final_pos;
}

float PathNodeSearch::GetPathDistance(const Vector2f& pos1, const Vector2f& pos2) {
  size_t first_index = FindNearestNodeBFS(pos1);
  size_t second_index = FindNearestNodeBFS(pos2);

  return GetPathDistance(first_index, second_index);
}

  float PathNodeSearch::GetPathDistance(std::size_t index1, std::size_t index2) {
    if (index1 == index2) return 0.0f;

    float distance = 0.0f;

    size_t start = std::min(index1, index2);
    size_t end = std::max(index1, index2);

    for (size_t i = start; i < end; i++) {
      distance += path[i].Distance(path[i + 1]);
    }
    return distance;
  }

}  // namespace path
}  // namespace marvin
