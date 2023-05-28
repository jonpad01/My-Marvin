#pragma once

#include <vector>

#include "GameProxy.h"

namespace marvin {

std::string Lowercase(const std::string& str);

std::string GetWorkingDirectory();

bool CheckStatus(GameProxy& game, KeyController& keys, bool use_max);

std::size_t FindOpenFreq(const std::vector<uint16_t>& list, std::size_t start_pos);

}  // namespace marvin
