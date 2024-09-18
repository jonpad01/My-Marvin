#pragma once

#include <fuse/ClientSettings.h>
#include <fuse/ExeProcess.h>
#include <fuse/HookInjection.h>
#include <fuse/Map.h>
#include <fuse/Player.h>
#include <fuse/Weapon.h>
#include <fuse/render/Renderer.h>

#include <memory>
#include <string>
#include <vector>

namespace fuse {

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

struct ShipCapability {
  u8 stealth : 1;
  u8 cloak : 1;
  u8 xradar : 1;
  u8 antiwarp : 1;
  u8 multifire : 1;
  u8 bouncing_bullets : 1;
  u8 proximity : 1;
  u8 padding : 1;
};

struct ShipStatus {
  u32 recharge = 0;
  u32 thrust = 0;
  u32 speed = 0;
  u32 rotation = 0;
  u32 shrapnel = 0;

  ShipCapability capability;
};

struct GameMemory {
  MemoryAddress module_base_continuum = 0;
  MemoryAddress module_base_menu = 0;
  MemoryAddress game_address = 0;
};

class Fuse {
 public:
  FUSE_EXPORT static Fuse& Get();

  FUSE_EXPORT void Inject();
  FUSE_EXPORT void Update();

  FUSE_EXPORT render::Renderer& GetRenderer() { return *renderer; }
  FUSE_EXPORT void SetRenderer(std::unique_ptr<render::Renderer> renderer) { this->renderer = std::move(renderer); }

  FUSE_EXPORT ExeProcess& GetExeProcess() { return exe_process; }

  // This returns true if the client is on the menu and isn't attempting to connect to a zone.
  FUSE_EXPORT bool IsOnMenu() const;
  // Returns true if in game and map isnt downloading
  FUSE_EXPORT bool IsInGame() const;
  FUSE_EXPORT ConnectState GetConnectState() const;

  FUSE_EXPORT bool IsGameMenuOpen() const;
  FUSE_EXPORT void SetGameMenuOpen(bool open);

  FUSE_EXPORT std::string GetZoneName() const;
  FUSE_EXPORT std::string GetArenaName() const;
  FUSE_EXPORT std::string GetMapName() const;

  FUSE_EXPORT Map& GetMap() { return map; }

  FUSE_EXPORT const ClientSettings& GetSettings() const;
  FUSE_EXPORT const ShipSettings& GetShipSettings() const;
  FUSE_EXPORT const ShipSettings& GetShipSettings(int ship) const;

  FUSE_EXPORT Player* GetPlayer() { return main_player; }
  FUSE_EXPORT const Player* GetPlayer() const { return main_player; }
  FUSE_EXPORT std::string GetName();

  FUSE_EXPORT void RegisterHook(std::unique_ptr<HookInjection> hook) { hooks.push_back(std::move(hook)); }

  FUSE_EXPORT const std::vector<std::unique_ptr<HookInjection>>& GetHooks() const { return hooks; }
  FUSE_EXPORT void ClearHooks() { hooks.clear(); }

  FUSE_EXPORT HWND GetGameWindowHandle();
  FUSE_EXPORT bool UpdateMemory();
  FUSE_EXPORT GameMemory& GetGameMemory() { return game_memory; }

  FUSE_EXPORT void HandleWindowsEvent(LPMSG msg, HWND hWnd);

  FUSE_EXPORT const ShipStatus& GetShipStatus() const { return ship_status; }

  FUSE_EXPORT const std::vector<Player>& GetPlayers() const { return players; }
  FUSE_EXPORT const std::vector<Weapon>& GetWeapons() const { return weapons; }

 private:
  Fuse();

  void ReadPlayers();
  void ReadWeapons();

  std::vector<std::unique_ptr<HookInjection>> hooks;

  std::vector<Player> players;
  std::vector<Weapon> weapons;

  std::unique_ptr<render::Renderer> renderer;

  Map map;

  ShipStatus ship_status;
  Player* main_player = nullptr;
  u16 player_id = 0xFFFF;

  ExeProcess exe_process;
  GameMemory game_memory;

  bool initialized = false;
};

}  // namespace fuse
