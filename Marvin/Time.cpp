#include "Time.h"

#include <algorithm>
#include <chrono>

namespace marvin {

uint64_t Time::GetTime() {
  return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now())
      .time_since_epoch()
      .count();
}

// causes an initial delay 
bool Time::TimedActionDelay(std::string key, uint64_t delay) {
  if (!Has(key) || GetTime() > Value(key) + 100) {
    Set(key, GetTime() + delay);
  } else if (GetTime() > Value(key)) {
    Erase(key);
    return true;
  }
  return false;
}

// triggers on 1st run but delays afterwards
bool Time::RepeatedActionDelay(std::string key, uint64_t delay) {
  if (!Has(key) || GetTime() > Value(key) || delay != DelayValue(key)) {
    Set(key, GetTime() + delay);
    SetDelay(key, delay);
    return true;
  }
  return false;
}

// create a consitent gap between each unique id
// zones endup with large gaps between player ids so use a sorted vector to sandwich them together
uint64_t Time::UniqueIDTimer(uint16_t id) {
  uint64_t time = 0;

  std::vector<uint16_t> id_list;

  uint64_t id_count = game_.GetPlayers().size();

  for (std::size_t i = 0; i < game_.GetPlayers().size(); i++) {
    id_list.push_back(game_.GetPlayers()[i].id);
  }

  std::sort(id_list.begin(), id_list.end());

  for (std::size_t i = 0; i < id_list.size(); i++) {
    if (game_.GetPlayer().id == id_list[i]) {
      bool odd = i % 2 == 0;

      if (odd) {
        time = ((uint64_t)i + 1 + id_count) * 500;
      } else {
        time = ((uint64_t)i + 1) * 500;
      }
    }
  }
  return time;
}

uint64_t Time::DevaFreqTimer(std::vector<std::string> names) {
  uint64_t offset = 200;
  uint64_t offset_index = 0;
  std::vector<uint16_t> active_list;

  bool wait_for_bot = false;
  bool bot_dead = false;

  // as long as the list of names stays in the same order, each bot will return a unique offset depending on where its
  // name is in the list
  for (std::size_t j = 0; j < names.size(); j++) {
    // loop through all players and try to match name, if player isnt online the index wont get increased
    for (std::size_t i = 0; i < game_.GetPlayers().size(); i++) {
      const Player& player = game_.GetPlayers()[i];
      if (player.name == names[j]) {
        // dont increase index if player isnt in a ship
        if (player.ship != 8) {
          // active_list.push_back(player.id);
          offset_index++;
          // special state to make the bot wait for other bots to jump into the game first
          if (game_.GetPlayer().frequency == 00 || game_.GetPlayer().frequency == 01) {
            if (player.frequency != 00 && player.frequency != 01 && player.name != game_.GetPlayer().name) {
              wait_for_bot = true;
              if (!player.active) {
                bot_dead = true;
              }
            }
          }
        }
      }
    }
    // if name matches, the index is will be unique to this name
    if (game_.GetPlayer().name == names[j]) {
      offset += offset_index * 200;
    }
  }
  // after the loop has completed, if the special state was found, make bots that are basing wait the maximum offset
  // limit until the bots that arent basing jump in
  if (wait_for_bot) {
    offset += offset_index * 200;
    if (bot_dead) {
      // if the bot its waiting for was dead, increase the wait time by the zones enter delay
      offset += 5000;
    }
  }

  return offset;
}

PerformanceTimer::PerformanceTimer() {
  begin_time = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now())
                   .time_since_epoch()
                   .count();
  last_update_time = begin_time;
}

u64 PerformanceTimer::TimeSinceConstruction() {
  u64 now = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now())
                .time_since_epoch()
                .count();

  u64 elapsed = now - begin_time;

  return elapsed;
}

u64 PerformanceTimer::GetElapsedTime() {
  u64 now = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now())
    .time_since_epoch()
    .count();

  u64 elapsed = now - last_update_time;

  last_update_time = now;

  return elapsed;
}

}  // namespace marvin
