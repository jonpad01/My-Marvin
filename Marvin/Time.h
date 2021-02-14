#pragma once

#include "platform/Platform.h"


namespace marvin {

	class Time {

	public:

		Time() : settimer_(true), cooldown_(0), type_("") {}

		uint64_t GetTime();

		bool TimedActionDelay(uint64_t delay, std::string type);


	private:

		bool settimer_;

		uint64_t cooldown_;

		std::string type_;

	};


}