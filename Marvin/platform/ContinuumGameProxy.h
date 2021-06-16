#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "../GameProxy.h"
#include "ExeProcess.h"

namespace marvin {

#pragma pack(push, 1)
struct WeaponData {
  u32 _unused1;

  u32 x;  // 0x04
  u32 y;  // 0x08

  u32 _unuseda;
  i32 velocity_x;
  i32 velocity_y;
  u32 _unused2[32];

  u32 pid;  // 0x98

  char _unused3[11];

  u16 type;
};
#pragma pack(pop)

// In memory weapon data
class ContinuumWeapon : public Weapon {
 public:
  ContinuumWeapon(WeaponData* data) : weapon_(data) {}

  u16 GetPlayerId() const { return weapon_->pid; }

  Vector2f GetPosition() const { return Vector2f(weapon_->x / 1000.0f / 16.0f, weapon_->y / 1000.0f / 16.0f); }

  Vector2f GetVelocity() const {
    return Vector2f(weapon_->velocity_x / 1000.0f / 16.0f, weapon_->velocity_y / 1000.0f / 16.0f);
  }

  u16 GetType() const { return weapon_->type; }

 private:
  WeaponData* weapon_;
};

class ContinuumGameProxy : public GameProxy {
 public:
  ContinuumGameProxy(HWND hwnd);
  void LoadGame();

  bool Update(float dt) override;

  std::string GetName() const override;
  Chat GetChat() const override;
  int GetEnergy() const override;
  const float GetEnergyPercent() override;
  Vector2f GetPosition() const override;
  const std::vector<Player>& GetPlayers() const override;
  const ClientSettings& GetSettings() const override;
  const ShipSettings& GetShipSettings() const override;
  const ShipSettings& GetShipSettings(int ship) const override;
  const float GetMaxEnergy() override;
  const float GetRotation() override;
  const float GetMaxSpeed() override;
  const std::string GetZone() override;
  const std::string GetMapFile() const override;
  const Map& GetMap() const override;
  void SetTileId(Vector2f position, u8 id) override;
  const Player& GetPlayer() const override;
  int64_t TickerPosition() override;
  const Player& GetSelectedPlayer() const override;
  const uint32_t GetSelectedPlayerIndex() const override;
  const Player* GetPlayerById(u16 id) const override;
  const std::vector<BallData>& GetBalls() const override;

  std::vector<Weapon*> GetWeapons() override;

  void SetEnergy(float percent) override;
  void SetFreq(int freq) override;
  bool SetShip(uint16_t ship) override;
  void Warp() override;
  void Stealth() override;
  void Cloak(KeyController& keys) override;
  void MultiFire() override;
  void Flag() override;
  bool Spec() override;
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
  void SetSelectedPlayer(uint16_t id) override;

  void SetWindowFocus() override;

  ExeProcess& GetProcess();

 private:
  std::string GetServerFolder();
  void SetZone();
  void FetchChat();
  void FetchPlayers();
  void FetchBallData();
  void FetchWeapons();
  void SendKey(int vKey);

  std::vector<BallData> balls_;
  Chat chat_;
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
  std::string zone_;
};

}  // namespace marvin
