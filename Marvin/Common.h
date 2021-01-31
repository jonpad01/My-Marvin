#pragma once
#include <memory>
#include "GameProxy.h"
#include "behavior/BehaviorEngine.h"

namespace marvin {

	class Common {

	public:

		Common(std::shared_ptr<GameProxy> game) : game_(std::move(game)) {}

		Vector2f CalculateShot(const Vector2f& pShooter, const Vector2f& pTarget, const Vector2f& vShooter, const Vector2f& vTarget, float sProjectile);

		bool ShootWall(Vector2f target_pos, Vector2f target_vel, Vector2f direction, float proj_speed, int alive_time, uint8_t bounces);

		void RenderWorldLine(Vector2f screenCenterWorldPosition, Vector2f from, Vector2f to);
		void RenderPath(GameProxy& game, behavior::Blackboard& blackboard);

		uint64_t GetTime();

		std::size_t FindOpenFreq(std::vector<uint16_t> list, std::size_t start_pos);

		std::size_t FindPathNode(std::vector<Vector2f> path, Vector2f position);
		float PathLength(std::vector<Vector2f> path, std::size_t index1, std::size_t index2);

	private :

		std::shared_ptr<GameProxy> game_;




	};






}