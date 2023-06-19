#pragma once

#include <unordered_map>

#include "GameProxy.h"
//#include "behavior/BehaviorEngine.h"
//#include "platform/Platform.h"

namespace marvin {

    struct TimerPair {
      uint64_t timestamp;
      uint64_t delay;
    };

class Time {
 public:
  Time(GameProxy& game) : game_(game) {}

  uint64_t GetTime();

  bool TimedActionDelay(std::string key, uint64_t delay);
  bool RepeatedActionDelay(std::string key, uint64_t delay);

  uint64_t UniqueIDTimer(uint16_t id);

  uint64_t DevaFreqTimer(std::vector<std::string> names);

 private:
  bool Has(const std::string& key) { return data_.find(key) != data_.end(); }

  void Set(const std::string& key, const uint64_t& value) { data_[key].timestamp = value; }
  void SetDelay(const std::string& key, const uint64_t& value) { data_[key].delay = value; }

  uint64_t Value(const std::string& key) {
    auto iter = data_.find(key);
    return iter->second.timestamp;
  }

  uint64_t DelayValue(const std::string& key) {
    auto iter = data_.find(key);
    return iter->second.delay;
  }

  void Clear() { data_.clear(); }
  void Erase(const std::string& key) { data_.erase(key); }

  GameProxy& game_;
  std::string unique_action_;

  std::unordered_map<std::string, TimerPair> data_;
};

struct PerformanceTimer {
  u64 begin_time;
  u64 last_update_time;

  PerformanceTimer();

  // Returns elapsed time and resets begin time
  u64 GetElapsedTime();
  // Returns elapsed time since this timer was created
  u64 TimeSinceConstruction();
};

}  // namespace marvin
