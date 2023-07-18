#include "Common.h"

#include <algorithm>
#include <chrono>

#include "Debug.h"

namespace marvin {

  MapCoord ToNearestTile(Vector2f position) {
    return MapCoord(std::round(position.x), std::round(position.y));
  }

std::string Lowercase(const std::string& str) {
  std::string result;

  std::string name = str;

  //remove "^" that gets placed on names when biller is down
  if (!name.empty() && name[0] == '^') {
    name.erase(0, 1);
  }

  result.resize(name.size());

  std::transform(name.begin(), name.end(), result.begin(), ::tolower);

  return result;
}

std::string GetWorkingDirectory() {
  std::string directory;

  directory.resize(GetCurrentDirectory(0, NULL));

  GetCurrentDirectory(directory.size(), &directory[0]);

  return directory.substr(0, directory.size() - 1);
}

bool CheckStatus(GameProxy& game, KeyController& keys, bool use_max) {
  float max_energy = 0.0f;

  if (use_max) {
    max_energy = (float)game.GetShipSettings().MaximumEnergy;
  } else {
    max_energy = (float)game.GetShipSettings().InitialEnergy;
  }

  float energy_pct = ((float)game.GetEnergy() / max_energy) * 100.0f;
  bool result = false;

  if ((game.GetPlayer().status & 2) != 0) {
    game.Cloak(keys);
  } else if (energy_pct == 100.0f) {
    result = true;
  }

  return result;
}

uint16_t FindOpenFreq(const std::vector<uint16_t>& list, uint16_t start_pos) {
  uint16_t open_freq = 0;

  for (uint16_t i = start_pos; i < list.size(); i++) {
    if (list[i] == 0) {
      open_freq = i;
      break;
    }
  }

  return open_freq;
}

std::vector<std::string_view> SplitString(std::string_view string, std::string_view delim) {
  std::vector<std::string_view> result;

  std::size_t offset = 0;
  std::size_t start = 0;

  while ((offset = string.find(delim, offset)) != std::string::npos) {
    std::string_view split = string.substr(start, offset - start);

    result.push_back(split);

    offset += delim.size();
    start = offset;
  }

  result.push_back(string.substr(start));

  return result;
}

Vector2f DiscreteToHeading(uint16_t rotation) {
  const float kToRads = (static_cast<float>(M_PI) / 180.0f);
  float rads = (((40 - (rotation + 30)) % 40) * 9.0f) * kToRads;
  float x = std::cos(rads);
  float y = -std::sin(rads);

  return Vector2f(x, y);
}

float DiscreteToRadians(uint16_t rotation) {
  const float kToRads = (static_cast<float>(M_PI) / 180.0f);
  float rads = (((40 - (rotation + 30)) % 40) * 9.0f) * kToRads;

  return rads;
}

}  // namespace marvin
