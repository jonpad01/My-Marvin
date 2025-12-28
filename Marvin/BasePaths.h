#pragma once
#include <vector>
#include <map>
#include <string>

#include "TeamGoals.h"



namespace marvin {

class Vector2f;
class Map;
struct PairedGoals;
struct MapCoord;
//using TeamGoals = std::unordered_map<std::string, PairedGoals>;

namespace path {
struct Pathfinder;
}


class BasePaths {
 public:
  //BasePaths(const TeamGoals& spawns, float radius, path::Pathfinder& pathfinder, const Map& map);
  BasePaths(const std::vector<TeamGoals>& spawns, float radius, path::Pathfinder& pathfinder, const Map& map);

  const std::vector<std::vector<Vector2f>>& GetBasePaths() { return base_paths_; }
  //const std::unordered_map<std::string, std::vector<Vector2f>>& GetBasePaths() { return base_paths_; }
  const std::vector<Vector2f>& GetPath() { return base_paths_[current_base_]; }
  std::size_t GetBase() { return current_base_; }
 // const std::vector<Vector2f>& GetPath() { 
  //  auto it = base_paths_.find(current_base_);
 //   if (it == base_paths_.end()) return {};
 //   return it->second; 
 // }
  //const std::string& GetBase() { return current_base_; }
  void SetBase(std::size_t index) { current_base_ = index; }
  //void SetBase(const std::string& name) { current_base_ = name; }

 private:
  void BuildPaths(const std::vector<MapCoord>& start, const std::vector<MapCoord>& end, float radius, path::Pathfinder& pathfinder,
                  const Map& map);
  std::vector<std::vector<Vector2f>> base_paths_;
  //std::unordered_map<std::string, std::vector<Vector2f>> base_paths_;
  std::size_t current_base_ = 0;
};

}  // namespace marvin
