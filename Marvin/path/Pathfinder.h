#pragma once

#include <algorithm>
#include <bitset>
#include <memory>
#include <unordered_set>
#include <vector>

#include "../Vector2f.h"
#include "NodeProcessor.h"
#include "Path.h"

namespace marvin {

float PathLength(std::vector<Vector2f> path, Vector2f pos1, Vector2f pos2);
std::size_t FindPathIndex(std::vector<Vector2f> path, Vector2f position);
Vector2f LastLOSNode(const Map& map, std::size_t index, bool count_down, std::vector<Vector2f> path, float radius);

namespace path {

template <typename T, typename Compare, typename Container = std::vector<T>>

class PriorityQueue {
 public:
  using const_iterator = typename Container::const_iterator;

  const_iterator begin() const { return container_.cbegin(); }
  const_iterator end() const { return container_.cend(); }

  void Push(T item) {
    container_.push_back(item);
    std::push_heap(container_.begin(), container_.end(), comparator_);
  }

  T Pop() {
    T item = container_.front();
    std::pop_heap(container_.begin(), container_.end(), comparator_);
    container_.pop_back();
    return item;
  }

  // sort from highest at beginning to lowest at end
  void Update() { std::make_heap(container_.begin(), container_.end(), comparator_); }

  void Clear() { container_.clear(); }
  std::size_t Size() const { return container_.size(); }
  bool Empty() const { return container_.empty(); }

 private:
  Container container_;
  Compare comparator_;
};

struct Pathfinder {
 public:
  Pathfinder(std::unique_ptr<NodeProcessor> processor, RegionRegistry& regions);
  std::vector<Vector2f> FindPath(const Map& map, const std::vector<Vector2f>& mines, const Vector2f& from, const Vector2f& to,
                                 float radius);

  std::vector<Vector2f> SmoothPath(const std::vector<Vector2f>& path, const Map& map, float ship_radius);

  std::vector<Vector2f> CreatePath(std::vector<Vector2f> path, Vector2f from, Vector2f to, float radius);

  void CreateMapWeights(const Map& map);
  void SetPathableNodes(const Map& map, float radius);
  void DebugUpdate(const Vector2f& position);

 private:
  float GetWallDistance(const Map& map, u16 x, u16 y, u16 radius);
  struct NodeCompare {
    bool operator()(const Node* lhs, const Node* rhs) const { return lhs->f > rhs->f; }
  };

  std::unique_ptr<NodeProcessor> processor_;
  RegionRegistry& regions_;
  PriorityQueue<Node*, NodeCompare> openset_;
  std::unordered_set<Node*> touched_nodes_;
};

template <typename T>
struct CircularQueue {
  T* buffer;
  size_t write_index = 0;
  size_t read_index = 0;
  size_t max_size;
  size_t count = 0;

  CircularQueue(size_t max_size) : max_size(max_size) { buffer = new T[max_size]; }
  ~CircularQueue() { delete[] buffer; }

  inline void Clear() {
    write_index = read_index = 0;
    count = 0;
  }

  inline bool Empty() const { return count == 0; }

  T* Pop() {
    if (count == 0) return nullptr;
    if (read_index == write_index) return nullptr;

    --count;

    if (read_index < write_index) {
      return buffer + read_index++;
    }

    T* result = buffer + read_index++;

    if (read_index >= max_size) {
      read_index = 0;
    }

    return result;
  }

  bool Push(const T& state) {
    if (write_index < read_index) {
      // There's no room in the queue
      if (read_index - write_index <= 1) return false;

      ++count;
      buffer[write_index++] = state;
      return true;
    } else if (write_index > read_index) {
      if (write_index + 1 >= max_size) {
        // If write index hits max size and read_index is 0, then there's no room left in the queue.
        if (read_index == 0) return false;

        buffer[write_index] = state;
        write_index = 0;
      } else {
        buffer[write_index++] = state;
      }

      ++count;

      return true;
    }

    ++count;
    buffer[write_index++] = state;

    return true;
  }
};

struct PathNodeSearch {
  struct VisitState {
    MapCoord coord;
    float distance;

    VisitState() : coord(0, 0), distance(0.0f) {}
    VisitState(MapCoord coord, float distance) : coord(coord), distance(distance) {}
  };

  const Map& map;
  CircularQueue<VisitState> queue;
  std::bitset<1024 * 1024> visited;
  std::bitset<1024 * 1024> path_set;
  size_t search_range;
  const std::vector<Vector2f>& path;

  static std::unique_ptr<PathNodeSearch> Create(const Map& map, const std::vector<Vector2f>& path,
                                                size_t search_range) {
    return std::unique_ptr<PathNodeSearch>(new PathNodeSearch(map, path, search_range));
  }

  // Use breadth first search to find the nearest node index.
  size_t FindNearestNodeBFS(const Vector2f& start, float ship_radius) {
    if (path.empty()) return 0;

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

        if (!visited[check.y * 1024 + check.x] && map.CanOccupy(Vector2f(check.x, check.y), ship_radius)) {
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

      if (!visited[west.y * 1024 + west.x] && map.CanOccupy(Vector2f(west.x, west.y), ship_radius)) {
        queue.Push(VisitState(west, state.distance + 1.0f));
        visited[west.y * 1024 + west.x] = true;
      }

      if (!visited[east.y * 1024 + east.x] && map.CanOccupy(Vector2f(east.x, east.y), ship_radius)) {
        queue.Push(VisitState(east, state.distance + 1.0f));
        visited[east.y * 1024 + east.x] = true;
      }

      if (!visited[north.y * 1024 + north.x] && map.CanOccupy(Vector2f(north.x, north.y), ship_radius)) {
        queue.Push(VisitState(north, state.distance + 1.0f));
        visited[north.y * 1024 + north.x] = true;
      }

      if (!visited[south.y * 1024 + south.x] && map.CanOccupy(Vector2f(south.x, south.y), ship_radius)) {
        queue.Push(VisitState(south, state.distance + 1.0f));
        visited[south.y * 1024 + south.x] = true;
      }
    }

    // This method failed, so get nearest node by distance.
    return FindNearestNodeByDistance(start);
  }

  std::size_t FindNearestNodeByDistance(const Vector2f& position) const {
    float closest_distance_sq = std::numeric_limits<float>::max();
    std::size_t path_index = 0;

    if (path.empty()) {
      return path_index;
    }

    for (std::size_t i = 0; i < path.size(); i++) {
      float distance_sq = position.DistanceSq(path[i]);

      if (closest_distance_sq > distance_sq) {
        path_index = i;
        closest_distance_sq = distance_sq;
      }
    }

    return path_index;
  }

  float GetPathDistance(const Vector2f& pos1, const Vector2f& pos2) {
    size_t first_index = FindNearestNodeBFS(pos1, 14.0f / 16.0f);
    size_t second_index = FindNearestNodeBFS(pos2, 14.0f / 16.0f);

    if (first_index == second_index) return 0.0f;

    float distance = 0.0f;

    size_t start = std::min(first_index, second_index);
    size_t end = std::max(first_index, second_index);

    for (size_t i = start; i < end; i++) {
      distance += path[i].Distance(path[i + 1]);
    }

    return distance;
  }

 private:
  // Private constructor to ensure it's allocated on the heap.
  PathNodeSearch(const Map& map, const std::vector<Vector2f>& path, size_t search_range)
      : map(map), path(path), search_range(search_range), queue(GetQueueSize(search_range)) {
    for (MapCoord coord : path) {
      path_set[coord.y * 1024 + coord.x] = 1;
    }
  }

  static inline size_t GetQueueSize(size_t search_range) {
    size_t d = (search_range * 2);
    return d * d + 1;
  }
};

}  // namespace path
}  // namespace marvin
