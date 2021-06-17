#pragma once

#include <unordered_map>
#include <vector>

#include "../GameProxy.h"
#include "../Map.h"
#include "Node.h"

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
    // NodeProcessor(const Map & map) : map_(map) {
    nodes_.resize(kMaxNodes);
  }

  GameProxy& GetGame() { return game_; }

  void ResetNodes();

  bool Mined(std::vector<Vector2f> mines, NodePoint point);

  // NodeConnections FindEdges(Node* node, Node* start, Node* goal);
  NodeConnections FindEdges(std::vector<Vector2f> mines, Node* node, Node* start, Node* goal, float radius);
  Node* GetNode(NodePoint point);
  bool IsSolid(u16 x, u16 y) { return map_.IsSolid(x, y); }

 private:
  std::vector<Node> nodes_;
  const Map& map_;
  GameProxy& game_;
};

}  // namespace path
}  // namespace marvin
