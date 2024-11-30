#pragma once

#include <unordered_map>

#include "GameProxy.h"
//#include "behavior/BehaviorEngine.h"
//#include "platform/Platform.h"

namespace marvin {

struct TimerInfo {
  uint64_t expire_timestamp = 0;
  uint64_t initial_timestamp = 0;
  uint64_t initial_delay = 0; 
};

class Time {
 public:

  uint64_t GetTime();
  void Update();

  bool TimedActionDelay(std::string key, uint64_t delay);
  bool RepeatedActionDelay(std::string key, uint64_t delay);

  uint64_t UniqueTimerByName(const std::string& name, const std::vector<std::string>& names, uint64_t offset = 200);
  uint64_t UniqueIDTimer(GameProxy& game, uint16_t id);
  uint64_t UniqueIDTimer(GameProxy& game, std::vector<std::string> list);
  uint64_t UniqueIDTimer(GameProxy& game, std::vector<uint16_t> list);
  uint64_t DevaFreqTimer(GameProxy& game, std::vector<std::string> names);

 private:
  bool Has(const std::string& key) { return data_.find(key) != data_.end(); }
  void Set(const std::string& key, const TimerInfo& value) { data_[key] = value; }

  TimerInfo Value(const std::string& key) {
    auto iter = data_.find(key);
    return iter->second;
  }

  void Clear() { data_.clear(); }
  void Erase(const std::string& key) { data_.erase(key); }

  std::unordered_map<std::string, TimerInfo> data_;
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
