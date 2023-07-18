#pragma once

#include <any>
#include <optional>
#include <string>
#include <unordered_map>

#include "../player.h"
#include "../MapCoord.h"
#include "../GameProxy.h"

namespace marvin {


#if 0
    enum class BB { 
        UseRepel, UseBurst, UseDecoy, UseRocket, UseThor, UseBrick, UsePortal,
        UseMultiFire, UseCloak, UseStealth, UseXRadar, UseAntiWarp,
        PubTeam0, PubTeam1, 
        EnemySafe, TeamSafe, BaseIndex,
        EnemyNetBulletTravel, 
        End };
#endif

class Blackboard {
 private:
  // 8 bytes
  const Player* target_;
  const Player* anchor;
  const Player* enemy_anchor;
  std::size_t bd_base_index_;
  std::size_t patrol_index_;
  std::vector<const Player*> team_list_;
  std::vector<const Player*> enemy_list_;
  std::vector<const Player*> combined_list_;
  uint64_t bd_warp_cooldown_;
  uint64_t set_ship_cooldown_;
  HSItems hs_items_; 

  // 4 bytes
  int base_teams_count_;
  float enemy_net_bullet_travel_;
  float bomb_cooldown_;
  std::vector<int> team_ship_counts;
  Vector2f solution_;
  MapCoord team_safe_;
  MapCoord enemy_safe_;

  // 2 bytes
  CombatRole combat_role_;
  CombatRole combat_role_default_;
  BDState bd_state_;
  CommandRequestType command_request_;
  WarpToState warp_to_state_;
  Ship ship_;
  uint16_t pub_team0_;
  uint16_t pub_team1_;
 // uint16_t ship_;
  uint16_t freq_;
  std::vector<uint16_t> freq_list_;
  MapCoord center_spawn_;
  std::vector<MapCoord> patrol_nodes_;
  short team0_score_;
  short team1_score_;

  // 1 byte
  std::string arena_;
  //ChatType bd_chat_type_;
  bool in_center_;
  bool steer_backwards_;
  bool team_in_base_;
  bool enemy_in_base_;
  bool last_in_base_;
  bool target_in_sight_;
  bool use_repel_;
  bool use_multifire_;
  bool use_cloak_;
  bool use_stealth_;
  bool use_decoy_;
  bool use_xradar_;
  bool use_burst_;
  bool use_antiwarp_;
  bool use_rocket_;
  bool use_brick_;
  bool use_portal_;
  bool use_thor_;
  bool use_repel_default_;
  bool use_multifire_default_;
  bool use_cloak_default_;
  bool use_stealth_default_;
  bool use_decoy_default_;
  bool use_xradar_default_;
  bool use_burst_default_;
  bool use_antiwarp_default_;
  bool use_rocket_default_;
  bool use_brick_default_;
  bool use_portal_default_;
  bool use_thor_default_;
  bool freq_lock_;
  bool command_lock_;
  bool swarm_;
  bool team_has_summoner;
  bool update_lancs_flag;
  bool can_flag;

 public:
  Blackboard() {
    
    target_ = nullptr; 
    anchor = nullptr;
    enemy_anchor = nullptr;
    bd_base_index_ = 0;
    patrol_index_ = 0;
    bd_warp_cooldown_ = 0;
    set_ship_cooldown_ = 200;

    enemy_net_bullet_travel_ = 0.0f;
    bomb_cooldown_ = 0.0f;
    base_teams_count_ = 0;
    team_ship_counts.resize(8, 0);

    pub_team0_ = 00;
    pub_team1_ = 01;
   // ship_ = 0;
    ship_ = Ship::Spectator;
    freq_ = 999;
    freq_list_.resize(100, 0);
    combat_role_ = CombatRole::Rusher;
    combat_role_default_ = CombatRole::Rusher;
    bd_state_ = BDState::Stopped;
    warp_to_state_ = WarpToState::None;
    command_request_ = CommandRequestType::None;
    team0_score_ = 0;
    team1_score_ = 0;

    //bd_chat_type_ = ChatType::Private;
    in_center_ = true;
    steer_backwards_ = false;
    team_in_base_ = false;
    enemy_in_base_ = false;
    last_in_base_ = true;
    target_in_sight_ = false;
    use_repel_ = true;
    use_multifire_ = false;
    use_cloak_ = true;
    use_stealth_ = true;
    use_decoy_ = true;
    use_xradar_ = true;
    use_burst_ = true;
    use_antiwarp_ = true;
    use_rocket_ = true;
    use_brick_ = true;
    use_portal_ = true;
    use_thor_ = true;
    use_repel_default_ = true;
    use_multifire_default_ = false;
    use_cloak_default_ = true;
    use_stealth_default_ = true;
    use_decoy_default_ = true;
    use_xradar_default_ = true;
    use_burst_default_ = true;
    use_antiwarp_default_ = true;
    use_rocket_default_ = true;
    use_brick_default_ = true;
    use_portal_default_ = true;
    use_thor_default_ = true;
    freq_lock_ = false;
    swarm_ = false;
    command_lock_ = false;  
    can_flag = false;
    team_has_summoner = false;
    update_lancs_flag = true;
  }

  void SetHSItems(const HSItems& items) { hs_items_ = items; }
  const HSItems& GetHSItems() { return hs_items_; }

  void SetBDBaseIndex(std::size_t index) { bd_base_index_ = index; }
  std::size_t GetBDBaseIndex() { return bd_base_index_; }

  void SetPatrolIndex(std::size_t index) { patrol_index_ = index; }
  std::size_t GetPatrolIndex() { return patrol_index_; }

  void SetBDWarpCoolDown(uint64_t cooldown) { bd_warp_cooldown_ = cooldown; }
  uint64_t GetBDWarpCoolDown() { return bd_warp_cooldown_; }

  void SetSetShipCoolDown(uint64_t cooldown) { set_ship_cooldown_ = cooldown; }
  uint64_t GetSetShipDown() { return set_ship_cooldown_; }

  void SetTarget(const Player* target) { target_ = target; }
  const Player* GetTarget() { return target_; }

  const Player* GetAnchor() { return anchor; }
  void SetAnchor(const Player* player) { anchor = player; }

  const Player* GetEnemyAnchor() { return enemy_anchor; }
  void SetEnemyAnchor(const Player* player) { enemy_anchor = player; } 

  int GetBaseTeamsCount() { return base_teams_count_; }
  void SetBaseTeamsCount(int count) { base_teams_count_ = count; } 

  void SetEnemyNetBulletTravel(float travel) { enemy_net_bullet_travel_ = travel; }
  float GetEnemyNetBulletTravel() { return enemy_net_bullet_travel_; }

  void SetBombCooldown(float time) { bomb_cooldown_ = time; }
  float GetBombCooldown() { return bomb_cooldown_; }

  void ClearTeamShipCounts() { memset(&team_ship_counts[0], 0, team_ship_counts.size() * sizeof team_ship_counts[0]); }
  void IncrementTeamShipCounts(Ship index) { team_ship_counts[(std::size_t)index]++; }
  const std::vector<int>& GetTeamShipCounts() { return team_ship_counts; }

  void SetSolution(Vector2f position) { solution_ = position; }
  Vector2f GetSolution() { return solution_; }

  void SetTeamSafe(MapCoord position) { team_safe_ = position; }
  MapCoord GetTeamSafe() { return team_safe_; }

  void SetEnemySafe(MapCoord position) { enemy_safe_ = position; }
  MapCoord GetEnemySafe() { return enemy_safe_; }

  void SetCenterSpawn(MapCoord position) { center_spawn_ = position; }
  MapCoord GetCenterSpawn() { return center_spawn_; }

  void ClearPatrolNodes() { patrol_nodes_.clear(); }
  void SetPatrolNodes(const std::vector<MapCoord>& nodes) { patrol_nodes_ = nodes; }
  const std::vector<MapCoord>& GetPatrolNodes() { return patrol_nodes_; }

  void SetPubTeam0(uint16_t freq) { pub_team0_ = freq; }
  uint16_t GetPubTeam0() { return pub_team0_; }

  void SetPubTeam1(uint16_t freq) { pub_team1_ = freq; }
  uint16_t GetPubTeam1() { return pub_team1_; }

  //void SetShip(uint16_t ship) { 
   // ship_ = ship;
   // command_request_ = CommandRequestType::ShipChange;
  //}
  //uint16_t GetShip() { return ship_; }

  void SetShip(Ship ship) {
    ship_ = ship;
    command_request_ = CommandRequestType::ShipChange;
  }
  Ship GetShip() { return ship_; }

  void SetFreq(uint16_t freq) { 
      freq_ = freq;
    command_request_ = CommandRequestType::FreqChange;
  }
  uint16_t GetFreq() { return freq_; }

  void SetTeam0Score(short score) { team0_score_ = score; }
  short GetTeam0Score() { return team0_score_; }

  void SetTeam1Score(short score) { team1_score_ = score; }
  short GetTeam1Score() { return team1_score_; }

  void ClearFreqList() { memset(&freq_list_[0], 0, freq_list_.size() * sizeof freq_list_[0]); }
  void IncrementFreqList(std::size_t index) { freq_list_[index]++; }
  const std::vector<uint16_t>& GetFreqList() { return freq_list_; }

  void SetCombatRole(CombatRole role) { combat_role_ = role; }
  void SetCombatRoleDefault(CombatRole role) { combat_role_default_ = role; }
  CombatRole GetCombatRole() { return combat_role_; }

 // void SetBDChatType(ChatType type) { bd_chat_type_ = type; }
 // ChatType GetBDChatType() { return bd_chat_type_; }

  void SetInCenter(bool state) { in_center_ = state; }
  bool GetInCenter() { return in_center_; }

  void SetSteerBackwards(bool state) { steer_backwards_ = state; }
  bool GetSteerBackwards() { return steer_backwards_; }

  void SetTeamInBase(bool state) { team_in_base_ = state; }
  bool GetTeamInBase() { return team_in_base_; }

  void SetEnemyInBase(bool state) { enemy_in_base_ = state; }
  bool GetEnemyInBase() { return enemy_in_base_; }

  void SetLastInBase(bool state) { last_in_base_ = state; }
  bool GetLastInBase() { return last_in_base_; }

  void SetTargetInSight(bool state) { target_in_sight_ = state; }
  bool GetTargetInSight() { return target_in_sight_; }

  void SetFreqLock(bool lock) { freq_lock_ = lock; }
  bool GetFreqLock() { return freq_lock_; }

  void SetSwarm(bool state) { swarm_ = state; }
  bool GetSwarm() { return swarm_; }

  void SetCommandLock(bool lock) { command_lock_ = lock; }
  bool GetCommandLock() { return command_lock_; }

  bool GetUpdateLancsFlag() { return update_lancs_flag; }
  void SetUpdateLancsFlag(bool count) { update_lancs_flag = count; }

  bool GetCanFlag() { return can_flag; }
  void SetCanFlag(bool value) { can_flag = value; }

  bool GetTeamHasSummoner() { return team_has_summoner; }
  void SetTeamHasSummoner(bool value) { team_has_summoner = value; } 

  void SetCommandRequest(CommandRequestType type) { command_request_ = type; }
  CommandRequestType GetCommandRequest() { return command_request_; }

  void SetArena(const std::string& arena) { 
    arena_ = arena;
    command_request_ = CommandRequestType::ArenaChange;
  }
  const std::string& GetArena() { return arena_; }

  void SetBDState(BDState state) { bd_state_ = state; }
  BDState GetBDState() { return bd_state_; }

  void SetWarpToState(WarpToState state) { warp_to_state_ = state; }
  WarpToState GetWarpToState() { return warp_to_state_; }

  void SetUseRepel(bool state) { use_repel_ = state; }
  void SetRepelDefault(bool state) { use_repel_default_ = state; }
  bool GetUseRepel() { return use_repel_; }

  void SetUseMultiFire(bool state) { use_multifire_ = state; }
  void SetMultiFireDefault(bool state) { use_multifire_default_ = state; }
  bool GetUseMultiFire() { return use_multifire_; }

  void SetUseCloak(bool state) { use_cloak_ = state; }
  void SetCloakDefault(bool state) { use_cloak_default_ = state; }
  bool GetUseCloak() { return use_cloak_; }

  void SetUseStealth(bool state) { use_stealth_ = state; }
  void SetStealthDefault(bool state) { use_stealth_default_ = state; }
  bool GetUseStealth() { return use_stealth_; }

  void SetUseXradar(bool state) { use_xradar_ = state; }
  void SetXradarDeufault(bool state) { use_xradar_default_ = state; }
  bool GetUseXradar() { return use_xradar_; }

  void SetUseDecoy(bool state) { use_decoy_ = state; }
  void SetDecoyDefault(bool state) { use_decoy_default_ = state; }
  bool GetUseDecoy() { return use_decoy_; }

  void SetUseBurst(bool state) { use_burst_ = state; }
  void SetBurstDefault(bool state) { use_burst_default_ = state; }
  bool GetUseBurst() { return use_burst_; }

  void SetUseAntiWarp(bool state) { use_antiwarp_ = state; }
  void SetAntiWarpDefault(bool state) { use_antiwarp_default_ = state; }
  bool GetUseAntiWarp() { return use_antiwarp_; }

  void SetUseRocket(bool state) { use_rocket_ = state; }
  void SetRocketDefault(bool state) { use_rocket_default_ = state; }
  bool GetUseRocket() { return use_rocket_; }

  void SetUseBrick(bool state) { use_brick_ = state; }
  void SetBrickDefault(bool state) { use_brick_default_ = state; }
  bool GetUseBrick() { return use_brick_; }

  void SetUsePortal(bool state) { use_portal_ = state; }
  void SetPortalDefault(bool state) { use_portal_default_ = state; }
  bool GetUsePortal() { return use_portal_; }

  void SetUseThor(bool state) { use_thor_ = state; }
  void SetThorDefault(bool state) { use_thor_default_ = state; }
  bool GetUseThor() { return use_thor_; }

  void SetToDefaultBehavior() { 
    combat_role_ = combat_role_default_;
    use_repel_ = use_repel_default_;
    use_multifire_ = use_multifire_default_;
    use_cloak_ = use_cloak_default_;
    use_stealth_ = use_stealth_default_;
    use_xradar_ = use_xradar_default_;
    use_decoy_ = use_decoy_default_;
    use_burst_ = use_burst_default_;
    swarm_ = false;
  }

  void ClearTeamList() { team_list_.clear(); }
  void PushTeamList(const Player* player) { team_list_.emplace_back(player); }
  const std::vector<const Player*> GetTeamList() { return team_list_; }

  void ClearEnemyList() { enemy_list_.clear(); }
  void PushEnemyList(const Player* player) { enemy_list_.emplace_back(player); }
  const std::vector<const Player*>& GetEnemyList() { return enemy_list_; }

  void ClearCombinedList() { combined_list_.clear(); }
  void PushCombinedList(const Player* player) { combined_list_.emplace_back(player); }
  const std::vector<const Player*>& GetCombinedList() { return combined_list_; }

#if 0 
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
  void SetDefaultValue(const BB& key, const T& value) {
    data2_default_[key] = std::any(value);
  }

  void SetToDefault(const BB& key) { 
      auto iter = data2_default_.find(key);

    if (iter == data2_default_.end()) {
      return;
    }

    data2_[key] = iter->second;
  }

  void SetAllToDefault() {

    for (short key = 0; key != (short)BB::End; ++key) {
      auto iter = data2_default_.find((BB)key);

      if (iter == data2_default_.end()) {
        continue;
      }

      data2_[(BB)key] = iter->second;
    }
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
#endif





  // std::unordered_map<std::string, std::any> data_;
  // std::unordered_map<BB, std::any> data2_;
  // std::unordered_map<BB, std::any> data2_default_;
};

}  // namespace marvin
