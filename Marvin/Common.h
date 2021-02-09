#pragma once
#include <memory>
#include "GameProxy.h"
#include "behavior/BehaviorEngine.h"
#include "path/Pathfinder.h"

namespace marvin {

	bool InRect(Vector2f pos, Vector2f min_rect, Vector2f max_rect);

	bool IsValidPosition(Vector2f position);

	class Common {

	public:

		Common(GameProxy& game) : game_(game) {}

		uint64_t GetTime();

		std::size_t FindOpenFreq(std::vector<uint16_t> list, std::size_t start_pos);

	private :

		GameProxy& game_;




	};






}