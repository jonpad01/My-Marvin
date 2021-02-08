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

		bool CalculateShot(Vector2f pShooter, Vector2f pTarget, Vector2f vShooter, Vector2f vTarget, float sProjectile, Vector2f* Solution);

		bool ShootWall(Vector2f target_pos, Vector2f target_vel, float target_radius, Vector2f direction, bool* bomb_hit, Vector2f* wall_pos);

		uint64_t GetTime();

		std::size_t FindOpenFreq(std::vector<uint16_t> list, std::size_t start_pos);

		std::size_t FindPathNode(std::vector<Vector2f> path, Vector2f position);
		float PathLength(std::vector<Vector2f> path, std::size_t index1, std::size_t index2);

		bool CanShootGun(const Map& map, const Player& bot_player, const Player& target);
		bool CanShootBomb(const Map& map, const Player& bot_player, const Player& target);

		std::vector<Vector2f> CreatePath(path::Pathfinder& pathfinder, std::vector<Vector2f> path, Vector2f from, Vector2f to, float radius);

	private :

		GameProxy& game_;




	};






}