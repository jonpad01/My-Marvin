#pragma once

#include <cstddef>
#include <memory>

#include "../GameProxy.h"
#include "ExeProcess.h"

namespace marvin {
    
class ContinuumGameProxy : public GameProxy {
 public:
     
  ContinuumGameProxy(HWND hwnd);

  void Update(float dt) override;

  std::string GetName() const override;
  int GetEnergy() const override;
  Vector2f GetPosition() const override;
  const std::vector<Player>& GetPlayers() const override;
  const ClientSettings& GetSettings() const override;
  const ShipSettings& GetShipSettings() const override;
  const ShipSettings& GetShipSettings(int ship) const override;
  std::string GetServerFolder() const;
  std::string Zone() override;
  const Map& GetMap() const override;
  const Player& GetPlayer() const override;
  

  void SetFreq(int freq) override;
  bool SetShip(int ship) override;
  void Warp() override;
  void Stealth() override;
  void Cloak(KeyController& keys) override;
  void MultiFire() override;
  void Flag() override;
  bool Spec() override;
  void Attach(std::string name) override;
  std::string TickName() override;
  void PageUp() override;
  void PageDown() override;
  void XRadar() override;
  void Burst(KeyController& keys) override;
  void F7() override;

  void SetWindowFocus();

  ExeProcess& GetProcess();

 private:
  void FetchPlayers();
  void SendKey(int vKey);

  ExeProcess process_;
  HWND hwnd_;
  std::size_t module_base_continuum_;
  std::size_t module_base_menu_;
  std::size_t game_addr_;
  uint32_t* position_data_;
  uint16_t player_id_;
  std::unique_ptr<Map> map_;
  Player* player_;
  std::vector<Player> players_;
};

}  // namespace marvin