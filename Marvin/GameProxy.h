#pragma once

#include <string>
#include <vector>

#include "ClientSettings.h"
#include "KeyController.h"
#include "Player.h"
#include "Types.h"
#include "Vector2f.h"
#include "zones/Zone.h"

namespace marvin {

class Map;

enum class WeaponType : short { None, Bullet, BouncingBullet, Bomb, ProximityBomb, Repel, Decoy, Burst, Thor };

struct WeaponData {
  WeaponType type : 5;
  u16 level : 2;
  u16 shrapbouncing : 1;
  u16 shraplevel : 2;
  u16 shrap : 5;
  u16 alternate : 1;
};

class Weapon {
 public:
  virtual u16 GetPlayerId() const = 0;
  virtual Vector2f GetPosition() const = 0;
  virtual Vector2f GetVelocity() const = 0;
  virtual WeaponData GetData() const = 0;

  bool IsMine() {
    WeaponData data = GetData();

    return data.alternate && (data.type == WeaponType::Bomb || data.type == WeaponType::ProximityBomb);
  }
};

struct ChatMessage {
  ChatMessage() : message(""), player(""), type(0) {}
  std::string message;
  std::string player;
  int type;
};

class GameProxy {
 public:
  virtual ~GameProxy() {}

  virtual bool Update(float dt) = 0;

  virtual std::string GetName() const = 0;
  virtual std::vector<ChatMessage> GetChat() const = 0;
  virtual int GetEnergy() const = 0;
  virtual const float GetEnergyPercent() = 0;
  virtual Vector2f GetPosition() const = 0;
  virtual const std::vector<Player>& GetPlayers() const = 0;
  virtual const ClientSettings& GetSettings() const = 0;
  virtual const ShipSettings& GetShipSettings() const = 0;
  virtual const ShipSettings& GetShipSettings(int ship) const = 0;
  virtual const float GetMaxEnergy() = 0;
  virtual const float GetRotation() = 0;
  virtual const float GetMaxSpeed() = 0;
  virtual const Zone GetZone() = 0;
  virtual const std::string GetMapFile() const = 0;
  virtual const Map& GetMap() const = 0;
  virtual void SetTileId(Vector2f position, u8 id) = 0;
  virtual const Player& GetPlayer() const = 0;
  virtual void SendChatMessage(const std::string& mesg) const = 0;
  virtual void SendPrivateMessage(const std::string& target, const std::string& mesg) const = 0;
  virtual int64_t TickerPosition() = 0;
  virtual const Player& GetSelectedPlayer() const = 0;
  virtual const uint32_t GetSelectedPlayerIndex() const = 0;
  virtual const Player* GetPlayerById(u16 id) const = 0;
  virtual std::vector<Weapon*> GetWeapons() = 0;
  virtual const std::vector<BallData>& GetBalls() const = 0;
  virtual void SetWindowFocus() = 0;

  // May need to be called more than once to transition the game menu
  // Returns true if it attempts to set the ship this call.
  virtual void SetEnergy(float percent) = 0;
  virtual bool SetShip(uint16_t ship) = 0;
  virtual void SetFreq(int freq) = 0;
  virtual void Warp() = 0;
  virtual void Stealth() = 0;
  virtual void Cloak(KeyController& keys) = 0;
  virtual void MultiFire() = 0;
  virtual void Flag() = 0;
  virtual bool Spec() = 0;
  virtual void Attach(std::string name) = 0;
  virtual void P() = 0;
  virtual void L() = 0;
  virtual void R() = 0;
  virtual void PageUp() = 0;
  virtual void PageDown() = 0;
  virtual void XRadar() = 0;
  virtual void Burst(KeyController& keys) = 0;
  virtual void Repel(KeyController& keys) = 0;
  virtual void F7() = 0;
  virtual void SetSelectedPlayer(uint16_t id) = 0;
};

}  // namespace marvin
