#pragma once

#include <vector>
#include "GameProxy.h"

namespace marvin {

	bool CheckStatus(GameProxy& game, KeyController& keys, bool use_max);

	std::size_t FindOpenFreq(std::vector<uint16_t> list, std::size_t start_pos);

	


}