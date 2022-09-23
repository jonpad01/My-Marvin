#pragma once

#include <any>
#include <optional>
#include <string>
#include <unordered_map>

#include "../player.h"

namespace marvin {

    enum class BB : short { Ship, PubTeam0, PubTeam1, EnemyNetBulletTravel };

namespace behavior {

    

class Blackboard {
 public:
  Blackboard()
      : pubteam0_(00),
        pubteam1_(01),
        chatcount_(0),
        lock_(false),
        freqlock_(false),
        anchor_(false),
        repel_(false),
        multifire_(false),
        swarm_(false),
        ship_(0),
        freq_(999),
        teaminbase_(false),
        enemyinbase_(false),
        lastinbase_(true),
        regionindex_(0),
        incenter_(true),
        targetinsight_(false),
        steerbackwards_(false),
        target_(nullptr) {}

  bool Has(const std::string& key) { return data_.find(key) != data_.end(); }

  bool Has(const BB& key) { return data2_.find(key) != data2_.end(); }

  template <typename T>
  void Set(const std::string& key, const T& value) {
    data_[key] = std::any(value);
  }

  template <typename T>
  void Set(const BB& key, const T& value) {
    data2_[key] = std::any(value);
  }

  template <typename T>
  std::optional<T> Value(const std::string& key) {
    auto iter = data_.find(key);

    if (iter == data_.end()) {
      return std::nullopt;
    }

    auto& any = iter->second;

    try {
      return std::any_cast<T>(any);
    } catch (const std::bad_any_cast&) {
      return std::nullopt;
    }
  }

  template <typename T>
  std::optional<T> Value(const BB& key) {
    auto iter = data2_.find(key);

    if (iter == data2_.end()) {
      return std::nullopt;
    }

    auto& any = iter->second;

    try {
      return std::any_cast<T>(any);
    } catch (const std::bad_any_cast&) {
      return std::nullopt;
    }
  }

  template <typename T>
  T ValueOr(const std::string& key, const T& or_result) {
    return Value<T>(key).value_or(or_result);
  }

  template <typename T>
  T ValueOr(const BB& key, const T& or_result) {
    return Value<T>(key).value_or(or_result);
  }

  void Clear() { 
    data_.clear();
    data2_.clear();
  }
  void Erase(const std::string& key) { data_.erase(key); }
  void Erase(const BB& key) { data2_.erase(key); }

#if 0
        void SetPubTeam0(uint16_t freq) { pubteam0_ = freq; }
        uint16_t GetPubTeam0() { return pubteam0_; }

        void SetPubTeam1(uint16_t freq) { pubteam1_ = freq; }
        uint16_t GetPubTeam1() { return pubteam1_; }


        void SetChatCount(std::size_t count) { chatcount_ = count; }
        std::size_t GetChatCount() { return chatcount_; }

        void SetLock(bool lock) { lock_ = lock; }
        bool GetLock() { return lock_; }

        void SetFreqLock(bool lock) { freqlock_ = lock; }
        bool GetFreqLock() { return freqlock_; }

        void SetAnchor(bool state) { anchor_ = state; }
        bool IsAnchor() { return anchor_; }

        void SetRepel(bool state) { repel_ = state; }
        bool UseRepel() { return repel_; }

        void SetMultiFire(bool state) { multifire_ = state; }
        bool UseMultiFire() { return multifire_; }

        void SetSwarm(bool state) { swarm_ = state; }
        bool GetSwarm() { return swarm_; }

        void SetShip(int ship) { ship_ = ship; }
        int GetShip() { return ship_; }

        void SetFreq(int freq) { freq_ = freq; }
        int GetFreq() { return freq_; }

        
        void SetFreqList(std::vector<uint16_t> list) { freqlist_ = list; }
        std::vector<uint16_t> GetFreqList() { return freqlist_; }

        void ClearTeamList() { teamlist_.clear(); }
        void PushTeamList(Player player) { teamlist_.emplace_back(player); }
        std::vector<Player>& GetTeamList() { return teamlist_; }

        void ClearEnemyList() { enemylist_.clear(); }
        void PushEnemyList(Player player) { enemylist_.emplace_back(player); }
        std::vector<Player>& GetEnemyList() { return enemylist_; }

        void ClearCombinedList() { combinedlist_.clear(); }
        void PushCombinedList(Player player) { combinedlist_.emplace_back(player); }
        std::vector<Player>& GetCombinedList() { return combinedlist_; }

        void SetTeamInBase(bool state) { teaminbase_ = state; }
        bool TeamInBase() { return teaminbase_; }

        void SetEnemyInBase(bool state) { enemyinbase_ = state; }
        bool EnemyInBase() { return enemyinbase_; }

        void SetLastInBase(bool state) { lastinbase_ = state; }
        bool LastInBase() { return lastinbase_; }

        void SetRegionIndex(std::size_t index) { regionindex_ = index; }
        std::size_t GetRegionIndex() { return regionindex_; }

        void SetInCenter(bool state) { incenter_ = state; }
        bool InCenter() { return incenter_; }

        void SetTeamSafe(Vector2f position) { teamsafe_ = position; }
        Vector2f TeamSafe() { return teamsafe_; }

        void SetEnemySafe(Vector2f position) { enemysafe_ = position; }
        Vector2f EnemySafe() { return enemysafe_; }

        void SetTarget(const Player* target) { target_ = target; }
        const Player* GetTarget() { return target_; }

        void SetPath(std::vector<Vector2f> path) { path_ = path; }
        std::vector<Vector2f> GetPath() { return path_; }

        void SetTargetInSight(bool state) { targetinsight_ = state; }
        bool TargetInSight() { return targetinsight_; }

        void SetSteerBackwards(bool state) { steerbackwards_ = state; }
        bool GetSteerBackwards() { return steerbackwards_; }

        void SetSolution(Vector2f position) { solution_ = position; }
        Vector2f GetSolution() { return solution_; }
#endif

 private:
  uint16_t pubteam0_;
  uint16_t pubteam1_;

  std::size_t chatcount_;
  bool lock_;
  bool freqlock_;
  bool anchor_;
  bool repel_;
  bool multifire_;
  bool swarm_;
  int ship_;
  int freq_;

  std::vector<uint16_t> freqlist_;

  std::vector<Player> teamlist_;
  std::vector<Player> enemylist_;
  std::vector<Player> combinedlist_;

  bool teaminbase_;
  bool enemyinbase_;
  bool lastinbase_;

  std::size_t regionindex_;
  bool incenter_;

  Vector2f teamsafe_;
  Vector2f enemysafe_;

  const Player* target_;
  bool targetinsight_;
  bool steerbackwards_;

  Vector2f solution_;

  std::vector<Vector2f> path_;

 
  std::unordered_map<std::string, std::any> data_;
  std::unordered_map<Var, std::any> data2_;
};

}  // namespace behavior
}  // namespace marvin
