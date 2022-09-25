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

class Bot;

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

  const std::vector<Vector2f>& GetPath() { return path_; }
  void SetPath(std::vector<Vector2f> path) { path_ = path; }

  std::vector<Vector2f> SmoothPath(Bot& bot, const std::vector<Vector2f>& path, float ship_radius);

  std::vector<Vector2f> CreatePath(Bot& bot, Vector2f from, Vector2f to, float radius);

  void CreateMapWeights(const Map& map);
  void SetPathableNodes(const Map& map, float radius);
  void DebugUpdate(const Vector2f& position);

 private:
  float GetWallDistance(const Map& map, u16 x, u16 y, u16 radius);
  struct NodeCompare {
    bool operator()(const Node* lhs, const Node* rhs) const { return lhs->f > rhs->f; }
  };


  std::vector<Vector2f> path_;
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

  Bot& bot;
  CircularQueue<VisitState> queue;
  std::bitset<1024 * 1024> visited;
  std::bitset<1024 * 1024> path_set;
  size_t search_range;
  const std::vector<Vector2f>& path;

  static std::unique_ptr<PathNodeSearch> Create(Bot& bot, const std::vector<Vector2f>& path,
                                                size_t search_range) {
    return std::unique_ptr<PathNodeSearch>(new PathNodeSearch(bot, path, search_range));
  }

  size_t FindNearestNodeBFS(const Vector2f& start);

  std::size_t FindNearestNodeByDistance(const Vector2f& position) const;

 Vector2f FindLOSNode(Bot& bot, Vector2f position, std::size_t index, float radius, bool count_down);
 Vector2f FindForwardLOSNode(Bot& bot, Vector2f position, std::size_t index, float radius, bool high_side);
 Vector2f FindRearLOSNode(Bot& bot, Vector2f position, std::size_t index, float radius, bool high_side);

  float GetPathDistance(const Vector2f& pos1, const Vector2f& pos2);
  float GetPathDistance(std::size_t index1, std::size_t index2);

 private:
  // Private constructor to ensure it's allocated on the heap.
  PathNodeSearch(Bot& bot, const std::vector<Vector2f>& path, size_t search_range)
      : bot(bot), path(path), search_range(search_range), queue(GetQueueSize(search_range)) {
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
