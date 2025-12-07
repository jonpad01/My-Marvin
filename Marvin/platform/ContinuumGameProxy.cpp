#include "ContinuumGameProxy.h"

#include <thread>
#include <vector>

#include "../Bot.h"
#include "../Debug.h"
#include "../Map.h"


/* Reading a bad memory address here can result in some very annoying problems that don't show up right away, so double check and mark any new additions.
these addresses were checked and fixed on 03/27/2022.

Last fix:  reading multifire status and item counts on other players was not the correct address and sometimes resulted in access violations.
*/

namespace marvin {

ContinuumGameProxy::ContinuumGameProxy() : module_base_menu_(0), module_base_continuum_(0) {

  UpdateMemory();
}

bool ContinuumGameProxy::IsLoaded() {
  if (!UpdateMemory() || !player_) {
    return false;
  }
  return true;
}

bool ContinuumGameProxy::UpdateMemory() {
  if (!module_base_menu_) {
    module_base_menu_ = process_.GetModuleBase("menu040.dll");

    if (!module_base_menu_) {
      return false;
    }
  }

  player_name_ = GetName();

  if (!module_base_continuum_) {
    module_base_continuum_ = process_.GetModuleBase("Continuum.exe");

    if (!module_base_continuum_) return false;
  }

  if (!game_addr_) {
    game_addr_ = process_.ReadU32(module_base_continuum_ + 0xC1AFC);
    position_data_ = (uint32_t*)(game_addr_ + 0x126BC);
    // Skip over existing messages on load
    // this needs to be set when the bot reloads
    u32 chat_base_addr = game_addr_ + 0x2DD08;
    chat_index_ = *(u32*)(chat_base_addr + 8);
  }

  return true;
}

UpdateState ContinuumGameProxy::Update() {
  PerformanceTimer timer;
  // Continuum stops processing input when it loses focus, so update the memory
  // to make it think it always has focus.
  SetWindowFocus();

  if (!UpdateMemory()) return UpdateState::Wait;  

  // this fixes memory exceptions when arena recycles
  if (!IsInGame()) {
    return UpdateState::Wait;
  }

  u8* map_memory = (u8*)*(u32*)(game_addr_ + 0x127ec + 0x1d6d0);
  std::string map_file_path = GetServerFolder() + "\\" + GetMapFile();

  // load new map
  if (mapfile_path_ != map_file_path) {
    if (!map_memory) {
      return UpdateState::Wait;
    } else {
      mapfile_path_ = map_file_path;
      map_ = Map::Load(mapfile_path_);
      return UpdateState::Reload;
    }
  }

  FetchZone();
  //g_RenderState.RenderDebugText("Game: Set Zone: %llu", timer.GetElapsedTime());
  FetchPlayers();
  //g_RenderState.RenderDebugText("Game: Fetch Players: %llu", timer.GetElapsedTime());
  FetchBallData();
 // g_RenderState.RenderDebugText("Game: Fetch Ball Data: %llu", timer.GetElapsedTime());
  FetchDroppedFlags();
  //g_RenderState.RenderDebugText("Game: Fetch Dropped Flags: %llu", timer.GetElapsedTime());
  FetchGreens();
  //g_RenderState.RenderDebugText("Game: Fetch Greens: %llu", timer.GetElapsedTime());
  FetchChat();
 // g_RenderState.RenderDebugText("Game: Fetch Chat: %llu", timer.GetElapsedTime());
  FetchWeapons();
  //g_RenderState.RenderDebugText("Game: Fetch Weapons: %llu", timer.GetElapsedTime());
  
  if (ProcessQueuedMessages()) {
    return UpdateState::Wait;
  }

  if (reset_ship_index_ != 0) { // run and finish before checking setfreq and setarena
    ResetShip();
    return UpdateState::Wait;
  } 

  if (set_freq_) SetFreq(desired_freq_);  // set freq uses resetship
  if (set_arena_) SetArena(desired_arena_);  // set arena uses resetship



  map_->SetMinedTiles(GetEnemyMines());

  return UpdateState::Clear;
}

void ContinuumGameProxy::FetchPlayers() {
  PerformanceTimer timer;
  const std::size_t kPosOffset = 0x04;
  const std::size_t kVelocityOffset = 0x10;
  const std::size_t kIdOffset = 0x18;
  const std::size_t kBountyOffset1 = 0x20;
  const std::size_t kBountyOffset2 = 0x24;
  const std::size_t kFlagOffset1 = 0x30;
  const std::size_t kFlagOffset2 = 0x34;
  const std::size_t kRotOffset = 0x3C;
  const std::size_t kExplodeTimerOffset1 = 0x40;  // count down timer for the exploding graphic when player dies
  const std::size_t kDeadOffset = 0x4C;           // a possible death flag
  const std::size_t kShipOffset = 0x5C;
  const std::size_t kFreqOffset = 0x58;
  const std::size_t kStatusOffset = 0x60;
  const std::size_t kNameOffset = 0x6D;
  const std::size_t kDeathCountOffset = 0xD0;                    // total player death count tracked by the server
  const std::size_t kUnknownTimestampOffset = 0x13C;             // don't know what this is
  const std::size_t kPlayerLastPositionTimestampOffset = 0x14C;  // 0 for the main player in most cases
  const std::size_t kPlayerPing = 0x164;                         // same number that's displayed by player names
  const std::size_t kPlayerActiveOffset = 0x178;  // triggers if the player is just idling in a ship at near 0 velocity, or dead
  const std::size_t kEnergyOffset1 = 0x208;
  const std::size_t kEnergyOffset2 = 0x20C;
  const std::size_t kMultiFireCapableOffset = 0x2EC;
  const std::size_t kMultiFireStatusOffset = 0x32C;

  if (!game_addr_) return;

  std::size_t base_addr = game_addr_ + 0x127EC;
  std::size_t players_addr = base_addr + 0x884;
  std::size_t count_addr = base_addr + 0x1884;

  if (players_addr == 0 || count_addr == 0) return;

  std::size_t count = process_.ReadU32(count_addr) & 0xFFFF;
  
  player_generation_id_++;

  // players are arranged in memory in the same order as the player list is in the game
  // pressing F2 while in the game will rearange this list
  for (std::size_t i = 0; i < count; ++i) {
    std::size_t player_addr = process_.ReadU32(players_addr + (i * 4));

    if (!player_addr) continue;

    Player player;

    player.generation_id = player_generation_id_;

    player.name = (char*)(player_addr + kNameOffset);
    //g_RenderState.RenderDebugText("Player Name: %s", player.name.c_str());

    player.position.x = process_.ReadU32(player_addr + kPosOffset) / 1000.0f / 16.0f;

    player.position.y = process_.ReadU32(player_addr + kPosOffset + 4) / 1000.0f / 16.0f;

    player.velocity.x = process_.ReadI32(player_addr + kVelocityOffset) / 10.0f / 16.0f;

    player.velocity.y = process_.ReadI32(player_addr + kVelocityOffset + 4) / 10.0f / 16.0f;

    player.id = static_cast<uint16_t>(process_.ReadU32(player_addr + kIdOffset));

    player.discrete_rotation = static_cast<uint16_t>(process_.ReadU32(player_addr + kRotOffset) / 1000);

    player.ship = static_cast<uint16_t>(process_.ReadU32(player_addr + kShipOffset));

    player.frequency = static_cast<uint16_t>(process_.ReadU32(player_addr + kFreqOffset));

    player.status = static_cast<uint8_t>(process_.ReadU32(player_addr + kStatusOffset));  // a bitset/bitfield of bools

    player.bounty = *(u32*)(player_addr + kBountyOffset1) + *(u32*)(player_addr + kBountyOffset2);

    player.flags = *(u32*)(player_addr + kFlagOffset1) + *(u32*)(player_addr + kFlagOffset2);

    // triggers if player is not moving and has no velocity, not used
    // player.dead = process_.ReadU32(player_addr + kDeadOffset) == 0;

    // might not work on bot but works well on other players
    // player.active = *(u32*)(player_addr + kActiveOffset1) == 0 && *(u32*)(player_addr + kActiveOffset2) == 0;
    player.dead = *(u32*)(player_addr + kExplodeTimerOffset1) != 0 || *(u32*)(player_addr + kDeadOffset) != 0;

    // these are not valid when reading on other players and will result in crashes
    if (player_name_ == player.name) {
      player_id_ = player.id;
      player_addr_ = player_addr;

      player.flight_status.rotation = *(u32*)(player_addr + 0x278) + *(u32*)(player_addr + 0x274);
      player.flight_status.recharge = *(u32*)(player_addr + 0x1E8) + *(u32*)(player_addr + 0x1EC);
      player.flight_status.shrapnel = *(u32*)(player_addr + 0x2A8) + *(u32*)(player_addr + 0x2AC);
      player.flight_status.thrust = *(u32*)(player_addr + 0x244) + *(u32*)(player_addr + 0x248);
      player.flight_status.speed = *(u32*)(player_addr + 0x350) + *(u32*)(player_addr + 0x354);
      player.flight_status.max_energy = *(u32*)(player_addr + 0x1C8) + *(u32*)(player_addr + 0x1C4);

      // the id of the player the bot is attached to, a value of -1 means its not attached, or 65535
      player.attach_id = process_.ReadU32(player_addr + 0x1C);

      u32 capabilities = *(u32*)(player_addr + 0x2EC);
      u32 cloak_stealth = *(u32*)(player_addr + 0x2E8);

      player.capability.stealth = (cloak_stealth & (1 << 17)) != 0;
      player.capability.cloak = (cloak_stealth & (1 << 30)) != 0;
      player.capability.antiwarp = (capabilities & (1 << 7)) != 0;
      player.capability.xradar = (capabilities & (1 << 9)) != 0;
      player.capability.multifire = (capabilities & (1 << 15)) != 0;
      player.capability.bouncing_bullets = (capabilities & (1 << 19)) != 0;
      player.capability.proximity = (capabilities & (1 << 26)) != 0;

      player.multifire_status = static_cast<uint8_t>(process_.ReadU32(player_addr + kMultiFireStatusOffset));
      // player.multifire_capable = (process_.ReadU32(player_addr + kMultiFireCapableOffset)) & 0x8000;

      // remaining emp ticks if hit with emp
      player.emp_ticks = *(u32*)(player_addr + 0x2f4) + *(u32*)(player_addr + 0x2f8);

      player.repels = *(u32*)(player_addr + 0x2B0) + *(u32*)(player_addr + 0x2B4);
      player.bursts = *(u32*)(player_addr + 0x2B8) + *(u32*)(player_addr + 0x2BC);
      player.decoys = *(u32*)(player_addr + 0x2D8) + *(u32*)(player_addr + 0x2DC);
      player.thors = *(u32*)(player_addr + 0x2D0) + *(u32*)(player_addr + 0x2D4);
      player.bricks = *(u32*)(player_addr + 0x2C0) + *(u32*)(player_addr + 0x2C4);
      player.rockets = *(u32*)(player_addr + 0x2C8) + *(u32*)(player_addr + 0x2CC);
      player.portals = *(u32*)(player_addr + 0x2E0) + *(u32*)(player_addr + 0x2E4);

      // level 0 = no guns, level 1 = red, etc.  Does not align with level found in weapon memory
      // because weapon memory starts with red as 0 and uses gun type None
      player.gun_level = *(u32*)(player_addr + 0x298) + *(u32*)(player_addr + 0x29C);
      player.bomb_level = *(u32*)(player_addr + 0x2A0) + *(u32*)(player_addr + 0x2A4);

      // Energy calculation @4485FA
      u32 energy1 = process_.ReadU32(player_addr + kEnergyOffset1);
      u32 energy2 = process_.ReadU32(player_addr + kEnergyOffset2);

      // energy1 and energy2 are larger than a uint32_t can store so it rolls over
      u32 combined = energy1 + energy2;
      // this then takes the result and basically divides it by 1000
      u64 energy = ((combined * (u64)0x10624DD3) >> 32) >> 6;

      player.energy = static_cast<uint16_t>(energy);

    } else {

      u32 first = *(u32*)(player_addr + 0x150);
      u32 second = *(u32*)(player_addr + 0x154);

      player.energy = static_cast<uint16_t>(first + second);   
   
      SetDefaultWeaponLevels(player);
    }
 
    // remove "^" that gets placed on names when biller is down
    if (!player.name.empty() && player.name[0] == '^') {
      player.name.erase(0, 1);
    }
 
    if (players_.empty()) {
      players_.emplace_back(player);
    } else {
      // update players list with new player
      for (std::size_t i = 0; i < players_.size(); i++) {
        if (players_[i].name == player.name) {
          if (players_[i].ship != player.ship) {
            // overwrite everything
            players_[i] = player;
          } else {
            // Update player
            players_[i].Update(player);
          }
          break;
        } else if (i == players_.size() - 1) {
          // add the new player to the list
          players_.emplace_back(player);
          break;
        }
      }
    }

    if (player.id == player_id_) {
#if DEBUG_RENDER_GAMEPROXY
      g_RenderState.RenderDebugText("Player Name: %s", player.name.c_str());
      g_RenderState.RenderDebugText("XRadar Capable: %u", player.capability.xradar);
      g_RenderState.RenderDebugInBinary("Status: ", player.status);  // print in binary
      g_RenderState.RenderDebugText("Player Energy: %u", player.energy);
      g_RenderState.RenderDebugText("Player Max Energy: %u", player.flight_status.max_energy);
#endif
    } 
  }

  //g_RenderState.RenderDebugText("End: %llu", timer.TimeSinceConstruction());
  // remove players that left the arena
  for (std::size_t i = 0; i < players_.size(); i++) {
    if (players_[i].generation_id != player_generation_id_) {
      // remove the existing player
      players_.erase(players_.begin() + i);
      i--;
    }
  }

  // if the players list is changed after this is set the player_ pointer will be invalid
  for (std::size_t i = 0; i < players_.size(); i++) {
    if (players_[i].id == player_id_) {
      player_ = &players_[i];
      break;
    }
  }
}

void ContinuumGameProxy::SetDefaultWeaponLevels(Player& player) {
  Zone zone = GetZone();
  uint16_t ship = player.ship;

  // bot can find it's own in memory 
  if (player.name == player_name_) return;

  uint32_t bomb_level = 0;
  uint32_t gun_level = 0;

    switch (zone) {
      case Zone::Devastation:
        bomb_level = GetShipSettings(ship).MaxBombs;
        gun_level = GetShipSettings(ship).MaxGuns;
        break;
      default:
        bomb_level = GetShipSettings(ship).InitialBombs;
        gun_level = GetShipSettings(ship).InitialGuns;
    }

    player.gun_level = gun_level;
    player.bomb_level = bomb_level;
}

void ContinuumGameProxy::FetchBallData() {
  balls_.clear();

  for (size_t i = 0;; ++i) {
    u32 ball_ptr = *(u32*)(game_addr_ + 0x2F3C8 + i * 4);

    if (ball_ptr == 0) break;

    BallData balldata;

    balldata.inactive_timer = *(u32*)(ball_ptr + 0x38);
    balldata.held = *(u32*)(ball_ptr + 0x34) == 0;

    balldata.last_holder_id = *(u16*)(ball_ptr + 0x32);

    balldata.position.x = *(u32*)(ball_ptr + 0x04) / 1000.0f / 16.0f;
    balldata.position.y = *(u32*)(ball_ptr + 0x08) / 1000.0f / 16.0f;

    balldata.velocity.x = *(i32*)(ball_ptr + 0x10) / 10.0f / 16.0f;
    balldata.velocity.y = *(i32*)(ball_ptr + 0x14) / 10.0f / 16.0f;

    balldata.last_activity.x = *(u16*)(ball_ptr + 0x2A) / 1000.0f / 16.0f;
    balldata.last_activity.y = *(u16*)(ball_ptr + 0x2C) / 1000.0f / 16.0f;

    // if (data.position.x == 0 && data.position.y == 0) continue;
    balls_.emplace_back(balldata);
  }
}

void ContinuumGameProxy::FetchGreens() {
  greens_.clear();

  u32 green_count = *(u32*)(game_addr_ + 0x2e350);
  Green* greens = (Green*)(game_addr_ + 0x2df50);

  for (size_t i = 0; i < green_count; ++i) {
    greens_.push_back(greens[i]);
  }
}

// Retrieves every chat message since the last call
void ContinuumGameProxy::FetchChat() {

  if (!game_addr_) return;

  current_chat_.clear();  // flush out old chat 

  u32 chat_base_addr = game_addr_ + 0x2DD08;

  ChatEntry* chat_ptr = *(ChatEntry**)(chat_base_addr);
  u32 entry_count = *(u32*)(chat_base_addr + 8);
  int read_count = entry_count - chat_index_;

  if (read_count < 0) {
    read_count += 64;
  }

  for (int i = 0; i < read_count; ++i) {
    if (chat_index_ >= 1024) {
      chat_index_ -= 64;
    }

    ChatEntry* entry = chat_ptr + chat_index_;
    ++chat_index_;

    if (!entry) continue;
    ChatMessage chat;

    chat.message = entry->message;
    chat.player = entry->player;
    chat.type = (ChatType)entry->type;

    if (chat.message.empty()) continue;

    current_chat_.push_back(chat);
    chat_.push_back(chat);  
  }
}

void ContinuumGameProxy::FetchWeapons() {
  // Grab the address to the main player structure
  u32 player_addr = *(u32*)(game_addr_ + 0x13070);

  // Follow a pointer that leads to weapon data
  u32 ptr = *(u32*)(player_addr + 0x0C);
  u32 weapon_count = *(u32*)(ptr + 0x1DD0) + *(u32*)(ptr + 0x1DD4);
  u32 weapon_ptrs = (ptr + 0x21F4);

  weapons_.clear();
  enemy_mines_.clear();

  for (size_t i = 0; i < weapon_count; ++i) {
    u32 weapon_data = *(u32*)(weapon_ptrs + i * 4);

    WeaponMemory* data = (WeaponMemory*)(weapon_data);
    WeaponType type = data->data.type;
    float alive_milliseconds = data->alive_ticks / 100.0f;
    
    Player* player = nullptr;

    for (std::size_t i = 0; i < players_.size(); ++i) {
      if (players_[i].id == data->pid) {
        player = &players_[i];
        break;
      }
    }
    
    float total_milliseconds = 0.0f;
    bool is_mine = false;

    switch (type) {
      case WeaponType::Bomb:
      case WeaponType::ProximityBomb:
      case WeaponType::Thor: {
        total_milliseconds = this->GetSettings().GetBombAliveTime();
        if (player) player->bomb_level = data->data.level + 1;
        if (data->data.alternate) {
          total_milliseconds = this->GetSettings().GetMineAliveTime();
          is_mine = true;        
        }
      } break;
      case WeaponType::Burst:
      case WeaponType::Bullet:
      case WeaponType::BouncingBullet: {
        total_milliseconds = this->GetSettings().GetBulletAliveTime();
        if (player) player->gun_level = data->data.level + 1;
      } break;
      case WeaponType::Repel: {
        total_milliseconds = this->GetSettings().GetRepelTime();
      } break;
      case WeaponType::Decoy: {
        total_milliseconds = this->GetSettings().GetDecoyAliveTime();
      } break;
      default: {
        total_milliseconds = this->GetSettings().GetBulletAliveTime();
      } break;
    }
    
    if (alive_milliseconds > total_milliseconds) alive_milliseconds = total_milliseconds;

    weapons_.emplace_back(data, total_milliseconds - alive_milliseconds);

    if (is_mine && player && player->frequency != GetPlayer().frequency) {
      enemy_mines_.emplace_back(data, total_milliseconds - alive_milliseconds);
    }
  }
}

void ContinuumGameProxy::FetchDroppedFlags() {
  u32 flag_count = *(u32*)(game_addr_ + 0x127ec + 0x1d4c);
  u32** flag_ptrs = (u32**)(game_addr_ + 0x127ec + 0x188c);
  // std::vector<marvin::Flag> flags;
  dropped_flags_.clear();
  dropped_flags_.reserve(flag_count);
  for (size_t i = 0; i < flag_count; ++i) {
    char* current = (char*)flag_ptrs[i];
    u32 flag_id = *(u32*)(current + 0x1C);
    u32 x = *(u32*)(current + 0x04);
    u32 y = *(u32*)(current + 0x08);
    u32 frequency = *(u32*)(current + 0x14);
    dropped_flags_.emplace_back(flag_id, frequency, Vector2f(x / 16000.0f, y / 16000.0f));
  }
}

ConnectState ContinuumGameProxy::GetConnectState() {
  if (!UpdateMemory()) return ConnectState::None;

  if (IsOnMenu()) return ConnectState::None;

  ConnectState state = *(ConnectState*)(game_addr_ + 0x127EC + 0x588);

  if (state == ConnectState::Playing) {
    // Check if the client has timed out.
    u32 ticks_since_recv = *(u32*)(game_addr_ + 0x127EC + 0x590);

    // @453420
    if (ticks_since_recv > 1500) {
      return ConnectState::Disconnected;
    }
  }

  return state;
}

void ContinuumGameProxy::ExitGame() {

  if (!game_addr_ && !UpdateMemory()) return;

  u8* leave_ptr = (u8*)(game_addr_ + 0x127ec + 0x58c);
  *leave_ptr = 1;
}

bool ContinuumGameProxy::GameIsClosing() {
  if (!game_addr_ && !UpdateMemory()) return true;

  u8* leave_ptr = (u8*)(game_addr_ + 0x127ec + 0x58c);
  return *leave_ptr == 1;
}

bool ContinuumGameProxy::IsOnMenu() {
  UpdateMemory();
  if (!module_base_menu_) return true;

  return *(u8*)(module_base_menu_ + 0x47a84) == 0;
}

bool ContinuumGameProxy::IsInGame() {

  if (!UpdateMemory()) return false;

  u8* map_memory = (u8*)*(u32*)(*(u32*)(0x4C1AFC) + 0x127ec + 0x1d6d0);
  if (map_memory && GetConnectState() == ConnectState::Playing) {
    return true;
  }

  return false;
}

const std::vector<Flag>& ContinuumGameProxy::GetDroppedFlags() {
  return dropped_flags_;
}

void ContinuumGameProxy::FetchZone() {
  zone_ = Zone::Other;
  zone_name_ = GetServerFolder().substr(6);

  if (zone_name_ == "SSCJ Devastation") {
    zone_ = Zone::Devastation;
  } else if (zone_name_ == "SSCU Extreme Games") {
    zone_ = Zone::ExtremeGames;  // EG does not allow any memory writing and will kick the bot
  } else if (zone_name_ == "SSCJ Galaxy Sports") {
    zone_ = Zone::GalaxySports;
  } else if (zone_name_ == "SSCE HockeyFootball Zone") {
    zone_ = Zone::Hockey;
  } else if (zone_name_ == "SSCE Hyperspace") {
    zone_ = Zone::Hyperspace;
  } else if (zone_name_ == "SSCJ PowerBall") {
    zone_ = Zone::PowerBall;
  }
}

// const ShipFlightStatus& ContinuumGameProxy::GetShipStatus() const {
//   return ship_status_;
// }

const Zone ContinuumGameProxy::GetZone() { return zone_; }
const std::string ContinuumGameProxy::GetZoneName() { return zone_name_; }


std::vector<ChatMessage> ContinuumGameProxy::GetChatHistory() { return chat_; }
std::vector<ChatMessage> ContinuumGameProxy::GetCurrentChat() { return current_chat_; }

ChatMessage ContinuumGameProxy::FindChatMessage(std::string match) {
  
  ChatMessage result;
    
  for (ChatMessage msg : current_chat_) {
    if (msg.message == match) {
      result = msg;
      break;
    }
  }

  return result;
}

const std::vector<BallData>& ContinuumGameProxy::GetBalls() const {
  return balls_;
}

std::vector<Weapon*> ContinuumGameProxy::GetEnemyMines() {
  std::vector<Weapon*> mines;

  for (std::size_t i = 0; i < enemy_mines_.size(); ++i) {
    mines.push_back(&enemy_mines_[i]);
  }

  return mines;
}

const std::vector<Green>& ContinuumGameProxy::GetGreens() const {
  return greens_;
}

std::vector<Weapon*> ContinuumGameProxy::GetWeapons() {
  std::vector<Weapon*> weapons;

  for (std::size_t i = 0; i < weapons_.size(); ++i) {
    weapons.push_back(&weapons_[i]);
  }

  return weapons;
}

const ClientSettings& ContinuumGameProxy::GetSettings() const {

  //if (!game_addr_) return ClientSettings();

  std::size_t addr = game_addr_ + 0x127EC + 0x1AE70;  // 0x2D65C

  return *reinterpret_cast<ClientSettings*>(addr);
}

// there are no ship settings for spectators
const ShipSettings& ContinuumGameProxy::GetShipSettings() const {
  
  //if (!player_) return ShipSettings();
    
  int ship = player_->ship;

  if (ship < 0 || ship > 7) {
    ship = 0;
  }
  return GetSettings().ShipSettings[ship];
}

// there are no ship settings for spectators
const ShipSettings& ContinuumGameProxy::GetShipSettings(int ship) const {
  if (ship < 0 || ship > 7) {
    ship = 0;
  }

  return GetSettings().ShipSettings[ship];
}

// the selected profile index captured here is valid until the game gets closed back into the menu
// then it will change to the last index that the menu was on when the game was launched
// so when running more than one bot it will change to the name of the last bot launched
// it might be saving the selected profile to a file and reloading it?
std::string ContinuumGameProxy::GetName() const {

  if (!module_base_menu_) return "";

  const std::size_t ProfileStructSize = 2860;

  uint16_t profile_index = process_.ReadU32(module_base_menu_ + 0x47FA0) & 0xFFFF;
  std::size_t addr = process_.ReadU32(module_base_menu_ + 0x47A38) + 0x15;

  if (!addr) return "";
  
  addr += profile_index * ProfileStructSize;

  std::string name = process_.ReadString(addr, 23);

  // remove "^" that gets placed on names when biller is down
  if (!name.empty() && name[0] == '^') {
    name.erase(0, 1);
  }

  name = name.substr(0, strlen(name.c_str()));

  return name;
}

bool ContinuumGameProxy::SetMenuProfileIndex() {
  UpdateMemory();
  if (!module_base_menu_) return false;
  if (!IsOnMenu()) return false;
  if (player_name_.empty()) return false;

  const std::size_t ProfileStructSize = 2860;
  std::size_t addr = process_.ReadU32(module_base_menu_ + 0x47A38) + 0x15;
  u32 count = process_.ReadU32(module_base_menu_ + 0x47A30);  // size of profile list

  std::string name;

  if (addr == 0) {
    return false;
  }

  for (uint16_t i = 0; i < count; i++) {
    
    name = process_.ReadString(addr, 23);

    if (name == player_name_) {
      *(u32*)(module_base_menu_ + 0x47FA0) = i;
      return true;
    }

    addr += ProfileStructSize;
  }

  return false;
}

HWND ContinuumGameProxy::GetGameWindowHandle() {
  HWND handle = 0;

  if (game_addr_) {
    handle = *(HWND*)(game_addr_ + 0x8C);
  }

  return handle;
}

int ContinuumGameProxy::GetEnergy() const {
  return player_->energy;
}

Vector2f ContinuumGameProxy::GetPosition() const {
  float x = (*position_data_) / 16.0f;
  float y = (*(position_data_ + 1)) / 16.0f;

  return Vector2f(x, y);
}

const std::vector<Player>& ContinuumGameProxy::GetPlayers() const {
  return players_;
}

void ContinuumGameProxy::SetTileId(Vector2f position, u8 id) {
  map_->SetTileId(position, id);
}

const Map& ContinuumGameProxy::GetMap() const {
  return *map_;
}
const Player& ContinuumGameProxy::GetPlayer() const {
  return *player_;
}

const Player* ContinuumGameProxy::GetPlayerById(u16 id) const {
  for (std::size_t i = 0; i < players_.size(); ++i) {
    if (players_[i].id == id) {
      return &players_[i];
    }
  }
  return nullptr;
}

const Player* ContinuumGameProxy::GetPlayerByName(std::string_view name) const {
  for (std::size_t i = 0; i < players_.size(); ++i) {
    if (players_[i].name == name) {
      return &players_[i];
    }
  }
  return nullptr;
}

const float ContinuumGameProxy::GetEnergyPercent() {
  return ((float)GetEnergy() / player_->flight_status.max_energy) * 100.0f;
}

const float ContinuumGameProxy::GetMaxEnergy() {
  return (float)player_->flight_status.max_energy;
}

const float ContinuumGameProxy::GetThrust() {
  return (float)player_->flight_status.thrust * 10.0f / 16.0f;
}

const float ContinuumGameProxy::GetMaxSpeed() {
  float speed = ((float)player_->flight_status.speed) / 10.0f / 16.0f;

  if (player_->velocity.Length() > speed) {
    speed = std::abs(speed + GetShipSettings().GravityTopSpeed);
  }
  return speed;
}

const float ContinuumGameProxy::GetMaxSpeed(u16 ship) {
  float speed = 0.0f;
  switch (zone_) {
    case Zone::Devastation: {
      speed = GetShipSettings(ship).GetMaximumSpeed();
      break;
    }
    default:
      speed = GetShipSettings(ship).GetInitialSpeed();
  }
  return speed;
  //return player_->flight_status.speed;
}

const float ContinuumGameProxy::GetRotation() {
  // float rotation = GetShipSettings().GetInitialRotation();

  // if (zone_ == Zone::Devastation) {
  //   rotation = GetShipSettings().GetMaximumRotation();
  // }
  //
  // changed from  / 200.f as / 400.f converts to rotations/second
  float rotation = ((float)player_->flight_status.rotation) / 400.0f;

  return rotation;
}

const float ContinuumGameProxy::GetRadius() {
  if (!game_addr_) return 0.0f;

  return GetShipSettings().GetRadius();
}

const uint64_t ContinuumGameProxy::GetRespawnTime() {
  return (uint64_t)GetSettings().EnterDelay * 10;
}

// TODO: Find level data or level name in memory
std::string ContinuumGameProxy::GetServerFolder() {
  std::size_t folder_addr = *(uint32_t*)(game_addr_ + 0x127ec + 0x5a3c) + 0x10D;
  std::string server_folder = process_.ReadString(folder_addr, 256);

  return server_folder;
}

const std::string ContinuumGameProxy::GetMapFile() const {
  std::string file = process_.ReadString((*(u32*)(game_addr_ + 0x127ec + 0x6C4)) + 0x01, 16);

  return file;
}

void ContinuumGameProxy::SetVelocity(Vector2f desired) {

  int32_t vel_x = int32_t(desired.x * 10.0f * 16.0f);
  int32_t vel_y = int32_t(desired.y * 10.0f * 16.0f);

  process_.WriteI32(player_addr_ + 0x10, vel_x);
  process_.WriteI32(player_addr_ + (0x10 + 4), vel_y);
}

void ContinuumGameProxy::SetPosition(Vector2f desired) {
  uint32_t vel_x = uint32_t(desired.x * 10.0f * 16.0f);
  uint32_t vel_y = uint32_t(desired.y * 10.0f * 16.0f);

  process_.WriteU32(player_addr_ + 0x04, vel_x);
  process_.WriteU32(player_addr_ + (0x04 + 4), vel_y);
}

void ContinuumGameProxy::SetSpeed(float desired) {

    uint32_t speed = (uint32_t)desired;

    speed *= 10;
    speed *= 16;

    process_.WriteU32(player_addr_ + 0x350, speed / 2);
    process_.WriteU32(player_addr_ + 0x354, speed / 2);

  //player.flight_status.speed = *(u32*)(player_addr + 0x350) + *(u32*)(player_addr + 0x354);
}

void ContinuumGameProxy::SetThrust(uint32_t desired) {

    const uint64_t overflow = 4294967296;

    desired *= 16;
    desired /= 10;

    u64 large_value = desired + overflow;

    process_.WriteU32(player_addr_ + 0x244, (u32)(large_value / 2));
    process_.WriteU32(player_addr_ + 0x248, (u32)(large_value / 2));

    //player.flight_status.thrust = *(u32*)(player_addr + 0x244) + *(u32*)(player_addr + 0x248);
}

void ContinuumGameProxy::SetEnergy(uint64_t percent, std::string reason) {
#if !DEBUG_USER_CONTROL
  const uint64_t overflow = 4294967296;

  if (percent > 100) {
    percent = 100;
  }

  //SendChatMessage("Energy was set to " + std::to_string(percent) + " percent for reason: " + reason + ".");

  u64 max_energy = (u64)GetMaxEnergy();

  u64 desired = u64(max_energy * (percent / 100));

  u64 mem_energy = (desired * 1000) + overflow;

  process_.WriteU32(player_addr_ + 0x208, (u32)(mem_energy / 2));
  process_.WriteU32(player_addr_ + 0x20C, (u32)(mem_energy / 2));
#endif
}

#if 0
void ContinuumGameProxy::SetFreq(int freq, bool retry) {  // if player is dead it has to wait for respawn
#if !DEBUG_USER_CONTROL


  if (retry) {
    desired_freq_ = freq;
    set_freq_ = true;
  }

 if (time_.GetTime() < setfreq_cooldown_) return;
 if (freq == player_->frequency) return;
 setfreq_cooldown_ = time_.GetTime() + 250;
 
  ResetStatus();
  SetEnergy(100, "warping");
  *(u32*)(player_addr_ + 0x40) = 0; // if player just died this can respawn the ship

  if (retry) {
    setfreq_trys_++;

    if (setfreq_trys_ > 10) {
      desired_freq_ = player_->frequency;  // stop trying
      setfreq_trys_ = 0;
      return;
    }
  }
  
  SendQueuedMessage("=" + std::to_string(freq));

  //*(u32*)(player_addr_ + 0x58) = freq;  // does some funny things

#endif
}
#endif

void ContinuumGameProxy::SetFreq(int freq) {
#if !DEBUG_USER_CONTROL

  if (player_->dead) {
    desired_freq_ = freq;
    set_freq_ = true;
    if (reset_ship_index_ == 0) reset_ship_index_ = 1;  // use resetship to respawn quickly
  } else {
    set_freq_ = false;
    ResetStatus();
    SetEnergy(100, "ship change");
    SendQueuedMessage("=" + std::to_string(freq));
  }
#endif
}
// hack to respawn ship and bypass death timer
// used when the bot needs to change frequency or when using swarm
bool ContinuumGameProxy::ResetShip() {  
#if !DEBUG_USER_CONTROL

   uint16_t ship = 0;

  switch (reset_ship_index_) {
    case 0:
      reset_ship_index_++;
      break;
    case 1:
      ship = player_->ship + 1;
      if (ship >= 8) ship -= 2;
      prev_ship_ = player_->ship;
      SetShip(ship);
      reset_ship_index_++;
      break;
    case 2:
      if (prev_ship_ != player_->ship) {
        reset_ship_index_++;
      } else {
        ship = player_->ship + 1;
        if (ship >= 8) ship -= 2;
        SetShip(ship);
      }
      break;
    case 3:
      SetShip(prev_ship_);
      reset_ship_index_++;
      break;
    case 4:
      if (prev_ship_ == player_->ship) {
        reset_ship_index_ = 0;
        return true;
      } else {
        SetShip(prev_ship_);
      }
      break;
    default:
      reset_ship_index_ = 0;
      break;
  }
#endif

  return false;
}

void ContinuumGameProxy::SetShip(uint16_t ship) {  // can set ship even when dead
    // make sure ship is in bounds
    if (ship > 8) ship = 0;

    if (!game_addr_ && !UpdateMemory()) return;

    if (time_.GetTime() < setship_cooldown_) return;
    if (ship == player_->ship) return;
    setship_cooldown_ = time_.GetTime() + 250;

    int* menu_open_addr = (int*)(game_addr_ + 0x12F39);
    //bool menu_open = (*menu_open_addr & 1); // can test if menu is open
    *menu_open_addr = (*menu_open_addr ^ 1);  // just set the menu to open

    //*(u32*)(player_addr_ + 0x5C) = ship; // this is pretty funny and broken

    ResetStatus();
    SetEnergy(100, "ship change");
    if (ship == 8) {
     PostMessage(GetGameWindowHandle(), WM_CHAR, (WPARAM)('s'), 0);
    } else {
     PostMessage(GetGameWindowHandle(), WM_CHAR, (WPARAM)('1' + ship), 0);
    }
}

// prefered method for hyperspace
void ContinuumGameProxy::Attach(std::string name) {

  if (time_.GetTime() < attach_cooldown_) return;
 
  ResetStatus();
  SetEnergy(100, "attach to player");

  if (zone_ == Zone::Hyperspace) {
    SendQueuedMessage(":" + name + ":?attach");
    attach_cooldown_ = time_.GetTime() + 1000;
  } else {
    const Player* player = GetPlayerByName(name);
    SetSelectedPlayer(player->id);
    SendKey(VK_F7);
    attach_cooldown_ = time_.GetTime() + 100;
  }
}

// prefered method when attach uses F7 key
void ContinuumGameProxy::Attach(uint16_t id) {

  if (time_.GetTime() < attach_cooldown_) return;

  ResetStatus();
  SetEnergy(100, "attach to player");

  if (zone_ == Zone::Hyperspace) {
    const Player* player = GetPlayerById(id);
    SendQueuedMessage(":" + player->name + ":?attach");
    attach_cooldown_ = time_.GetTime() + 1000;
  } else {
    SetSelectedPlayer(id);
    SendKey(VK_F7);
    attach_cooldown_ = time_.GetTime() + 100;
  }
}

void ContinuumGameProxy::Attach() {

  if (time_.GetTime() < attach_cooldown_) return;

    ResetStatus();
  SetEnergy(100, "attach to player");

  SendKey(VK_F7);

  attach_cooldown_ = time_.GetTime() + 100;
}

void ContinuumGameProxy::HSFlag() {

  if (time_.GetTime() < flag_cooldown_) return;

    ResetStatus();
  SetEnergy(100, "hs flag");

  SendQueuedMessage("?flag");

  flag_cooldown_ = time_.GetTime() + 1000;
}

void ContinuumGameProxy::ResetStatus() {
  uint8_t status = player_->status;

  status &= ~(Status_Stealth | Status_Cloak | Status_XRadar | Status_Antiwarp);

  *(u8*)(player_addr_ + 0x60) = status;
}

void ContinuumGameProxy::SetStatus(StatusFlag status, bool on_off) {
  uint8_t current_status = player_->status;

  if (on_off) {
    current_status ^= status;
  } else {
    current_status &= ~(status);
  }

  *(u8*)(player_addr_ + 0x60) = current_status;
}

void ContinuumGameProxy::XRadar() {
  SendKey(VK_END);
}

void ContinuumGameProxy::Antiwarp(KeyController& keys) {
  keys.Press(VK_SHIFT);
  SendKey(VK_END);
}

void ContinuumGameProxy::Burst(KeyController& keys) {
  // must be sent using this combination
  keys.Press(VK_SHIFT);
  SendKey(VK_DELETE);
}
void ContinuumGameProxy::Repel(KeyController& keys) {
  keys.Press(VK_SHIFT);
  keys.Press(VK_CONTROL);
}

void ContinuumGameProxy::SetArena(const std::string& arena) {
#if !DEBUG_USER_CONTROL

  if (player_->dead) {
    desired_arena_ = arena;
    set_arena_ = true;
    if (reset_ship_index_ == 0) reset_ship_index_ = 1;  // use resetship to respawn quickly
  } else {
    set_arena_ = false;
    ResetStatus();
    SetEnergy(100, "arena change");
    SendQueuedMessage("?go " + arena);
  }
#endif
}



void ContinuumGameProxy::Warp() {
  ResetShip();
  //SendKey(VK_INSERT);
}

void ContinuumGameProxy::Stealth() {
  SendKey(VK_HOME);
}

void ContinuumGameProxy::Cloak(KeyController& keys) {
  keys.Press(VK_SHIFT);
  SendKey(VK_HOME);
}

void ContinuumGameProxy::MultiFire() {
  SendKey(VK_DELETE);
}

void ContinuumGameProxy::SetWindowFocus() {
  std::size_t focus_addr = game_addr_ + 0x3039c;

  process_.WriteU32(focus_addr, 1);
}

ExeProcess& ContinuumGameProxy::GetProcess() {
  return process_;
}

void ContinuumGameProxy::SendKey(int vKey) {
#if !DEBUG_USER_CONTROL
  PostMessage(GetGameWindowHandle(), WM_KEYDOWN, (WPARAM)vKey, 0);
  PostMessage(GetGameWindowHandle(), WM_KEYUP, (WPARAM)vKey, 0);
#endif
}

bool ContinuumGameProxy::LimitMessageRate() {
  const int kFloodLimit = 3;
  const u64 kFloodCooldown = 1200;

  if (IsKnownBot()) return false;  // bypass rate limiter if the zone allows bot to spam

  if (time_.GetTime() > message_cooldown_) {
    sent_message_count_ = 0;
    message_cooldown_ = time_.GetTime() + kFloodCooldown;
  }

  if (sent_message_count_ >= kFloodLimit) return true;

  return false;
}

bool ContinuumGameProxy::IsKnownBot() {

  if (zone_ != Zone::Devastation) return false;  // currently deva is the only zone whitelisting bots

  for (const std::string& name : kBotNames) {
    if (name == player_->name) return true;
  }
  return false;
}

// priority messages will make the bot stop until it sends the messages
// currently used for buying items in hyperspace
bool ContinuumGameProxy::ProcessQueuedMessages() {
  bool result = false;

  if (!priority_message_queue_.empty()) {
    result = true;
  }

  if ((message_queue_.empty() && priority_message_queue_.empty())) return result;

  if (LimitMessageRate()) return false;
      
  // ignore if there is a duplicate message in the queue
  for (std::size_t i = 0; i < message_queue_.size(); i++) {
    if (i != 0 && message_queue_[0] == message_queue_[i]) {
     // message_queue_.pop_front();
    //  i--;
    //  return result;
    }
  }
  
  if (!priority_message_queue_.empty()) {
    SendQueuedMessage(priority_message_queue_[0]);
    priority_message_queue_.pop_front();
  } else if (!message_queue_.empty()) {
    SendQueuedMessage(message_queue_[0]);
    message_queue_.pop_front();
  }
   
  if (!IsKnownBot()) sent_message_count_++;

  return result;
}

// this might be needed for messages that need to be sent while the bot is on a safe tile
// such as buying items in hyperspace
void ContinuumGameProxy::SendPriorityMessage(const std::string& message) {
  priority_message_queue_.emplace_back(message);
  //SendMessage(message);
}

void ContinuumGameProxy::SendQueuedMessage(const std::string& mesg) {
#if !DEBUG_USER_CONTROL

  if (mesg.empty()) return;
  
  // arbitrary number, continuum allows 249 characters in messages
  // but this method seems to crash with strings lengths of 244 or greater
  // doesnt always crash
  const std::size_t kMaxLength = 240;
  std::size_t length = mesg.length();
  
  if (length > kMaxLength) length = kMaxLength;

 #if 0  // backup method
  for (std::size_t i = 0; i < mesg.size(); i++) {
    PostMessage(g_hWnd, WM_CHAR, mesg[i], 0);
  }

  PostMessage(hwnd_, WM_KEYDOWN, VK_RETURN, 0);
  PostMessage(hwnd_, WM_KEYUP, VK_RETURN, 0);
  #endif

  #if 1
  typedef void(__fastcall * ChatSendFunction)(void* This, void* thiscall_garbage, char* msg, u32* unknown1,
                                              u32* unknown2);

  // The address to the current text input buffer
  std::size_t chat_input_addr = game_addr_ + 0x2DD14;
  char* input = (char*)(chat_input_addr);
  memcpy(input, mesg.c_str(), length);  // should be safe even if the string is longer than the length
  input[length] = 0;

  ChatSendFunction send_func = (ChatSendFunction)(*(u32*)(module_base_continuum_ + 0xAC30C));

  void* This = (void*)(game_addr_ + 0x2DBF0);

  // Some value that the client passes in for some reason
  u32 value = 0x4AC370;

  send_func(This, nullptr, input, &value, 0);

  // Clear the text buffer after sending the message
  input[0] = 0;
  #endif

#endif
}

void ContinuumGameProxy::SendChatMessage(const std::string& mesg) {
  message_queue_.emplace_back(mesg);
}

void ContinuumGameProxy::SendPrivateMessage(const std::string& target, const std::string& mesg) {
  if (!target.empty()) {
   SendChatMessage(":" + target + ":" + mesg);
  }
}

const Player& ContinuumGameProxy::GetSelectedPlayer() const {
  u32 selected_index = *(u32*)(game_addr_ + 0x127EC + 0x1B758);
  std::size_t base_addr = game_addr_ + 0x127EC;
  std::size_t players_addr = base_addr + 0x884;
  std::size_t player_addr = process_.ReadU32(players_addr + (selected_index * 4));
  uint16_t player_id = static_cast<uint16_t>(process_.ReadU32(player_addr + 0x18));

  for (std::size_t i = 0; i < players_.size(); i++) {
   if (players_[i].id == player_id) {
      return players_[i];
   }
  }

  return *player_;
}

void ContinuumGameProxy::SetSelectedPlayer(uint16_t id) {
#if !DEBUG_USER_CONTROL

  std::size_t base_addr = game_addr_ + 0x127EC;
  std::size_t players_addr = base_addr + 0x884;
  std::size_t count_addr = base_addr + 0x1884;
  std::size_t count = process_.ReadU32(count_addr) & 0xFFFF;

  for (std::size_t i = 0; i < count; ++i) {
   std::size_t player_addr = process_.ReadU32(players_addr + (i * 4));
   uint16_t player_id = static_cast<uint16_t>(process_.ReadU32(player_addr + 0x18));

  //for (std::size_t i = 0; i < players_.size(); ++i) {
  //  const Player& player = players_[i];

    if (player_id == id) {
      *(u32*)(game_addr_ + 0x2DED0) = i;
      return;
    }
  }
#endif
}



// find the id index
std::size_t ContinuumGameProxy::GetIDIndex() {
  std::size_t index = 0;

  if (!player_) return index;

  for (const Player& player : players_) {
    // don't count the index of players who are not playing
    if (player.ship == 8 || (player.name.size() > 0 && player.name[0] == '<')) continue;

    if (player.id < player_->id) {
      index++;
    }
  }

  return index;
}

bool ContinuumGameProxy::WriteToPlayerProfile(const ProfileData& data) {
  if (!module_base_menu_) return false;
  if (!IsOnMenu()) return false;

  const std::size_t kProfileNameOffset = 0x00;
  const std::size_t kPlayerNameOffset = 0x15;
  const std::size_t kPasswordOffset = 0x2D;
  const std::size_t kSelectedShipOffset = 0x50;
  const std::size_t kResolutionWidthOffset = 0x54;
  const std::size_t kResolutionHeightOffset = 0x58;
  const std::size_t kRefreshRateOffset = 0x5C;
  const std::size_t kWindowModeOffset = 0x60;
  const std::size_t kZoneNameOffset = 0x64;
  const std::size_t kMacroNameOffset = 0xA5;
  const std::size_t kKeyDefNameOffset = 0xBA;
  const std::size_t kChatsOffset = 0xCF;
  const std::size_t kTargetOffset = 0x150;
  const std::size_t kAutoOffset = 0x154;

  #if 0 // memory alignment, does not appear to be a struct because of 32 bit aligment issues
    char profile_name[21];
    char player_name[24];  // 23 + null
    char password[33];
    char unknown[2];
    uint32_t selected_ship;      // default sets this to max value
    uint32_t resolution_width;   // default sets to 0
    uint32_t resolution_height;  // default sets to 0
    uint32_t refresh_rate;       // default sets to 0
    uint32_t window_mode;        // default sets to max value
    char zone_name[65];          // size unsure, maybe less.  default keeps the last zone selected in the drop down box?
    char macro_name[21];         // size unsure
    char keydef_name[21];        // size unsure
    char chats[129];
    uint32_t target;  // empty box sets to max value
    char autobox[250];
    char filler[2270];  // junk at the end seems to all be 0
  #endif

  std::size_t profile_addr = process_.ReadU32(module_base_menu_ + 0x47A38);
  if (profile_addr == 0) {
    return false;
  }

  //process_.WriteString(profile_addr + kProfileNameOffset, "Default", 21);
  process_.WriteString(profile_addr + kPlayerNameOffset, data.name, 24);
  process_.WriteString(profile_addr + kPasswordOffset, data.password, 33);
  //process_.WriteU32(profile_addr + kSelectedShipOffset, data.ship);
  //process_.WriteU32(profile_addr + kResolutionHeightOffset, 720);
  //process_.WriteU32(profile_addr + kResolutionWidthOffset, 1280);
  //process_.WriteU32(profile_addr + kRefreshRateOffset, 60);
  //process_.WriteU32(profile_addr + kWindowModeOffset, 0);
  //process_.WriteString(profile_addr + kZoneNameOffset, data.zone_name, 65);
  //process_.WriteString(profile_addr + kMacroNameOffset, "Default", 21);
  //process_.WriteString(profile_addr + kKeyDefNameOffset, "Default", 21);
  process_.WriteString(profile_addr + kChatsOffset, data.chats, 129);
  //process_.WriteU32(profile_addr + kTargetOffset, 0xFFFFFFFF);
  //process_.WriteString(profile_addr + kAutoOffset, "", 250);

  return true;
}

void ContinuumGameProxy::DumpMemoryToFile(std::size_t address, std::size_t size) {
  if (!module_base_menu_) return;

  const std::size_t ProfileStructSize = 2860;

  std::size_t zone_index = module_base_menu_ + 0x47F9C;
  uint16_t profile_index = process_.ReadU32(module_base_menu_ + 0x47FA0) & 0xFFFF;
  std::size_t profile_addr = process_.ReadU32(module_base_menu_ + 0x47A38);
  u32 count = process_.ReadU32(module_base_menu_ + 0x47A30);

  

  std::size_t addr = module_base_menu_ + 0x47000;

  std::ofstream f("profile_dump.bin", std::ios::binary);

  for (size_t i = 0; i < 50000; i++) {
    uint8_t b = process_.ReadU8(addr + i);
    f.put(b);
  }

  //uint8_t b = process_.ReadU8(profile_index);
  //f.put(b);

  f.close();
}

bool ContinuumGameProxy::SetMenuSelectedZone(std::string zone_name) {
  if (!module_base_menu_) return false;
  if (!IsOnMenu()) return false;

  struct ZoneInfo {
    char unknown1[76];
    unsigned char ip[4];  // Reversed due to endianness
    unsigned short port;
    char unknown2[64];
    char name[32];
    char unknown3[38];
  };

  static_assert(sizeof(ZoneInfo) == 216, "Zone size must be 216");

  ZoneInfo* zones = *(ZoneInfo**)(module_base_menu_ + 0x46C58);
  u16 zone_count = *(u16*)(module_base_menu_ + 0x46C54);
  u16* current_zone_index = (u16*)(module_base_menu_ + 0x47F9C);

  for (u16 i = 0; i < zone_count; ++i) {
    std::string_view name = zones[i].name;

    if (name.compare(zone_name) == 0) {
      *current_zone_index = i;
      return true;
    }
  }

  return false;
}

bool ContinuumGameProxy::SetMenuSelectedZone(uint16_t index) {
  if (!module_base_menu_) return false;
  if (!IsOnMenu()) return false;

  u16 zone_count = *(u16*)(module_base_menu_ + 0x46C54);
  u16* current_zone_index = (u16*)(module_base_menu_ + 0x47F9C);

  if (index >= zone_count) return false;

  *current_zone_index = index;

  return true;
}

std::string ContinuumGameProxy::GetMenuSelectedZoneName() {
  if (!module_base_menu_) return "";
  if (!IsOnMenu()) return "";

  struct ZoneInfo {
    char unknown1[76];
    unsigned char ip[4];  // Reversed due to endianness
    unsigned short port;
    char unknown2[64];
    char name[32];
    char unknown3[38];
  };

  static_assert(sizeof(ZoneInfo) == 216, "Zone size must be 216");

  ZoneInfo* zones = *(ZoneInfo**)(module_base_menu_ + 0x46C58);
  u16 zone_count = *(u16*)(module_base_menu_ + 0x46C54);
  u16* current_zone_index = (u16*)(module_base_menu_ + 0x47F9C);

  return zones[*current_zone_index].name;
}

uint16_t ContinuumGameProxy::GetMenuSelectedZoneIndex() {
  if (!module_base_menu_) return 0xFFFF;
  if (!IsOnMenu()) return 0xFFFF;

  return *(u16*)(module_base_menu_ + 0x47F9C);
}

}  // namespace marvin
