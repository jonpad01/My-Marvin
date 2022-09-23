#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "../GameProxy.h"
#include "ExeProcess.h"

namespace marvin {

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
  ContinuumWeapon(WeaponMemory* data, u32 remaining_ticks) : weapon_(data), remaining_ticks_(remaining_ticks) {}

  u16 GetPlayerId() const { return weapon_->pid; }

  Vector2f GetPosition() const { return Vector2f(weapon_->x / 1000.0f / 16.0f, weapon_->y / 1000.0f / 16.0f); }

  Vector2f GetVelocity() const {
    return Vector2f(weapon_->velocity_x / 10.0f / 16.0f, weapon_->velocity_y / 10.0f / 16.0f);
  }

  WeaponData GetData() const { return weapon_->data; }

  u32 GetAliveTicks() const { return weapon_->alive_ticks; }
  u32 GetRemainingTicks() const { return remaining_ticks_; }
  s32 GetRemainingBounces() const { return weapon_->remaining_bounces; }

 private:
  WeaponMemory* weapon_;
  u32 remaining_ticks_;
};

class ContinuumGameProxy : public GameProxy {
 public:
  ContinuumGameProxy(HWND hwnd);
  void LoadGame();

  bool Update(float dt) override;

  std::string GetName() const override;
  std::vector<ChatMessage> GetChat() const override;
  int GetEnergy() const override;
  const float GetEnergyPercent() override;
  Vector2f GetPosition() const override;
  const ShipStatus& GetShipStatus() const override;

   const Player& GetPlayer() const override;
  const std::vector<Player>& GetPlayers() const override;
  const std::vector<Player>& GetTeam() const override;
  const std::vector<Player>& GetEnemies() const override;
  const std::vector<Player>& GetEnemyTeam() const override;

  const ClientSettings& GetSettings() const override;
  const ShipSettings& GetShipSettings() const override;
  const ShipSettings& GetShipSettings(int ship) const override;

  const float GetMaxEnergy() override;
  const float GetThrust() override;
  const float GetRotation() override;
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

  const std::vector<BallData>& GetBalls() const override;
  const std::vector<Green>& GetGreens() const override;
  std::vector<Weapon*> GetWeapons() override;
  std::vector<marvin::Flag> GetDroppedFlags() override;

  void SetEnergy(float percent) override;
  void SetFreq(int freq) override;
  bool SetShip(uint16_t ship) override;
  void Warp() override;
  void Stealth() override;
  void Cloak(KeyController& keys) override;
  void MultiFire() override;
  void Flag() override;
  void Attach(std::string name) override;
  void P() override;
  void L() override;
  void R() override;
  void PageUp() override;
  void PageDown() override;
  void XRadar() override;
  void Burst(KeyController& keys) override;
  void Repel(KeyController& keys) override;
  void F7() override;
  void SendChatMessage(const std::string& mesg) const override;
  void SendPrivateMessage(const std::string& target, const std::string& mesg) const override;
  void SetSelectedPlayer(uint16_t id) override;

  void SetWindowFocus() override;

  ExeProcess& GetProcess();

 private:
  std::string GetServerFolder();
  void SetZone();
  void FetchChat();
  void FetchPlayers();
  void SortPlayers();
  void FetchBallData();
  void FetchGreens();
  void FetchWeapons();
  void SendKey(int vKey);
  void LogMemoryLocations();

  
  std::vector<BallData> balls_;
  std::vector<Green> greens_;
  ShipStatus ship_status_;
  std::vector<ChatMessage> recent_chat_;
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
  std::vector<Player> team_;
  std::vector<Player> enemy_team_;
  std::vector<Player> enemies_;
  std::vector<ContinuumWeapon> weapons_;
  std::string mapfile_path_;
  Zone zone_;


  std::vector<std::size_t> offsets_{0x04, 0x10, 0x18, 0x20, 0x24,  0x30,  0x34,  0x3C, 0x40,
                                    0x4C, 0x5C, 0x58, 0x60, 0x6D, 0x178, 0x208, 0x20C, 0x2EC, 0x32C};

  std::vector<std::string> offset_titles_{"kPosOffset",
                                          "kVelocityOffset",
                                          "kIdOffset",
                                          "kBountyOffset1",
                                          "kBountyOffset2",
                                          "kFlagOffset1",
                                          "kFlagOffset2",
                                          "kRotOffset",
                                          "kActiveOffset1",
                                          "kActiveOffset2",
                                          "kShipOffset",
                                          "kFreqOffset",
                                          "kStatusOffset",
                                          "kNameOffset",
                                          "kDeadOffset",
                                          "kEnergyOffset1",
                                          "kEnergyOffset2",
                                          "kMultiFireCapableOffset",
                                          "kMultiFireStatusOffset"};
};

}  // namespace marvin
