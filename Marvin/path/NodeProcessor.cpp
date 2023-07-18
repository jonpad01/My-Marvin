#include "NodeProcessor.h"

#include "../Debug.h"

namespace marvin {
namespace path {

    enum class Neighbors : std::size_t { North, South, West, East, NorthWest, NorthEast, SouthWest, SouthEast, None };

Neighbors SolidEastOrWest(const Map& map, MapCoord pos) {
  MapCoord east = MapCoord(pos.x + 1, pos.y);
  MapCoord west = MapCoord(pos.x - 1, pos.y);

  if (map.IsSolid(east)) {
    return Neighbors::East;
  } else if (map.IsSolid(west)) {
    return Neighbors::West;
  }
  return Neighbors::None;
}

Neighbors SolidNorthOrSouth(const Map& map, MapCoord pos) {
  MapCoord north = MapCoord(pos.x, pos.y - 1);
  MapCoord south = MapCoord(pos.x, pos.y + 1);

  if (map.IsSolid(north)) {
    return Neighbors::North;
  } else if (map.IsSolid(south)) {
    return Neighbors::South;
  }
  return Neighbors::None;
}

Neighbors GetDiagonalNeighbor(Neighbors one, Neighbors two) {
  switch (one) {
    case Neighbors::North: {
      if (two == Neighbors::East) {
        return Neighbors::NorthEast;
      } else {
        return Neighbors::NorthWest;
      }
      break;
    }
    case Neighbors::South: {
      if (two == Neighbors::East) {
        return Neighbors::SouthEast;
      } else {
        return Neighbors::SouthWest;
      }
      break;
    }
    case Neighbors::East: {
      if (two == Neighbors::North) {
        return Neighbors::NorthEast;
      } else {
        return Neighbors::SouthEast;
      }
      break;
    }
    case Neighbors::West: {
      if (two == Neighbors::North) {
        return Neighbors::NorthWest;
      } else {
        return Neighbors::SouthWest;
      }
      break;
    }
  }
  return Neighbors::None;
}

    inline bool CanOccupy(const Map& map, OccupiedRect& rect, Vector2f offset) {
  Vector2f min = Vector2f(rect.start_x, rect.start_y) + offset;
  Vector2f max = Vector2f(rect.end_x, rect.end_y) + offset;

  for (u16 y = (u16)min.y; y <= (u16)max.y; ++y) {
    for (u16 x = (u16)min.x; x <= (u16)max.x; ++x) {
      if (map.IsSolid(x, y)) {
        return false;
      }
    }
  }

  return true;
}

inline bool CanOccupyAxis(const Map& map, OccupiedRect& rect, Vector2f offset) {
  Vector2f min = Vector2f(rect.start_x, rect.start_y) + offset;
  Vector2f max = Vector2f(rect.end_x, rect.end_y) + offset;

  if (offset.x < 0) {
    // Moving west, so check western section of rect
    for (u16 y = (u16)min.y; y <= (u16)max.y; ++y) {
      if (map.IsSolid((u16)min.x, y)) {
        return false;
      }
    }
  } else if (offset.x > 0) {
    // Moving east, so check eastern section of rect
    for (u16 y = (u16)min.y; y <= (u16)max.y; ++y) {
      if (map.IsSolid((u16)max.x, y)) {
        return false;
      }
    }
  } else if (offset.y < 0) {
    // Moving north, so check north section of rect
    for (u16 x = (u16)min.x; x <= (u16)max.x; ++x) {
      if (map.IsSolid(x, (u16)min.y)) {
        return false;
      }
    }
  } else if (offset.y > 0) {
    // Moving south, so check south section of rect
    for (u16 x = (u16)min.x; x <= (u16)max.x; ++x) {
      if (map.IsSolid(x, (u16)max.y)) {
        return false;
      }
    }
  }

  return true;
}

EdgeSet NodeProcessor::FindEdges(Node* node) {
  NodePoint point = GetPoint(node);
  size_t index = (size_t)point.y * 1024 + point.x;
  EdgeSet edges = this->edges_[index];

  // Any extra checks can be done here to remove dynamic edges.
  // if (west bad) edges.Erase(CoordOffset::WestIndex());

  // current node is in a diagonal gap
  // so look at previous node to determine which edges are not valid
  // TODO: SolidEastOrWest does not expand back for ship radius
  // currently only works for 2 tiles ships
  if (node->parent && (node->flags & NodeFlag_DiagonalGap)) {
    NodePoint prev_point = GetPoint(node->parent);
    CoordOffset dir = CoordOffset(point.x - prev_point.x, point.y - prev_point.y);

    for (std::size_t i = 0; i < 8; ++i) {
      if (!edges.IsSet(i)) continue;
      CoordOffset offset = CoordOffset::FromIndex(i);
      if (dir.x == offset.x && dir.y == offset.y) {
        switch ((Neighbors)i) {
          case Neighbors::North: {
            edges.Erase((std::size_t)Neighbors::North);
            Neighbors other = SolidEastOrWest(map_, Vector2f(prev_point.x, prev_point.y));
            edges.Erase((std::size_t)other);
            edges.Erase((std::size_t)GetDiagonalNeighbor(Neighbors::North, other));
            break;
          }
          case Neighbors::South: {
            edges.Erase((std::size_t)Neighbors::South);
            Neighbors other = SolidEastOrWest(map_, Vector2f(prev_point.x, prev_point.y));
            edges.Erase((std::size_t)other);
            edges.Erase((std::size_t)GetDiagonalNeighbor(Neighbors::South, other));
            break;
          }
          case Neighbors::East: {
            edges.Erase((std::size_t)Neighbors::East);
            Neighbors other = SolidNorthOrSouth(map_, Vector2f(prev_point.x, prev_point.y));
            edges.Erase((std::size_t)other);
            edges.Erase((std::size_t)GetDiagonalNeighbor(Neighbors::East, other));
            break;
          }
          case Neighbors::West: {
            edges.Erase((std::size_t)Neighbors::West);
            Neighbors other = SolidNorthOrSouth(map_, Vector2f(prev_point.x, prev_point.y));
            edges.Erase((std::size_t)other);
            edges.Erase((std::size_t)GetDiagonalNeighbor(Neighbors::West, other));
            break;
          }
          case Neighbors::NorthWest: {
            edges.Erase((std::size_t)Neighbors::North);
            edges.Erase((std::size_t)Neighbors::West);
            edges.Erase((std::size_t)Neighbors::NorthWest);
            break;
          }
          case Neighbors::NorthEast: {
            edges.Erase((std::size_t)Neighbors::North);
            edges.Erase((std::size_t)Neighbors::East);
            edges.Erase((std::size_t)Neighbors::NorthEast);
            break;
          }
          case Neighbors::SouthWest: {
            edges.Erase((std::size_t)Neighbors::South);
            edges.Erase((std::size_t)Neighbors::West);
            edges.Erase((std::size_t)Neighbors::SouthWest);
            break;
          }
          case Neighbors::SouthEast: {
            edges.Erase((std::size_t)Neighbors::South);
            edges.Erase((std::size_t)Neighbors::East);
            edges.Erase((std::size_t)Neighbors::SouthEast);
            break;
          }
        }
      }
    }
  }

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
  bool constricted = false;

  bool* setters[8] = {&north, &south, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
  bool* requirements[8] = {nullptr, nullptr, nullptr, nullptr, &north, &north, &south, &south};
  static const CoordOffset neighbors[8] = {CoordOffset::North(),     CoordOffset::South(),     CoordOffset::West(),
                                           CoordOffset::East(),      CoordOffset::NorthWest(), CoordOffset::NorthEast(),
                                           CoordOffset::SouthWest(), CoordOffset::SouthEast()};

  OccupiedRect occupied[64];
  size_t occupied_count = map_.GetAllOccupiedRects(Vector2f(base.x, base.y), radius, occupied);

  for (std::size_t i = 0; i < 8; i++) {
    bool* requirement = requirements[i];

    // ignore diagonal neighbors in tight spaces
    // if (i > 3) continue;
    if (requirement && !*requirement) continue;

    uint16_t world_x = base_point.x + neighbors[i].x;
    uint16_t world_y = base_point.y + neighbors[i].y;
    MapCoord pos(world_x, world_y);

    // if (!map_.CanOccupyRadius(Vector2f(world_x, world_y), radius)) {
    // constricted = true;
    // if (!map_.CanMoveTo(base, pos, radius)) {
    bool is_occupied = false;
    // Check each occupied rect to see if contains the target position.
    // The expensive check can be skipped because this spot is definitely occupiable.
    for (size_t j = 0; j < occupied_count; ++j) {
      OccupiedRect& rect = occupied[j];

      if (rect.Contains(Vector2f(pos.x, pos.y))) {
        is_occupied = true;
        break;
      }
    }

    if (!is_occupied) {
      continue;
    }

    NodePoint current_point(world_x, world_y);
    Node* current = GetNode(current_point);

    if (!current) continue;
   // if (!(current->flags & NodeFlag_Traversable)) continue;

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
    node->flags = NodeFlag_Initialized | (node->flags & NodeFlag_Traversable) | (node->flags & NodeFlag_DiagonalGap);
    node->parent = nullptr;
  }

  return &nodes_[index];
}

}  // namespace path
}  // namespace marvin
