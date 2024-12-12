#include "Pathfinder.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_set>
#include <immintrin.h>

#include "../Bot.h"
#include "../Debug.h"
#include "../RayCaster.h"
#include "..//RegionRegistry.h"

extern std::unique_ptr<marvin::Bot> bot;

/* when creating map weights, a single tile with a weight of 10 equals 10 tiles with a weight of 1,
or 5 tiles with a weight of 2, and so on. If a tile has a weight of 9, the pathfinder will need to step into 
9 tiles with a weight of 1 before stepping into that tile. */

namespace marvin {
namespace path {



inline NodePoint ToNodePoint(const Vector2f v) {
  NodePoint np;

  np.x = (uint16_t)v.x;
  np.y = (uint16_t)v.y;

  return np;
}

void Pathfinder::DebugUpdate(const Vector2f& position) {

  RenderWorldTile(position, position, RGB(200, 50, 50));

    Node* node = processor_->GetNode(ToNodePoint(position));
    EdgeSet edges = processor_->FindEdges(node);

       Vector2f neighbors[8] = {Vector2f(0, -1), Vector2f(0, 1), Vector2f(-1, 0),  Vector2f(1, 0),
                             Vector2f(-1, -1), Vector2f(1, -1), Vector2f(-1, 1), Vector2f(1, 1)};

  for (std::size_t i = 0; i < 8; ++i) {
      if (!edges.IsSet(i)) continue;

      Vector2f check = position + neighbors[i];

    RenderWorldTile(position, check, RGB(255, 255, 255));
   }

}



inline float Euclidean(NodeProcessor& processor, const Node* __restrict from, const Node* __restrict to) {
  NodePoint from_p = processor.GetPoint(from);
  NodePoint to_p = processor.GetPoint(to);

  float dx = static_cast<float>(from_p.x - to_p.x);
  float dy = static_cast<float>(from_p.y - to_p.y);

  return sqrt(dx * dx + dy * dy);
}

inline float Euclidean(const NodePoint& __restrict from_p, const NodePoint& __restrict to_p) {
  float dx = static_cast<float>(from_p.x - to_p.x);
  float dy = static_cast<float>(from_p.y - to_p.y);

  __m128 mult = _mm_set_ss(dx * dx + dy * dy);
  __m128 result = _mm_sqrt_ss(mult);

  return _mm_cvtss_f32(result);
}

Pathfinder::Pathfinder(std::unique_ptr<NodeProcessor> processor, RegionRegistry& regions)
    : processor_(std::move(processor)), regions_(regions) {}

const std::vector<Vector2f>& Pathfinder::FindPath(const Map& map, Vector2f from, Vector2f to, float radius) {
  //std::vector<Vector2f> path;
  path_.clear();

  if (!regions_.IsConnected(from, to)) {
    return path_;
  }

  if (!map.CanOverlapTile(from, radius)) {
        from = GetPathableNeighbor(map, regions_, from, radius);
  }
  if (!map.CanOverlapTile(to, radius)) {
        to = GetPathableNeighbor(map, regions_, to, radius);
  }

  Node* start = processor_->GetNode(ToNodePoint(from));
  Node* goal = processor_->GetNode(ToNodePoint(to));

  if (start == nullptr || goal == nullptr) {
        return path_;
  }

   // TODO: need a case for when the start node is a diagonal gap
  // which side of the gap does the bot start on?
  // need to look at bots position relative to the tile its currently on
  if (!(start->flags & NodeFlag_Traversable)) return path_;
  if (!(goal->flags & NodeFlag_Traversable)) return path_;

  NodePoint start_p = processor_->GetPoint(start);
  NodePoint goal_p = processor_->GetPoint(goal);

  #if 0 
  // Clear the touched nodes before pathfinding.
  for (Node* node : touched_nodes_) {
    // Setting the flag to zero causes GetNode to reset the node on next fetch.
    node->flags = 0;
  }
  touched_nodes_.clear();
  #endif

  // clear vector then add start node
  openset_.Clear();
  openset_.Push(start);

  //touched_nodes_.insert(start);
 // touched_nodes_.insert(goal);

  // at the start there is only one node here, the start node
  while (!openset_.Empty()) {
    // grab front item then delete it
    Node* node = openset_.Pop();

    //touched_nodes_.insert(node);
    touched_.push_back(node);

    if (node == goal) {
      break;
    }

    node->flags |= NodeFlag_Closed;

    // returns neighbor nodes that are not solid
   // NodeConnections connections = processor_->FindEdges(node, radius);
    //return path_;

    if (node->f > 0 && node->f == node->f_last) {
      // This node was re-added to the openset because its fitness was better, so skip reprocessing the same node.
      // This reduces pathing time by about 20%.
      continue;
    }
    node->f_last = node->f;

    NodePoint node_point = processor_->GetPoint(node);

    // returns neighbor nodes that are not solid
    EdgeSet edges = processor_->FindEdges(node);

    for (std::size_t i = 0; i < 8; ++i) {
      if (!edges.IsSet(i)) continue;

      CoordOffset offset = CoordOffset::FromIndex(i);

      NodePoint edge_point(node_point.x + offset.x, node_point.y + offset.y);
      Node* edge = processor_->GetNode(edge_point);
      float weight = edge->weight;

      if (map.IsMined(MapCoord(edge_point.x, edge_point.y))) {
        weight += 10.0f;
      }

      touched_.push_back(edge);

      // The cost to this neighbor is the cost to the current node plus the edge weight times the distance between the
      // nodes.
      float cost = node->g + weight * Euclidean(node_point, edge_point);

      // If the new cost is lower than the previously closed cost then remove it from the closed set.
      if ((edge->flags & NodeFlag_Closed) && cost < edge->g) {
        edge->flags &= ~NodeFlag_Closed;
      }

      // Compute a heuristic from this neighbor to the end goal.
      float h = Euclidean(edge_point, goal_p);

      // If this neighbor hasn't been considered or is better than its original fitness test, then add it back to the
      // open set.
      if (!(edge->flags & NodeFlag_Openset) || cost + h < edge->f) {
        edge->g = cost;
        edge->f = edge->g + h;
        edge->parent = node;
        edge->flags |= NodeFlag_Openset;

        openset_.Push(edge);
      }
    }
  }

  // Construct path backwards from goal node
  std::vector<NodePoint> points;
  Node* current = goal;

  while (current != nullptr && current != start) {
    NodePoint p = processor_->GetPoint(current);
    points.push_back(p);
    current = current->parent;
  }

  path_.reserve(points.size() + 1);

  if (goal->parent) {
    path_.push_back(map.GetOccupyCenter(Vector2f(start_p.x, start_p.y), radius));
   // path_.push_back(Vector2f(start_p.x + 0.5f, start_p.y + 0.5f));
  }

  // Reverse and store as vector
  for (std::size_t i = 0; i < points.size(); ++i) {
    std::size_t index = points.size() - i - 1;
    //Vector2f pos(points[index].x + 0.5f, points[index].y + 0.5f);
    Vector2f pos(points[index].x, points[index].y);

    //OccupyRect o_rect = map.GetPossibleOccupyRect(pos, radius);
    //path.push_back(pos);
    //if (!o_rect.occupy || map.CanOccupyRadius(Vector2f(points[index].x, points[index].y), radius)) {
    // if (!o_rect.occupy) {
    //  path_.push_back(pos);
  //  } else {
   //   Vector2f min(o_rect.start_x, o_rect.start_y);
   //   Vector2f max((float)o_rect.end_x + 1.0f, (float)o_rect.end_y + 1.0f);

   //   path_.push_back((min + max) * 0.5f);
   // }
    pos = map.GetOccupyCenter(pos, radius);

    path_.push_back(pos);
    
  }

  for (Node* node : touched_) {
    node->flags &= ~NodeFlag_Initialized;
  }

  touched_.clear();

  return path_;
}

#if 0
bool Pathfinder::InTube(const Map& map, Vector2f position, float radius) {
  for (float x = -1.0f; x <= 1.0f; x++) {
    for (float y = -1.0f; y <= 1.0f; y++) {
      Vector2f check(position.x + x, position.y + y);
      if (CrossCheck(map, check, radius)) {
        return true;
      }
    }
  }
  return false;
}

bool Pathfinder::CrossCheck(const Map& map, Vector2f position, float radius) {
  bool north = false;
  bool south = false;
  bool west = false;
  bool east = false;

  // snap pos to nearest tile
  MapCoord pos = ToNearestTile(position);

  uint16_t check_distance = uint16_t(radius + 0.5f) + 1;
 
  for (uint16_t i = 1; i <= check_distance; i++) {
    // check pos is already on this tile so ignore first pass
    if (map.IsSolid(MapCoord(pos.x, pos.y - (i - 1)))) {
      north = true;
    } 
    if (map.IsSolid(MapCoord(pos.x, pos.y + i))) {
      south = true;
    }
    if (map.IsSolid(MapCoord(pos.x + i, pos.y))) {
      east = true;
    }
    // check pos is already on this tile so ignore first pass
    if (map.IsSolid(MapCoord(pos.x - (i - 1), pos.y))) {
      west = true;
    }
  }
  if ((north && south) || (east && west)) {
    return true;
  }
  return false;
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

#endif

const std::vector<Vector2f>& Pathfinder::CreatePath(Bot& bot, Vector2f from, Vector2f to, float radius) {
  bool build = true;

  if (!path_.empty()) {
    // Check if the current destination is the same as the requested one.
    if (path_.back().DistanceSq(to) < 3 * 3) {
      Vector2f pos = bot.GetGame().GetPosition();
      Vector2f next = path_.front();

      // diameter cast causes a lot of rebuilding radius seems good
      // this is influenced by how the follow path node culls line of sight nodes
      bool hit = DiameterRayCastHit(bot, pos, next, radius);

      // Rebuild the path if the bot isn't in line of sight of its next node.
      if (!hit) {
        build = false;
      }
    }
  }
  

  if (build) {
    FindPath(bot.GetGame().GetMap(), from, to, radius);
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

void Pathfinder::SetTraversabletiles(const Map& map, float radius) {
  OccupiedRect* scratch_rects = new OccupiedRect[256];

  // Calculate which nodes are traversable before creating edges.
  for (u16 y = 0; y < 1024; ++y) {
    for (u16 x = 0; x < 1024; ++x) {
      if (map.IsSolid(x, y)) continue;

      Node* node = processor_->GetNode(NodePoint(x, y));

      if (map.CanOverlapTile(Vector2f(x, y), radius)) {
        node->flags |= NodeFlag_Traversable;

        size_t rect_count = map.GetAllOccupiedRects(Vector2f(x, y), radius, scratch_rects);

        // TODO: test this for larger radius using elm
        // This might be a diagonal tile
        if (rect_count == 2) {
          // Check if the two occupied rects are offset on both axes.
          if (scratch_rects[0].start_x != scratch_rects[1].start_x &&
              scratch_rects[0].start_y != scratch_rects[1].start_y) {
            // This is a diagonal-only tile, so skip it.
            node->flags |= NodeFlag_DiagonalGap;
          }
        }
      }
    }
  }
 delete[] scratch_rects;
}

void Pathfinder::CalculateEdges(const Map& map, float radius) {
 for (u16 y = 0; y < 1024; ++y) {
    for (u16 x = 0; x < 1024; ++x) {
      if (map.IsSolid(x, y)) continue;

      Node* node = this->processor_->GetNode(NodePoint(x, y));
      EdgeSet edges = processor_->CalculateEdges(node, radius);
      processor_->SetEdgeSet(x, y, edges);
    }
 }
}

void Pathfinder::SetMapWeights(const Map& map, float radius) {

    int int_radius = int(radius + 0.5f);

 for (u16 y = 0; y < 1024; ++y) {
    for (u16 x = 0; x < 1024; ++x) {
      if (map.IsSolid(x, y)) continue;

      Node* node = this->processor_->GetNode(NodePoint(x, y));
      node->weight = 1.0f;

      int close_distance = 5 + int_radius;
      float distance = GetWallDistance(map, x, y, close_distance);

      if (distance < 1) distance = 1;

      if (distance <= int_radius) {
        node->weight = 100.0f;
      } else if (distance < close_distance) {
        node->weight = close_distance / distance;
      }
    }
 }
}

Vector2f Pathfinder::GetPathableNeighbor(const Map& map, RegionRegistry& regions, Vector2f position, float radius) {
  
  Vector2f check_pos = position;
  int check_diameter = (int)((radius + 0.5f) * 2) + 1;

  // start in the center and search outward
  for (int i = 1; i < check_diameter; i++) {
    for (int y = -i; y <= i; y++) {
      for (int x = -i; x <= i; x++) {
        check_pos = Vector2f(position.x + (float)x, position.y + (float)y);
        if (map.CanOverlapTile(check_pos, radius) && regions.IsConnected(position, check_pos)) {
          return check_pos;
        }
      }
    }
  }
  return check_pos;
}

#if 0
void Pathfinder::SetPathableNodes(const Map& map, float radius) {
  for (u16 y = 0; y < 1024; ++y) {
    for (u16 x = 0; x < 1024; ++x) {

      if (map.IsSolid(x, y)) continue;

      Node* node = this->processor_->GetNode(NodePoint(x, y));

      if (map.CanPathOn(Vector2f(x, y), radius)) {
       // node->is_pathable = true;
      }
    }
  }
}
#endif

  // Use breadth first search to find the nearest node index.
 // use region registry to search through solid tiles that are not connected to the regions barrier
 // to get a more accurate result
size_t PathNodeSearch::FindNearestNodeBFS(const Vector2f& start) {

  // this may happen when targets die
  if (path.empty() || !IsValidPosition(start)) {
    return -1;
  }

  auto& registry = bot.GetRegions();

  if (!registry.IsConnected(start, path[0])) return -1;

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

    if (!registry.IsConnected(coord, start_coord)) {
      continue;
    }
    //if (state.distance > search_range) continue;

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

Vector2f PathNodeSearch::FindForwardLOSNode(Vector2f position, std::size_t index, float radius, bool high_side) {
  if (high_side) {
   return FindLOSNode(position, index, radius, true);
  } else {
    return FindLOSNode(position, index, radius, false);
  }
}

Vector2f PathNodeSearch::FindRearLOSNode(Vector2f position, std::size_t index, float radius, bool high_side) {
  if (high_side) {
    return FindLOSNode(position, index, radius, false);
  } else {
    return FindLOSNode(position, index, radius, true);
  }
}

  // Use the player position and path index to calculate the last path node the bot is 
// still in line of sight of.  Use edge raycast to ignore solids that arent a part 
// of the basees barrier.
Vector2f PathNodeSearch::FindLOSNode(Vector2f position, std::size_t index, float radius, bool count_down) {
  // this function should never be used on an empty path so this return is useless if it ever happens
  if (path.empty()) return position;
  if (index >= path.size()) return position;
    
  // return this if the loop fails its pretest (probably in a corner where it can't see any path nodes)
  Vector2f final_pos = path[index];
  // count_down is used to determine which direction to look in
  if (count_down) {
      // use signed here so i can be less than 0 and stop the loop (unsigned cant be negative)
    for (int i = (int)index; i >= 0; i--) {
      Vector2f current = path[std::size_t(i)];

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
    
    float max = FLT_MAX;

    if (path.empty()) return max;
    if (index1 >= path.size() || index2 >= path.size()) return max;
      
    if (index1 == index2) return 0.0f;

    float distance = 0.0f;

    size_t start = std::min(index1, index2);
    size_t end = std::max(index1, index2);

    for (size_t i = start; i < end; i++) {
      distance += path[i].Distance(path[i + 1]);
    }
    return distance;
  }

  std::unique_ptr<PathNodeSearch> PathNodeSearch::Create(Bot& bot, const std::vector<Vector2f>& path) {
    if (path.empty()) {
      return nullptr;
    }

    std::size_t size = bot.GetRegions().GetTileCount(path[0]) + 1;
    return std::unique_ptr<PathNodeSearch>(new PathNodeSearch(bot, path, size));
  }

}  // namespace path
}  // namespace marvin
