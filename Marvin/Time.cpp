#include "Time.h"

#include <chrono>

namespace marvin {


    uint64_t Time::GetTime() {
        return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
    }

    bool Time::TimedActionDelay(uint64_t delay, std::string type) {

        if (GetTime() > cooldown_ + 50) {
            settimer_ = true;
        }
        else if (type_ != type) {
            settimer_ = true;
            type_ = type;
        }

        if (settimer_) {
            cooldown_ = GetTime() + delay;
            settimer_ = false;
            return false;
        }
        else if (GetTime() > cooldown_) {
            settimer_ = true;
            return true;
        }
        return false;
    }
}