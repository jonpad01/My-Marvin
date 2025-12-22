#include "Time.h"

#include <algorithm>
#include <chrono>

namespace marvin {

uint64_t Time::GetTime() {
  return std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now())
      .time_since_epoch()
      .count();
}

void Time::Update() {
  
  for (auto i = data_.begin(); i != data_.end(); i++) {
    if (GetTime() > i->second.expire_timestamp + 100) {
      data_.erase(i->first);
      i = data_.begin();
    }
  }

}

// causes an initial delay 
bool Time::TimedActionDelay(std::string key, uint64_t delay) {
  
  TimerInfo info;
  uint64_t timeout_offset = 0;
  #if 0
  // if the delay is not the same update the current value
  if (Has(key) && Value(key).initial_delay != delay) {
    
    // the current delay has been shortened
    if (info.initial_delay >= delay) {
    //  timeout_offset = info.initial_delay - delay; 
    }

    info.initial_delay = delay;
    info.expire_timestamp = info.initial_timestamp + delay;

    Set(key, info);
  }
  #endif



  if (!Has(key) || delay != Value(key).initial_delay) {
  //if (!Has(key) || GetTime() > Value(key).expire_timestamp + 100 + timeout_offset) {
    info.initial_timestamp = GetTime();
    info.initial_delay = delay;
    info.expire_timestamp = GetTime() + delay;
    Set(key, info);
  } else if (GetTime() > Value(key).expire_timestamp) {
    Erase(key);
    return true;
  }
  
  return false;
}

// triggers on 1st run but delays afterwards
bool Time::RepeatedActionDelay(std::string key, uint64_t delay) {

    TimerInfo info;

  if (!Has(key) || GetTime() > Value(key).expire_timestamp) {
    info.expire_timestamp = GetTime() + delay;
    info.initial_delay = delay;
    info.initial_timestamp = GetTime();
      
    Set(key, info);
    return true;
  }
  return false;
}

uint64_t Time::UniqueTimerByName(const std::string& name, const std::vector<std::string>& names, uint64_t offset) {
  uint64_t result = offset;

  for (std::size_t i = 0; i < names.size(); i++) {
    if (name == names[i]) {
      result += i * offset;
    }
  }
  return result;
}


// create a consitent gap between each unique id
// zones endup with large gaps between player ids so use a sorted vector to sandwich them together
uint64_t Time::UniqueIDTimer(GameProxy& game, uint16_t id) {
  uint64_t time = 0;

  std::vector<uint16_t> id_list;

  uint64_t id_count = game.GetPlayers().size();

  for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
    id_list.push_back(game.GetPlayers()[i].id);
  }

  std::sort(id_list.begin(), id_list.end());

  for (std::size_t i = 0; i < id_list.size(); i++) {
    if (game.GetPlayer().id == id_list[i]) {
      //bool odd = i % 2 == 0;

      //if (odd) {
        //time = ((uint64_t)i + 1 + id_count) * 500;
      //} else {
        time = ((uint64_t)i + 1) * 200;
      //}
    }
  }
  return time;
}

uint64_t Time::UniqueDelay(GameProxy& game, const std::vector <std::string>& list) {
  uint64_t time = 0;

  for (std::size_t i = 0; i < list.size(); i++) {
    if (game.GetPlayer().name == list[i]) {
        time = ((uint64_t)i + 1) * 300;
        break;
    }
  }
  return time;
}

uint64_t Time::UniqueIDTimer(GameProxy& game, std::vector<uint16_t>& list) {
  uint64_t time = 0;

  std::sort(list.begin(), list.end());

  for (std::size_t i = 0; i < list.size(); i++) {
    if (game.GetPlayer().id == list[i]) {
        time = ((uint64_t)i + 1) * 300;
        break;
    }
  }
  return time;
}

#if 0
uint64_t Time::DevaFreqTimer(GameProxy& game, std::vector<std::string> names) {
  uint64_t offset = 200;
  uint64_t offset_index = 0;
  std::vector<uint16_t> active_list;

  bool wait_for_bot = false;
  bool bot_dead = false;

  // as long as the list of names stays in the same order, each bot will return a unique offset depending on where its
  // name is in the list
  for (std::size_t j = 0; j < names.size(); j++) {
    // loop through all players and try to match name, if player isnt online the index wont get increased
    for (std::size_t i = 0; i < game.GetPlayers().size(); i++) {
      const Player& player = game.GetPlayers()[i];
      if (player.name == names[j]) {
        // dont increase index if player isnt in a ship
        if (player.ship != 8) {
          // active_list.push_back(player.id);
          offset_index++;
          // special state to make the bot wait for other bots to jump into the game first
          if (game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01) {
            if (player.frequency != 00 && player.frequency != 01 && player.name != game.GetPlayer().name) {
              wait_for_bot = true;
              if (!player.dead) {
                bot_dead = true;
              }
            }
          }
        }
      }
    }
    // if name matches, the index is will be unique to this name
    if (game.GetPlayer().name == names[j]) {
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

#endif

uint64_t Time::DevaFreqTimer(GameProxy& game, const std::vector<std::string>& names) {
  uint64_t offset = 200;
  uint64_t offset_index = 0;
  bool dueling = false;
  bool others_not_dueling = false;
  bool others_not_dueling_and_dead = false;

  if (game.GetPlayer().frequency == 00 || game.GetPlayer().frequency == 01) {
    dueling = true;
  }

  // are any bots on a duel team?  are they respawning?
  for (std::size_t j = 0; j < names.size(); j++) {
    const Player* player = game.GetPlayerByName(names[j]);
    if (!player) continue;

    if (player->name == game.GetPlayer().name) {
      offset += j * 200;
    }

    if (player->frequency != 00 && player->frequency != 01 && player->name != game.GetPlayer().name) {
      others_not_dueling = true;
      if (player->dead) {
        others_not_dueling_and_dead = true;
      }
    }
  } 

  if (dueling && others_not_dueling) {
    offset += names.size() * 200;
    if (others_not_dueling_and_dead) {
      offset += 5001;
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
