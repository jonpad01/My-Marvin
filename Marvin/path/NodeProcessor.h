#pragma once

#include <cstring>
#include <unordered_map>
#include <vector>

#include "../GameProxy.h"
#include "../Map.h"
#include "Node.h"
#include "../RegionRegistry.h"

namespace marvin {
namespace path {

constexpr std::size_t kMaxNodes = 1024 * 1024;

struct NodeConnections {
  Node* neighbors[8];
  std::size_t count;
};

// Determines the node edges when using A*.
class NodeProcessor {
 public:
  NodeProcessor(GameProxy& game) : game_(game), map_(game.GetMap()) {
    nodes_.resize(kMaxNodes);
    memset(&nodes_[0], 0, kMaxNodes * sizeof(Node));
  }

  GameProxy& GetGame() { return game_; }

  NodeConnections FindEdges(Node* node, float radius);
  Node* GetNode(NodePoint point);
  bool IsSolid(u16 x, u16 y) { return map_.IsSolid(x, y); }

  // Calculate the node from the index.
  // This lets the node exist without storing its position so it fits in cache better.
  inline NodePoint GetPoint(const Node* node) const {
    size_t index = (node - &nodes_[0]);

    uint16_t world_y = (uint16_t)(index / 1024);
    uint16_t world_x = (uint16_t)(index % 1024);

    return NodePoint(world_x, world_y);
  }

 private:
  std::vector<Node> nodes_;
  const Map& map_;
  GameProxy& game_;
};

}  // namespace path
}  // namespace marvin
