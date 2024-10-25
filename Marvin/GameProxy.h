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
enum class ConnectState : u32 {
  // This is the ConnectState when the client is on the menu and not attempting to join any zone.
  None = 0,
  // @4478D8
  Connecting,
  // @451962
  Connected,
  // @42284F
  // This is used for news and biller popups.
  JoiningZone,
  // @42E0C0
  // The client will transition to this while switching arenas.
  JoiningArena,
  // @436B36
  // This state also includes map downloading.
  Playing,
  // @451AFD
  // Normally this only occurs when the server sends the disconnect packet, but Fuse will set it
  // to this if no packet is received within 1500 ticks.
  // This is the same tick count that Continuum uses for the "No data" notification.
  Disconnected
};
enum class UpdateState : short { Clear, Wait, Reload };
enum class ItemAction : short {Buy, Sell, DepotBuy, DepotSell, ListItems, ListSlots, None};
enum class CombatRole : short { Anchor, Rusher, Bomber, Gunner, Flagger, Turret, EMP, PowerBaller, None };
enum class CombatDifficulty : short { Nerf, Normal, None };
enum class BDWarpTo : short { Center, Base, None };
enum class BDState : short { Start, Running, Paused, Stopped, Ended };
enum class CommandRequestType : short { ShipChange, ArenaChange, FreqChange, None };
enum class WeaponType : short { None, Bullet, BouncingBullet, Bomb, ProximityBomb, Repel, Decoy, Burst, Thor };
enum class AnchorType : short { Summoner, Evoker, None};
enum class Ship : uint16_t { Warbird, Javelin, Spider, Leviathan, Terrier, Weasel, Lancaster, Shark, Spectator };
enum class ChatType {
  Arena,
  PublicMacro,
  Public,
  Team,
  OtherTeam,
  Private,
  RedWarning,
  RemotePrivate,
  RedError,
  Channel,
  Fuchsia = 79
};

enum StatusFlag {
  Status_Stealth = (1 << 0),
  Status_Cloak = (1 << 1),
  Status_XRadar = (1 << 2),
  Status_Antiwarp = (1 << 3),
  Status_Flash = (1 << 4),
  Status_Safety = (1 << 5),
  Status_UFO = (1 << 6),
  Status_InputChange = (1 << 7)
};

struct HSBuySellList {
  std::vector<std::string> items;
  std::string sender;
  ItemAction action = ItemAction::None;
  bool message_sent = false;
  bool set_ship_sent = false;
  int count = 0;
  uint64_t timestamp = 0;
  uint64_t allowed_time = 5000;
  uint16_t ship = 0;

  void Clear() {
    items.clear();
    sender.clear();
    action = ItemAction::None;
    message_sent = false;
    set_ship_sent = false;
    count = 0;
    timestamp = 0;
    ship = 0;
    allowed_time = 5000;
  }
};

struct AnchorSet {
  std::vector<const Player*> full_energy;
  std::vector<const Player*> no_energy;
  void Clear() { 
    full_energy.clear();
    no_energy.clear();
  }
};

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
  virtual float GetAliveMilliSeconds() const = 0;
  virtual float GetRemainingMilliSeconds() const = 0;
  virtual s32 GetRemainingBounces() const = 0;

 inline bool IsMine() {
    WeaponData data = GetData();
    return data.alternate && (data.type == WeaponType::Bomb || data.type == WeaponType::ProximityBomb);
  }
};



// if the status is on or off
struct ShipToggleStatus {
  u8 stealth : 1;
  u8 cloak : 1;
  u8 xradar : 1;
  u8 antiwarp : 1;
  u8 flash : 1;
  u8 safety : 1;
  u8 ufo : 1;
  u8 inputChange : 1;
};

struct Flag {
  u32 id;
  u32 frequency;
  Vector2f position;
  Flag(u32 id, u32 freq, Vector2f pos) : id(id), frequency(freq), position(pos) {}
  bool IsNeutral() const { return frequency == 0xFFFFFFFF; }
};

struct Green {
  s32 prize_id;
  u32 x;
  u32 y;
  u32 remaining_ticks;
};

struct ChatMessage {
  ChatMessage() : message(""), player(""), type(ChatType::Arena) {}
  std::string message;
  std::string player;
  ChatType type;
};

class GameProxy {
 public:
  virtual ~GameProxy() {}

  virtual UpdateState Update() = 0;

  virtual std::string GetName() const = 0;
  //virtual HWND GetGameWindowHandle() = 0;
  virtual int GetEnergy() const = 0;
  virtual const float GetEnergyPercent() = 0;
  virtual Vector2f GetPosition() const = 0;
  //virtual const ShipFlightStatus& GetShipStatus() const = 0;

  virtual const Player& GetPlayer() const = 0;
  virtual const std::vector<Player>& GetPlayers() const = 0;

  virtual const ClientSettings& GetSettings() const = 0;
  virtual const ShipSettings& GetShipSettings() const = 0;
  virtual const ShipSettings& GetShipSettings(int ship) const = 0;

  virtual const float GetMaxEnergy() = 0;
  virtual const float GetRotation() = 0;
  virtual const float GetRadius() = 0;
  virtual const float GetMaxSpeed() = 0;
  virtual const float GetMaxSpeed(u16 ship) = 0;
  virtual const float GetThrust() = 0;
  virtual const uint64_t GetRespawnTime() = 0;


  virtual const Zone GetZone() = 0;
  virtual const std::string GetZoneName() = 0;
  virtual const std::string GetMapFile() const = 0;
  virtual const Map& GetMap() const = 0;

  virtual ChatMessage FindChatMessage(std::string match) = 0;
  virtual std::vector<ChatMessage> GetChatHistory() = 0;
  virtual std::vector<ChatMessage> GetCurrentChat() = 0;
  virtual void SendChatMessage(const std::string& mesg) = 0;
  virtual void SendPrivateMessage(const std::string& target, const std::string& mesg) = 0;
  virtual void SendPriorityMessage(const std::string& message) = 0;
  virtual void SendQueuedMessage(const std::string& mesg) = 0;
  virtual void SendKey(int vKey) = 0;

  virtual void SetTileId(Vector2f position, u8 id) = 0;
  
  //virtual int64_t TickerPosition() = 0;
  virtual const Player& GetSelectedPlayer() const = 0;
  //virtual const uint32_t GetSelectedPlayerIndex() const = 0;

  virtual const Player* GetPlayerById(u16 id) const = 0;
  virtual const Player* GetPlayerByName(std::string_view name) const = 0;

  virtual std::vector<Weapon*> GetWeapons() = 0;
  virtual std::vector<Weapon*> GetEnemyMines() = 0;
  virtual const std::vector<BallData>& GetBalls() const = 0;
  virtual const std::vector<Green>& GetGreens() const = 0;
  virtual const std::vector<Flag>& GetDroppedFlags() = 0;

  virtual void SetWindowFocus() = 0;

  // May need to be called more than once to transition the game menu
  // Returns true if it attempts to set the ship this call.
  virtual void SetEnergy(uint64_t percent, std::string reason) = 0;
  virtual void SetVelocity(Vector2f desired) = 0;
  virtual void SetPosition(Vector2f desired) = 0;
  virtual void SetSpeed(float desired) = 0;
  virtual void SetThrust(uint32_t desired) = 0;
  virtual bool SetShip(uint16_t ship) = 0;
  virtual bool ResetShip() = 0;
  virtual void SetFreq(int freq) = 0;
  virtual void SetArena(const std::string& arena) = 0;
  virtual void ResetStatus() = 0;
  virtual void SetStatus(StatusFlag status, bool on_off) = 0;
  virtual void Attach(std::string name) = 0;
  virtual void Attach(uint16_t id) = 0;
  virtual void Attach() = 0;
  virtual void HSFlag() = 0;
  virtual void Warp() = 0;
  virtual void Stealth() = 0;
  virtual void Cloak(KeyController& keys) = 0;
  virtual void MultiFire() = 0;
  virtual void XRadar() = 0;
  virtual void Antiwarp(KeyController& keys) = 0;
  virtual void Burst(KeyController& keys) = 0;
  virtual void Repel(KeyController& keys) = 0;
  virtual void SetSelectedPlayer(uint16_t id) = 0;
  virtual std::size_t GetIDIndex() = 0;

};

}  // namespace marvin
