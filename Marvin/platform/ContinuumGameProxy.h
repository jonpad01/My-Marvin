#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <deque>

#include "../GameProxy.h"
#include "ExeProcess.h"
#include "../Time.h"

namespace marvin {

enum class SetShipStatus : short { Clear, SetShip, ResetShip };
enum class ChatStatus : short { Unread, ClearChat };

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
  ContinuumGameProxy(HWND hwnd);
  bool LoadGame();

  UpdateState Update(float dt) override;

  std::string GetName() const override;
  std::vector<ChatMessage> GetChat() override;
  ChatMessage GetNextCommand() override;
  int GetEnergy() const override;
  const float GetEnergyPercent() override;
  Vector2f GetPosition() const override;
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
  const std::string GetMapFile() const override;
  const Map& GetMap() const override;

  void SetTileId(Vector2f position, u8 id) override;

 
  int64_t TickerPosition() override;
  const Player& GetSelectedPlayer() const override;
  const uint32_t GetSelectedPlayerIndex() const override;
  const Player* GetPlayerById(u16 id) const override;
  const Player* GetPlayerByName(std::string_view name) const override;

  const std::vector<BallData>& GetBalls() const override;
  const std::vector<Green>& GetGreens() const override;
  std::vector<Weapon*> GetWeapons() override;
  std::vector<Weapon*> GetEnemyMines() override;
  const std::vector<Flag>& GetDroppedFlags() override;

  void SetEnergy(uint64_t percent, std::string reason) override;
  void SetFreq(int freq) override;
  bool SetShip(uint16_t ship) override;
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
  void SendKey(int vKey) const override;
  void SetSelectedPlayer(uint16_t id) override;
  std::size_t GetIDIndex() override;
  void SetStatus(StatusFlag status, bool on_off) override;
  void SetVelocity(Vector2f desired) override;
  void SetPosition(Vector2f desired) override;
  void SetSpeed(float desired) override;
  void SetThrust(uint32_t desired) override;

 
  

  void SetWindowFocus() override;

  ExeProcess& GetProcess();

 private:

  const ClientSettings& GetSettings() const override;
  const ShipSettings& GetShipSettings() const override;
  const ShipSettings& GetShipSettings(int ship) const override;

  std::string GetServerFolder();
  void SetZone();
  void FetchChat();
  void FetchPlayers();
  void FetchBallData();
  void FetchDroppedFlags();
  void FetchGreens();
  void FetchWeapons();

  bool ActionDelay();

  bool LimitMessageRate();
  bool IsKnownBot();  // Zone allows bot to bypass rate limter for chat.
  
  std::vector<BallData> balls_;
  std::vector<Green> greens_;
  std::vector<Flag> dropped_flags_;
  std::vector<ContinuumWeapon> enemy_mines_;
 // ShipFlightStatus ship_status_;
  ShipCapability capability_;
  std::vector<ChatMessage> recent_chat_;
  std::vector<ChatMessage> player_commands_;
  std::size_t chat_index_;
  ExeProcess process_;
  HWND hwnd_;
  std::size_t module_base_continuum_;
  std::size_t module_base_menu_;
  std::size_t game_addr_;
  std::size_t player_addr_;
  uint32_t* position_data_;
  uint16_t player_id_;
  std::unique_ptr<Map> map_;
  Player* player_;
  std::vector<Player> players_;
  std::vector<ContinuumWeapon> weapons_;
  std::string mapfile_path_;
  Zone zone_;
  Time time_;
  SetShipStatus set_ship_status_;
  SetShipStatus reset_ship_status_;
  bool set_freq_flag_;
  int tries_;
  int sent_message_count_;
  UpdateState game_status_;
  ChatStatus chat_status_;
  uint64_t attach_cooldown_;
  uint64_t flag_cooldown_;
  uint64_t setfreq_cooldown_;
  uint64_t setship_cooldown_;
  uint64_t message_cooldown_;
  uint64_t delay_timer_;
  uint16_t desired_ship_;
  uint16_t desired_freq_;
  uint16_t original_ship_;
  std::vector<uint16_t> id_list_;
  std::vector<std::string> name_list_;
  std::deque<std::string> message_queue_;
  std::deque<std::string> priority_message_queue_;
};

}  // namespace marvin
