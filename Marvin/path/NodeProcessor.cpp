#include "NodeProcessor.h"

#include "../Debug.h"

namespace marvin {
namespace path {

bool NodeProcessor::Mined(std::vector<Vector2f> mines, NodePoint point) {
  if (mines.empty()) {
    return false;
  }

  for (std::size_t i = 0; i < mines.size(); i++) {
    NodePoint np;
    np.x = (uint16_t)mines[i].x;
    np.y = (uint16_t)mines[i].y;

    if (np.x - 1 == point.x || np.x == point.x || np.x + 1 == point.x) {
      if (np.y - 1 == point.y || np.y == point.y || np.y + 1 == point.y) {
        return true;
      }
    }
  }

  return false;
}


NodeConnections NodeProcessor::FindEdges(std::vector<Vector2f> mines, Node* node, Node* start, Node* goal, float radius) {
  NodeConnections connections;
  connections.count = 0;

  NodePoint base_point = GetPoint(node);

  for (int y = -1; y <= 1; ++y) {
    for (int x = -1; x <= 1; ++x) {
      if (x == 0 && y == 0) {
        continue;
      }
      float radius_check = radius;

      uint16_t world_x = base_point.x + x;
      uint16_t world_y = base_point.y + y;

      if (map_.IsSolid(world_x, world_y)) {
        continue;
      }

      /* quick fix for diagonal walls separated by a single diagonal tile (CanOccupy will miss this), other option was 
      to disable this upper left neighbor */
      if (x == -1 && y == -1) {
        radius_check += 0.5f;
      }

      Vector2f check_pos(world_x, world_y);

      if (!map_.CanOccupy(check_pos, radius_check)) continue;

      NodePoint current_point(world_x, world_y);
      Node* current = GetNode(current_point);

      if (current) {
        if (Mined(mines, current_point)) {
          current->weight = 100.0f;
        } else if (map_.GetTileId(current_point.x, current_point.y) == kSafeTileId) {
          current->weight = 10.0f;
        } else if (current->weight != current->previous_weight) {
          current->weight = current->previous_weight;
        }
      }

      connections.neighbors[connections.count++] = current;

      if (connections.count >= 8) {
          return connections;
      }
    }
  }


  return connections;
}

Node* NodeProcessor::GetNode(NodePoint point) {
  if (point.x >= 1024 || point.y >= 1024) {
    return nullptr;
  }

  std::size_t index = point.y * 1024 + point.x;
  Node* node = &nodes_[index];

  if (!(node->flags & NodeFlag_Initialized)) {
    node->g = node->f = 0.0f;
    node->flags = NodeFlag_Initialized;
    node->parent = nullptr;
  }

  return &nodes_[index];
}

}  // namespace path
}  // namespace marvin
