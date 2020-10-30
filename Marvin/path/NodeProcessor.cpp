#include "NodeProcessor.h"

namespace marvin {
namespace path {

    bool NodeProcessor::Mined(std::vector<Vector2f> mines, NodePoint point) {
        if (mines.empty()) { return false; }

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

//NodeConnections NodeProcessor::FindEdges(Node* node, Node* start, Node* goal) {
    NodeConnections NodeProcessor::FindEdges(std::vector<Vector2f> mines, Node* node, Node* start, Node* goal, float radius) {
        NodeConnections connections;
        connections.count = 0;

        for (int y = -1; y <= 1; ++y) {
            for (int x = -1; x <= 1; ++x) {
                if (x == 0 && y == 0) { continue; }

                uint16_t world_x = node->point.x + x;
                uint16_t world_y = node->point.y + y;

                if (map_.IsSolid(world_x, world_y)) { continue; }

                Vector2f check_pos(world_x + 0.5f, world_y + 0.5f);
                //Vector2f check_pos(world_x, world_y);


                if (!map_.CanOccupy(check_pos, radius)) continue;

                NodePoint point(world_x, world_y);
                Node* current = GetNode(point);

                if (current != nullptr) {
                    if (Mined(mines, point)) {
                        current->weight = 100.0f;
                    }
                    else if (map_.GetTileId(point.x, point.y) == kSafeTileId) {
                        current->weight = 10.0f;
                    }
                    else {
                        current->weight = 0.0f;
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

void NodeProcessor::ResetNodes() {
  for (std::size_t i = 0; i < 1024 * 1024; ++i) {
    Node* node = &nodes_[i];

    node->closed = false;
    node->openset = false;
    node->g = node->h = node->f = 0;
    node->parent = nullptr;
  }
}

Node* NodeProcessor::GetNode(NodePoint point) {
  if (point.x >= 1024 || point.y >= 1024) {
    return nullptr;
  }

  std::size_t index = point.y * 1024 + point.x;
  Node* node = &nodes_[index];

  if (node->point.x != point.x) {
    node->point = point;
    node->g = node->h = node->f = 0;
    node->closed = false;
    node->openset = false;
    node->parent = nullptr;
  }

  return &nodes_[index];
}

}  // namespace path
}  // namespace marvin

