#include "NodeProcessor.h"

#include "../Debug.h"

namespace marvin {
namespace path {

EdgeSet NodeProcessor::FindEdges(Node* node, float radius) {
  NodePoint point = GetPoint(node);
  size_t index = (size_t)point.y * 1024 + point.x;
  EdgeSet edges = this->edges_[index];

  // Any extra checks can be done here to remove dynamic edges.
  // if (west bad) edges.Erase(CoordOffset::WestIndex());

  #if 1
  if (node->parent) {
    // Don't cycle back to parent. This saves a very small amount of time because that node would be ignored anyway.
    NodePoint parent_point = GetPoint(node->parent);
    CoordOffset offset(parent_point.x - point.x, parent_point.y - point.y);

    edges.Erase(offset.GetIndex());
  }
#endif

  return edges;

  #if 0 
  NodeConnections connections;
  connections.count = 0;

  NodePoint base_point = GetPoint(node);
  MapCoord base(base_point.x, base_point.y);

  bool north = false;
  bool south = false;

  // start by looping through sides first

  Vector2f pos;
  std::vector<Vector2f> neighbors{North(pos), South(pos), West(pos), East(pos)};

  for (std::size_t i = 0; i < neighbors.size(); i++) {
    uint16_t world_x = base_point.x + (uint16_t)neighbors[i].x;
    uint16_t world_y = base_point.y + (uint16_t)neighbors[i].y;
    MapCoord current_pos(world_x, world_y);

    if (!map_.CanOccupyRadius(pos, radius)) {
      if (!map_.CanMoveTo(base, current_pos, radius)) {
        continue;
      }
    }

    NodePoint current_point(world_x, world_y);
    Node* current = GetNode(current_point);

    // if (!current) {
    //  continue;
    // }
    // if (!current->is_pathable) {
    // continue;
    // }
    // if (map_.IsMined(MapCoord(world_x, world_y))) {
    //  current->weight = 100.0f;
    // } else if (map_.GetTileId(current_point.x, current_point.y) == kSafeTileId) {
    //  current->weight = 10.0f;
    // } else if (current->weight != current->previous_weight) {
    //   current->weight = current->previous_weight;
    // }

    connections.neighbors[connections.count++] = current;

    if (connections.count >= 8) {
      return connections;
    }

    // if a side node gets pushed into the connections, check which node and add diagonal neighbors
    switch (i) {
      case 0: {
        north = true;
      } break;
      case 1: {
        south = true;
      } break;
      case 2: {
        if (north == true) {
          neighbors.push_back(NorthWest(pos));
        }
        if (south == true) {
          neighbors.push_back(SouthWest(pos));
        }
      } break;
      case 3: {
        if (north == true) {
          neighbors.push_back(NorthEast(pos));
        }
        if (south == true) {
          neighbors.push_back(SouthEast(pos));
        }
      } break;
    }
  }
  // }
  return connections;
  #endif
}

EdgeSet NodeProcessor::CalculateEdges(Node* node, float radius) {
  EdgeSet edges = {};

  NodePoint base_point = GetPoint(node);
  MapCoord base(base_point.x, base_point.y);

  bool north = false;
  bool south = false;

  bool* setters[8] = {&north, &south, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
  bool* requirements[8] = {nullptr, nullptr, nullptr, nullptr, &north, &north, &south, &south};
  static const CoordOffset neighbors[8] = {CoordOffset::North(),     CoordOffset::South(),     CoordOffset::West(),
                                           CoordOffset::East(),      CoordOffset::NorthWest(), CoordOffset::NorthEast(),
                                           CoordOffset::SouthWest(), CoordOffset::SouthEast()};

  for (std::size_t i = 0; i < 8; i++) {
    bool* requirement = requirements[i];

    if (requirement && !*requirement) continue;

    uint16_t world_x = base_point.x + neighbors[i].x;
    uint16_t world_y = base_point.y + neighbors[i].y;
    MapCoord pos(world_x, world_y);

    if (!map_.CanOccupyRadius(Vector2f(world_x, world_y), radius)) {
      if (!map_.CanMoveTo(base, pos, radius)) {
        continue;
      }
    }

    NodePoint current_point(world_x, world_y);
    Node* current = GetNode(current_point);

    if (!current) continue;
    if (!(current->flags & NodeFlag_Traversable)) continue;

    if (map_.GetTileId(current_point.x, current_point.y) == kSafeTileId) {
      current->weight = 10.0f;
    }

    edges.Set(i);

    if (setters[i]) {
      *setters[i] = true;
    }
  }

  return edges;
}

Node* NodeProcessor::GetNode(NodePoint point) {
  if (point.x >= 1024 || point.y >= 1024) {
    return nullptr;
  }

  std::size_t index = point.y * 1024 + point.x;
  Node* node = &nodes_[index];

  if (!(node->flags & NodeFlag_Initialized)) {
    node->g = node->f = node->f_last = 0.0f;
    node->flags = NodeFlag_Initialized | (node->flags & NodeFlag_Traversable);
    node->parent = nullptr;
  }

  return &nodes_[index];
}

}  // namespace path
}  // namespace marvin
