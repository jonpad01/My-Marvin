#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <deque>

#include "../GameProxy.h"
#include "ExeProcess.h"
#include "../Time.h"

namespace marvin {



struct DoorState {
  s16 door_mode;
  u32 door_seed;

  // Doors are considered changed if the mode changes, or if the seed changes while set to randomized.
  bool operator!=(const DoorState& previous) const {
    if (previous.door_mode != door_mode) return true;
    return door_mode < 0 && door_seed != previous.door_seed;
  }

  bool operator==(const DoorState& previous) const { return !(*this != previous); }

  static DoorState Load() {
    DoorState result = {};

    result.door_mode = *(s16*)((*(u32*)0x4c1afc) + 0x156D0);
    result.door_seed = *(u32*)((*(u32*)0x4c1afc) + 0x12F40);

    return result;
  }
};

#pragma pack(push, 1)
struct WeaponMemory {
  u32 _unused1;  // 0x00
  u32 x;  // 0x04
  u32 y;  // 0x08
  u32 _unuseda;    // 0x0C
  i32 velocity_x;  // 0x10
  i32 velocity_y;  // 0x14
  u32 _unused2[29];
  s32 remaining_bounces;  // 0x8C
  u32 _unused3[2];
  u32 pid;  // 0x98
  char _unused4[11];
  WeaponData data;  // a7-a9
  char _unused5[23];
  u32 alive_ticks;
};
#pragma pack(pop)

static_assert(sizeof(WeaponMemory) > 0x40);
static_assert(offsetof(WeaponMemory, remaining_bounces) == 0x8C);
static_assert(offsetof(WeaponMemory, pid) == 0x98);
static_assert(offsetof(WeaponMemory, data) == 0xA7);

// In memory weapon data
class ContinuumWeapon : public Weapon {
 public:
  ContinuumWeapon(WeaponMemory* data, float remaining_milliseconds_)
      : weapon_(data), remaining_milliseconds_(remaining_milliseconds_) {}

  u16 GetPlayerId() const { return weapon_->pid; }

  Vector2f GetPosition() const { return Vector2f(weapon_->x / 1000.0f / 16.0f, weapon_->y / 1000.0f / 16.0f); }

  Vector2f GetVelocity() const {
    return Vector2f(weapon_->velocity_x / 10.0f / 16.0f, weapon_->velocity_y / 10.0f / 16.0f);
  }

  WeaponData GetData() const { return weapon_->data; }

  float GetAliveMilliSeconds() const { return weapon_->alive_ticks / 100.0f; }
  float GetRemainingMilliSeconds() const { return remaining_milliseconds_; }
  s32 GetRemainingBounces() const { return weapon_->remaining_bounces; }

 private:
  WeaponMemory* weapon_;
  float remaining_milliseconds_;
};

class ContinuumGameProxy : public GameProxy {
 public:
  ContinuumGameProxy(const std::string& name);

  bool IsLoaded() override;
  bool UpdateMemory();

  bool Update();


  ConnectState GetConnectState();
  void ExitGame();
  bool GameIsClosing();
  

  std::string GetName() override;
  void SetName(const std::string& name);
  
  ChatMessage FindChatMessage(std::string match) override;
  std::vector<ChatMessage> GetCurrentChat() override;
  std::vector<ChatMessage> GetChatHistory() override;
  int GetEnergy() const override;
  const float GetEnergyPercent() override;
  Vector2f GetPosition() const override;
  HWND GetGameWindowHandle();
  //const ShipFlightStatus& GetShipStatus() const override;

  const Player& GetPlayer() const override;
  const std::vector<Player>& GetPlayers() const override;

  const float GetMaxEnergy() override;
  const float GetThrust() override;
  const float GetRotation() override;
  const float GetRadius() override;
  const float GetMaxSpeed() override;
  const float GetMaxSpeed(u16 ship) override;
  const uint64_t GetRespawnTime() override;

  const Zone GetZone() override;
  const std::string GetZoneName() override;
  const std::string GetMapFile() const override;
  const Map& GetMap() const override;

  void SetTileId(Vector2f position, u8 id) override;

 
  const Player& GetSelectedPlayer() const override;
  const Player* GetPlayerById(u16 id) const override;
  const Player* GetPlayerByName(const std::string& name) const override;

  const std::vector<BallData>& GetBalls() const override;
  const std::vector<Green>& GetGreens() const override;
  std::vector<Weapon*> GetWeapons() override;
  std::vector<Weapon*> GetEnemyMines() override;
  const std::vector<Flag>& GetDroppedFlags() override;

  void SetEnergy(uint64_t percent, std::string reason) override;
  void SetFreq(int freq) override;
  void SetShip(uint16_t ship) override;
  void SetArena(const std::string& arena) override;
  void ResetStatus() override;
  bool ResetShip() override;
  void Attach(std::string name) override;
  void Attach(uint16_t id) override;
  void Attach() override;
  void HSFlag() override;
  void Warp() override;
  void Stealth() override;
  void Cloak(KeyController& keys) override;
  void MultiFire() override;
  void XRadar() override;
  void Antiwarp(KeyController& keys) override;
  void Burst(KeyController& keys) override;
  void Repel(KeyController& keys) override;
  bool ProcessQueuedMessages();
  void SendPriorityMessage(const std::string& message) override;
  void SendQueuedMessage(const std::string& mesg) override;
  void SendChatMessage(const std::string& mesg) override;
  void SendPrivateMessage(const std::string& target, const std::string& mesg) override;
  void SendKey(int vKey) override;
  void SetSelectedPlayer(uint16_t id) override;


  void SetStatus(StatusFlag status, bool on_off) override;
  void SetVelocity(Vector2f desired) override;
  void SetPosition(Vector2f desired) override;
  void SetSpeed(float desired) override;
  void SetThrust(uint32_t desired) override;

  
  bool IsInGame();
  

  void SetWindowFocus() override;

  ExeProcess& GetProcess();

 private:

  struct ChatEntry {
    char message[256];
    char player[24];
    char unknown[8];
    /*  Arena = 0,
      Public = 2,
      Private = 5,
      Channel = 9 */
    unsigned char type;
    char unknown2[3];
  };

  const ClientSettings& GetSettings() const override;
  const ShipSettings& GetShipSettings() const override;
  const ShipSettings& GetShipSettings(int ship) const override;

  std::string GetServerFolder();

  void SetDefaultWeaponLevels(Player& player);
  void FetchZone();
  void FetchChat();
  void FetchPlayers();
  void FetchBallData();
  void FetchDroppedFlags();
  void FetchGreens();
  void FetchWeapons();

  bool LimitMessageRate();
  bool IsNotRateLimited();  // Zone allows bot to bypass rate limter for chat.
  
  std::vector<BallData> balls_;
  std::vector<Green> greens_;
  std::vector<Flag> dropped_flags_;
  std::vector<ContinuumWeapon> enemy_mines_;
  ShipCapability capability_{0, 0, 0, 0, 0, 0, 0, 0};

  std::vector<ChatMessage> chat_;
  std::vector<ChatMessage> current_chat_;
  std::size_t chat_index_ = 0;
  std::deque<std::string> message_queue_;
  std::deque<std::string> priority_message_queue_;
  int sent_message_count_ = 0;

  ExeProcess process_;
  std::size_t module_base_continuum_ = 0;
  
  std::size_t game_addr_ = 0;
  std::size_t player_addr_ = 0;
  uint32_t* position_data_ = nullptr;

  uint16_t player_id_ = 0xFFFF;
  std::string player_name_;  // should be set once and never changed
  int player_generation_id_ = 0;
  std::unique_ptr<Map> map_ = nullptr;
  Player* player_ = nullptr;

  std::vector<Player> players_;
  std::vector<ContinuumWeapon> weapons_;

  std::string mapfile_path_;
  Zone zone_ = Zone::Other;
  std::string zone_name_;

  Time time_;

  uint16_t prev_ship_ = 0;
  int reset_ship_index_ = 0;
  bool set_freq_ = false;
  bool set_arena_ = false;
  bool hs_flag_ = false;
  uint16_t desired_freq_ = 0;
  std::string desired_arena_;

  
  uint64_t attach_cooldown_ = 0;
  uint64_t hs_flag_cooldown_ = 0;
  uint64_t setship_cooldown_ = 0;
  uint64_t message_cooldown_ = 0;
  
};

}  // namespace marvin
