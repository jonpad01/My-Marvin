#pragma once

#include "GameProxy.h"
#include "behavior/BehaviorEngine.h"
#include "platform/Platform.h"

namespace marvin {

class Time {
 public:
  Time(GameProxy& game) : game_(game), settimer_(true), cooldown_(0), unique_action_("") {}

  uint64_t GetTime();

  bool TimedActionDelay(std::string key, uint64_t delay);

  uint64_t UniqueIDTimer(uint16_t id);

  uint64_t DevaFreqTimer(std::vector<std::string> names);

 private:
  bool Has(const std::string& key) { return data_.find(key) != data_.end(); }

  void Set(const std::string& key, const uint64_t& value) { data_[key] = value; }

  uint64_t Value(const std::string& key) {
    auto iter = data_.find(key);
    return iter->second;
  }

  void Clear() { data_.clear(); }

  void Erase(const std::string& key) { data_.erase(key); }

  GameProxy& game_;

  bool settimer_;

  uint64_t cooldown_;

  std::string unique_action_;

  std::unordered_map<std::string, uint64_t> data_;
};

struct PerformanceTimer {
  u64 begin_time;

  PerformanceTimer();

  // Returns elapsed time and resets begin time
  u64 GetElapsedTime();
};

}  // namespace marvin
