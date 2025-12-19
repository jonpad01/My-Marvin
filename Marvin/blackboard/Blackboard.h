#ifndef BLACKBOARD_H
#define BLACKBOARD_H

#include <any>
#include <optional>
#include <string>
#include <array>

#include "../GameProxy.h"

/*
* Marvins behavior tree is designed to update 60 or more times per second.
* The nodes execute and then die, so they need a place to store data
* The blackboard is a data set that lives beyond the life of the behavior tree nodes
* The blackboard uses an array that is paired to an enum key
* No looping to find data, the key provides direct access to the array element
* The enum key should be more error safe as typos won't compile (vs a string key)
* To add new keys, they must be entered into the BBKey enum
* The blackboard should only handle small data because it cant return a reference to its data
*/

// TODO:  update to method that enforces data type at compile time?

namespace marvin {

    struct BBSlot {
      std::any value;
      bool hasValue = false;
    };


    enum class BBKey : std::size_t {
      // consumables
      UseRepel, UseBurst, UseDecoy, UseRocket, UseThor, UseBrick, UsePortal,
      // status
      UseMultiFire, UseCloak, UseStealth, UseXRadar, UseAntiWarp,
      // team basing stuff
      PubEventTeam0, PubEventTeam1,
      EnemyGuardPoint, TeamGuardPoint, BaseIndex,
      CombatRole,
      AnchorDistanceAdjustment, AllowLagAttaching,
      TeamAnchor, EnemyAnchor,
      TeamPlayerList, EnemyTeamPlayerList, TeamShipCount,
      TeamInBase, EnemyInBase, LastInBase,
      FlyInReverse, ActivateSwarm,
      // hyperspace stuff
      ReadingAnchorList, AnchorHasSummoner, FlaggingEnabled,
      // balling stuff
      TargetBall,
      // centering stuff
      PatrolIndex, PatrolNodes, FreqPopulationCount, CenterSpawnPoint,
      FrequencyLocked,
      // enemy targeting stuff
      TargetPlayer, AimingSolution, CombatDifficulty, TargetInSight,
      BombTimer,
      // deva baseduel hosting
      BDHostingState, BDWarpToArea, BDTeam0Score, BDTeam1Score, BDBaseIndex,
      // staff stuff
      StaffChatAnnouncments, CommandLocked,
      // timer stuff
      SetShipCoolDown,
      // command stuff
      RequestedCommand, RequestedArena, RequestedShip, RequestedFreq,
      COUNT
    };


class Blackboard {
 private:
  HSBuySellList hs_items_; 
 public:
 
  void SetHSBuySellList(const std::vector<std::string>& items) { hs_items_.items = items; }
  void SetHSBuySellAction(HSItemTransaction action) { hs_items_.action = action; }
  void SetHSBuySellActionCount(int count) { hs_items_.count = count; }
  void SetHSBuySellShip(uint16_t ship) { hs_items_.ship = ship; }
  void SetHSBuySellTimeStamp(uint64_t timestamp) { hs_items_.timestamp = timestamp; }
  void SetHSBuySellAllowedTime(uint64_t time) { hs_items_.allowed_time = time; }
  void SetHSBuySellMessageSent(bool state) { hs_items_.message_sent = state; }
  void SetHSBuySellSetShipSent(bool state) { hs_items_.set_ship_sent = state; }
  void SetHSBuySellSender(const std::string& sender) { hs_items_.sender = sender; }
  void ClearHSBuySellList() { hs_items_.items.clear(); }
  void ClearHSBuySellAll() { hs_items_.Clear(); }
  void EmplaceHSBuySellList(const std::string& item) { hs_items_.items.emplace_back(item); }
  const HSBuySellList& GetHSBuySellList() { return hs_items_; }


  bool Has(BBKey key) { return data[(size_t)key].hasValue; }

  template <typename T>
  void Set(BBKey key, const T& value) {
    auto& slot = data[(size_t)key];
    slot.value = std::move(value);
    slot.hasValue = true;
  }

  template <typename T>
  void SetDefaultValue(BBKey key, const T& value) {
    auto& slot = default_data[(size_t)key];
    slot.value = std::move(value);
    slot.hasValue = true;
  }

  void SetToDefault(BBKey key) {
    auto& default_slot = default_data[(size_t)key];
    if (!default_slot.hasValue) return;
    auto& slot = data[(size_t)key];
    slot.value = std::move(default_slot.value);
    slot.hasValue = default_slot.hasValue;
  }

  void SetAllToDefault() {
    for (std::size_t i = 0; i < (size_t)BBKey::COUNT; ++i) {
      auto& default_slot = default_data[i];
      if (!default_slot.hasValue) return;
      auto& slot = data[i];
      slot.value = std::move(default_slot.value);
      slot.hasValue = default_slot.hasValue;
    }
  }

  template <typename T>
  std::optional<T> Value(BBKey key) {
    auto& slot = data[(size_t)key];

    auto& any = slot.value;
    if (!slot.hasValue) return std::nullopt;

    try {
      return std::any_cast<T>(any);
    } catch (const std::bad_any_cast&) {
      return std::nullopt;
    }
  }

  template <typename T>
  T ValueOr(BBKey key, const T& or_result) {
    return Value<T>(key).value_or(or_result);
  }

void Clear() {
  for (std::size_t i = 0; i < (size_t)BBKey::COUNT; i++) {
      data[i].hasValue = false;
  }
}

void Erase(BBKey key) { 
    data[(size_t)key].hasValue = false; 
}

private:

   std::array<BBSlot, static_cast<size_t>(BBKey::COUNT)> data;
   std::array<BBSlot, static_cast<size_t>(BBKey::COUNT)> default_data;
};

}  // namespace marvin


#endif