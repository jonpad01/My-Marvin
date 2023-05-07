#pragma once

#include <vector>
#include "..//..//Vector2f.h"

namespace marvin {
namespace deva {

struct BaseWarps {
  bool empty() { return t0.empty() && t1.empty(); }
  void clear() {
    t0.clear();
    t1.clear();
  }
  std::vector<MapCoord> t0;
  std::vector<MapCoord> t1;
};

class BaseDuelWarpCoords {
 public:
  BaseDuelWarpCoords(const std::string& mapName);

  bool FoundMapFiles() { return foundMapFiles; }
  bool HasCoords() { return !warps_.empty(); }
  std::string TrimExtension(const std::string& mapName);
  bool LoadBaseDuelFile(const std::string& mapName);
  bool LoadFile(std::ifstream& file);
  int GetIntMatch(std::ifstream& file, const std::string& match);
  const BaseWarps& GetWarps() { return warps_; }

 private:
  bool foundMapFiles;
  BaseWarps warps_;
};

}  // namespace deva
}  // namespace marvin