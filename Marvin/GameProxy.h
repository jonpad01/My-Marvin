#pragma once

#include <string>
#include <vector>
#include "KeyController.h"
#include "ClientSettings.h"
#include "Player.h"
#include "Vector2f.h"

namespace marvin {

class Map;

class GameProxy {
 public:
  virtual ~GameProxy() {}

  virtual void Update(float dt) = 0;

  virtual std::string GetName() const = 0;
  virtual int GetEnergy() const = 0;
  virtual Vector2f GetPosition() const = 0;
  virtual const std::vector<Player>& GetPlayers() const = 0;
  virtual const ClientSettings& GetSettings() const = 0;
  virtual const ShipSettings& GetShipSettings() const = 0;
  virtual const ShipSettings& GetShipSettings(int ship) const = 0;
  virtual const Map& GetMap() const = 0;
  virtual std::string Zone() = 0;
  virtual const Player& GetPlayer() const = 0;
  

  // May need to be called more than once to transition the game menu
  // Returns true if it attempts to set the ship this call.
  virtual bool SetShip(int ship) = 0;
  virtual void SetFreq(int freq) = 0;
  virtual void Warp() = 0;
  virtual void Stealth() = 0;
  virtual void Cloak(KeyController& keys) = 0;
  virtual void MultiFire() = 0;
  virtual void Flag() = 0;
  virtual bool Spec() = 0;
  virtual void Attach(std::string name) = 0;
  virtual std::string TickName() = 0;
  virtual void PageUp() = 0;
  virtual void PageDown() = 0;
  virtual void XRadar() = 0;
  virtual void Burst(KeyController& keys) = 0;
  virtual void F7() = 0;
};

}  // namespace marvin
