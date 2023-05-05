#pragma once

#include <vector>
#include "..//..//Vector2f.h"

namespace marvin {
namespace deva {

struct BaseSpawns {
  bool empty() { return t0.empty() && t1.empty(); }
  void clear() {
    t0.clear();
    t1.clear();
  }
  std::vector<Vector2f> t0;
  std::vector<Vector2f> t1;
};

class BaseDuelSpawnCoords {
 public:
  BaseDuelSpawnCoords(const std::string& mapName);

  bool FoundMapFiles() { return foundMapFiles; }
  bool HasCoords() { return !spawns_.empty(); }
  std::string TrimExtension(const std::string& mapName);
  bool LoadBaseDuelFile(const std::string& mapName);
  bool LoadFile(std::ifstream& file);
  int GetIntMatch(std::ifstream& file, const std::string& match);
  const BaseSpawns& GetSpawns() { return spawns_; }

 private:
  bool foundMapFiles;
  BaseSpawns spawns_;
};

}  // namespace deva
}  // namespace marvin