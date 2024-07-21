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

ContinuumGameProxy::ContinuumGameProxy(HWND hwnd) : hwnd_(hwnd) {
  set_ship_status_ = SetShipStatus::Clear;
  reset_ship_status_ = SetShipStatus::Clear;
  UpdateState game_status_ = UpdateState::Clear;
  set_freq_flag_ = 0;
  desired_ship_ = 0;
  desired_freq_ = 0;
  original_ship_ = 0;
  attach_cooldown_ = 0;
  message_cooldown_ = 0;
  setfreq_cooldown_ = 0;
  setship_cooldown_ = 0;
  flag_cooldown_ = 0;
  delay_timer_ = 0;
  tries_ = 0;

  module_base_continuum_ = process_.GetModuleBase("Continuum.exe");
  module_base_menu_ = process_.GetModuleBase("menu040.dll");
  player_id_ = 0xFFFF;

  log.Open(GetName());
  log.Write("Log file created.");

  LoadGame();
}

bool ContinuumGameProxy::LoadGame() {
  PerformanceTimer timer;
  log.Write("LOADING GAME.", timer.GetElapsedTime());
  game_addr_ = process_.ReadU32(module_base_continuum_ + 0xC1AFC);
  position_data_ = (uint32_t*)(game_addr_ + 0x126BC);

  mapfile_path_ = GetServerFolder() + "\\" + GetMapFile();
  map_ = Map::Load(mapfile_path_);

  // map will be null if downloading a new map file
  if (map_ == nullptr) {
    game_status_ = UpdateState::Reload;
    return false;
  }

  log.Write("Map loaded: " + mapfile_path_, timer.GetElapsedTime());

  SetZone();
  FetchPlayers();
  log.Write("Players fetched.", timer.GetElapsedTime());

  // Skip over existing messages on load
  u32 chat_base_addr = game_addr_ + 0x2DD08;
  chat_index_ = *(u32*)(chat_base_addr + 8);

  for (auto& player : players_) {
    if (player.name == GetName()) {
      player_id_ = player.id;
      player_ = &player;
      FetchPlayers();  // get one more time to set the player_addr_ private variable
    }
  }

  log.Write("Game Loaded. TOTAL TIME", timer.TimeSinceConstruction());
  game_status_ = UpdateState::Clear;
  return true;
}

UpdateState ContinuumGameProxy::Update(float dt) {
  // Continuum stops processing input when it loses focus, so update the memory
  // to make it think it always has focus.
  SetWindowFocus();

  std::string map_file_path = GetServerFolder() + "\\" + GetMapFile();

  if (game_status_ == UpdateState::Reload) {
    if (LoadGame()) {
      return UpdateState::Reload;
    } else {
      return UpdateState::Wait;
    }
  }

  if (mapfile_path_ != map_file_path) {
    log.Write("New map file detected.");
    log.Write("Old map - " + mapfile_path_ + "  New map - " + map_file_path);
    if (!LoadGame()) {
      return UpdateState::Wait;
    }
    return UpdateState::Reload;
  }

  FetchPlayers();
  FetchBallData();
  FetchDroppedFlags();
  FetchGreens();
  FetchChat();
  FetchWeapons();

  if (set_ship_status_ == SetShipStatus::SetShip) {
    if (SetShip(desired_ship_)) {
      return UpdateState::Wait;
    }
  }

  if (reset_ship_status_ == SetShipStatus::ResetShip) {
    ResetShip();
    return UpdateState::Wait;
  }

  if (set_freq_flag_) {
    SetFreq(desired_freq_);  // must fetch players before this check to update player the frequency
    return UpdateState::Wait;
  }

  
  if (ProcessQueuedMessages()) {
    return UpdateState::Wait;
  }

  map_->SetMinedTiles(GetEnemyMines());

  return UpdateState::Clear;
}

void ContinuumGameProxy::FetchPlayers() {
  const std::size_t kPosOffset = 0x04;
  const std::size_t kVelocityOffset = 0x10;
  const std::size_t kIdOffset = 0x18;
  const std::size_t kBountyOffset1 = 0x20;
  const std::size_t kBountyOffset2 = 0x24;
  const std::size_t kFlagOffset1 = 0x30;
  const std::size_t kFlagOffset2 = 0x34;
  const std::size_t kRotOffset = 0x3C;
  const std::size_t kExplodeTimerOffset1 = 0x40; // count down timer for the exploding graphic when player dies
  const std::size_t kDeadOffset = 0x4C;  // a possible death flag
  const std::size_t kShipOffset = 0x5C;
  const std::size_t kFreqOffset = 0x58;
  const std::size_t kStatusOffset = 0x60;
  const std::size_t kNameOffset = 0x6D;
  const std::size_t kDeathCountOffset = 0xD0; // total player death count tracked by the server
  const std::size_t kUnknownTimestampOffset = 0x13C;  // don't know what this is
  const std::size_t kPlayerLastPositionTimestampOffset = 0x14C;  // 0 for the main player in most cases
  const std::size_t kPlayerPing = 0x164;  // same number that's displayed by player names
  const std::size_t kPlayerActiveOffset = 0x178;  // triggers if the player is just idling in a ship at near 0 velocity, or dead
  const std::size_t kEnergyOffset1 = 0x208;
  const std::size_t kEnergyOffset2 = 0x20C;
  const std::size_t kMultiFireCapableOffset = 0x2EC;
  const std::size_t kMultiFireStatusOffset = 0x32C;

  std::size_t base_addr = game_addr_ + 0x127EC;
  std::size_t players_addr = base_addr + 0x884;
  std::size_t count_addr = base_addr + 0x1884;

  std::size_t count = process_.ReadU32(count_addr) & 0xFFFF;

  players_.clear();
  id_list_.clear();
  name_list_.clear();

  for (std::size_t i = 0; i < count; ++i) {
    std::size_t player_addr = process_.ReadU32(players_addr + (i * 4));

    if (!player_addr) continue;

    Player player;

    player.name = process_.ReadString(player_addr + kNameOffset, 23);
    

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
    if (player.id == player_id_) {
      player.flight_status.rotation = *(u32*)(player_addr + 0x278) + *(u32*)(player_addr + 0x274);
      player.flight_status.recharge = *(u32*)(player_addr + 0x1E8) + *(u32*)(player_addr + 0x1EC);
      player.flight_status.shrapnel = *(u32*)(player_addr + 0x2A8) + *(u32*)(player_addr + 0x2AC);
      player.flight_status.thrust = *(u32*)(player_addr + 0x244) + *(u32*)(player_addr + 0x248);
      g_RenderState.RenderDebugText("Player Thrust 1: %u", *(u32*)(player_addr + 0x244));
      g_RenderState.RenderDebugText("Player Thrust 2: %u", *(u32*)(player_addr + 0x248));
      g_RenderState.RenderDebugText("Player Thrust: %u", *(u32*)(player_addr + 0x244) + *(u32*)(player_addr + 0x248));
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
    }

    // remove "^" that gets placed on names when biller is down
    if (!player.name.empty() && player.name[0] == '^') {
      player.name.erase(0, 1);
    }

    // only include players in ships and ignore hyperspace fake players
    if (player.ship != 8 && player.name.size() > 0 && player.name[0] != '<') {
      id_list_.emplace_back(player.id);
      name_list_.emplace_back(player.name);
    }

    // sort all players into this
    players_.emplace_back(player);

    if (player.id == player_id_) {
      player_ = &players_.back();
      player_addr_ = player_addr;

#if DEBUG_RENDER_GAMEPROXY
      g_RenderState.RenderDebugText("Player Name: %s", player.name.c_str());
      g_RenderState.RenderDebugText("XRadar Capable: %u", player.capability.xradar);
      g_RenderState.RenderDebugInBinary("Status: ", player.status);  // print in binary
      g_RenderState.RenderDebugText("Player Energy: %u", player.energy);
      g_RenderState.RenderDebugText("Player Max Energy: %u", player.flight_status.max_energy);
#endif

    }



  }


  // sorted list is used to get a unique timer offset for each bot based on bots id
  std::sort(id_list_.begin(), id_list_.end());
  std::sort(name_list_.begin(), name_list_.end());
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

  current_chat_.clear();  // flush out old chat 

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

    float total_milliseconds = 0.0f;
    bool place_mine = false;

    switch (type) {
      case WeaponType::Bomb:
      case WeaponType::ProximityBomb:
      case WeaponType::Thor: {
        total_milliseconds = this->GetSettings().GetBombAliveTime();
        if (data->data.alternate) {
          total_milliseconds = this->GetSettings().GetMineAliveTime();
          const Player* weapon_player = GetPlayerById(data->pid);
          if (weapon_player != nullptr && weapon_player->frequency != GetPlayer().frequency) {
            place_mine = true;
          }
        }
      } break;
      case WeaponType::Burst:
      case WeaponType::Bullet:
      case WeaponType::BouncingBullet: {
        total_milliseconds = this->GetSettings().GetBulletAliveTime();
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
    float alive_milliseconds = data->alive_ticks / 100.0f;
    if (alive_milliseconds > total_milliseconds) alive_milliseconds = total_milliseconds;

    weapons_.emplace_back(data, total_milliseconds - alive_milliseconds);
    if (place_mine) enemy_mines_.emplace_back(data, total_milliseconds - alive_milliseconds);
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

const std::vector<Flag>& ContinuumGameProxy::GetDroppedFlags() {
  return dropped_flags_;
}

void ContinuumGameProxy::SetZone() {
  zone_ = Zone::Devastation;

  if (GetServerFolder() == "zones\\SSCJ Devastation") {
    zone_ = Zone::Devastation;
  } else if (GetServerFolder() == "zones\\SSCU Extreme Games") {
    zone_ = Zone::ExtremeGames;  // EG does not allow any memory writing and will kick the bot
  } else if (GetServerFolder() == "zones\\SSCJ Galaxy Sports") {
    zone_ = Zone::GalaxySports;
  } else if (GetServerFolder() == "zones\\SSCE HockeyFootball Zone") {
    zone_ = Zone::Hockey;
  } else if (GetServerFolder() == "zones\\SSCE Hyperspace") {
    zone_ = Zone::Hyperspace;
  } else if (GetServerFolder() == "zones\\SSCJ PowerBall") {
    zone_ = Zone::PowerBall;
  }
}

// const ShipFlightStatus& ContinuumGameProxy::GetShipStatus() const {
//   return ship_status_;
// }

const Zone ContinuumGameProxy::GetZone() { return zone_; }


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
  std::size_t addr = game_addr_ + 0x127EC + 0x1AE70;  // 0x2D65C

  return *reinterpret_cast<ClientSettings*>(addr);
}

// there are no ship settings for spectators
const ShipSettings& ContinuumGameProxy::GetShipSettings() const {
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

std::string ContinuumGameProxy::GetName() const {
  const std::size_t ProfileStructSize = 2860;

  uint16_t profile_index = process_.ReadU32(module_base_menu_ + 0x47FA0) & 0xFFFF;
  std::size_t addr = process_.ReadU32(module_base_menu_ + 0x47A38) + 0x15;

  if (addr == 0) {
    return "";
  }

  addr += profile_index * ProfileStructSize;

  std::string name = process_.ReadString(addr, 23);

  // remove "^" that gets placed on names when biller is down
  if (!name.empty() && name[0] == '^') {
    name.erase(0, 1);
  }

  name = name.substr(0, strlen(name.c_str()));

  return name;
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

void ContinuumGameProxy::SetFreq(int freq) {
#if !DEBUG_USER_CONTROL
  if (time_.GetTime() < setfreq_cooldown_) {
  //if (!ActionDelay() || freq == player_->frequency) {
    return;
  }

  if (freq == player_->frequency) {
    set_freq_flag_ = false;
    return;
  }

  if (player_->dead) ResetShip();

  set_freq_flag_ = true;
  desired_freq_ = freq;
  tries_++;

  if (tries_ > 10) {
    set_freq_flag_ = false;
    tries_ = 0;
    return;
  }
  

  ResetStatus();
  SetEnergy(100, "frequency change");
  
  //SendPriorityMessage("=" + std::to_string(freq));
  SendQueuedMessage("=" + std::to_string(freq));
  setfreq_cooldown_ = time_.GetTime() + 175;
#endif
}

bool ContinuumGameProxy::ResetShip() {
#if !DEBUG_USER_CONTROL

  if (time_.GetTime() < setship_cooldown_) return false;

  uint16_t ship = player_->ship + 1;
  if (ship >= 8) ship -= 2;

  if (reset_ship_status_ == SetShipStatus::Clear) {
    SetShip(ship);
    original_ship_ = player_->ship;
    reset_ship_status_ = SetShipStatus::ResetShip;
    setship_cooldown_ = time_.GetTime() + 175;
  } else if (original_ship_ != player_->ship) {
     SetShip(original_ship_);
     reset_ship_status_ = SetShipStatus::Clear;
     setship_cooldown_ = time_.GetTime() + 175;
     return true;
  }
  return false;
#endif
}

bool ContinuumGameProxy::SetShip(uint16_t ship) {
    // make sure ship is in bounds
    if (ship > 8) ship = 0;

    if (time_.GetTime() < setship_cooldown_) return false;

    int* menu_open_addr = (int*)(game_addr_ + 0x12F39);
    bool menu_open = (*menu_open_addr & 1);

    if (player_->ship == ship) {
      set_ship_status_ = SetShipStatus::Clear;
      if (menu_open) SendKey(VK_ESCAPE);
      return true;
    }

    set_ship_status_ = SetShipStatus::SetShip;
    desired_ship_ = ship;

#if !DEBUG_USER_CONTROL
    if (!menu_open) {
      SendKey(VK_ESCAPE);
    } else {
      // if (ActionDelay()) {
      ResetStatus();
      SetEnergy(100, "ship change");
      if (ship == 8) {
      PostMessage(hwnd_, WM_CHAR, (WPARAM)('s'), 0);
      } else {
      PostMessage(hwnd_, WM_CHAR, (WPARAM)('1' + ship), 0);
      }
      setship_cooldown_ = time_.GetTime() + 175;
      return true;
      // }
    }
#endif

    return false;
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

    ResetStatus();
  SetEnergy(100, "arena change");

  SendChatMessage("?go " + arena);
}



void ContinuumGameProxy::Warp() {
  if (!player_->dead && player_->ship != 8) {

      ResetStatus();
    SetEnergy(100, "warping");

    SendKey(VK_INSERT);
  }
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

void ContinuumGameProxy::SendKey(int vKey) const {
#if !DEBUG_USER_CONTROL
  PostMessage(hwnd_, WM_KEYDOWN, (WPARAM)vKey, 0);
  PostMessage(hwnd_, WM_KEYUP, (WPARAM)vKey, 0);
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

  for (std::string name : kBotNames) {
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

 #if 0
  if (mesg.empty()) return;

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
  memcpy(input, mesg.c_str(), mesg.length());
  input[mesg.length()] = 0;

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

int64_t ContinuumGameProxy::TickerPosition() {
  // this reads the same adress as get selected player but returns a number
  // used to determine if the bot should page up or page down
  u32 addr = game_addr_ + 0x2DF44;

  int64_t selected = process_.ReadU32(addr) & 0xFFFF;
  return selected;
}

const Player& ContinuumGameProxy::GetSelectedPlayer() const {
  u32 selected_index = *(u32*)(game_addr_ + 0x127EC + 0x1B758);

  return players_[selected_index];
}

const uint32_t ContinuumGameProxy::GetSelectedPlayerIndex() const {
  u32 selected_index = *(u32*)(game_addr_ + 0x127EC + 0x1B758);
  return selected_index;
}

void ContinuumGameProxy::SetSelectedPlayer(uint16_t id) {
#if !DEBUG_USER_CONTROL
  for (std::size_t i = 0; i < players_.size(); ++i) {
    const Player& player = players_[i];

    if (player.id == id) {
      *(u32*)(game_addr_ + 0x2DED0) = i;
      return;
    }
  }
#endif
}

// use id index to offset the bots delay from other bots
// that might be playing, prevents spam like
// a bunch of bots all jumping onto the same freq
bool ContinuumGameProxy::ActionDelay() {
  if (delay_timer_ == 0) {
    delay_timer_ = time_.GetTime() + (((uint64_t)GetIDIndex() * 200) + 100);
  } else if (time_.GetTime() > delay_timer_) {
    delay_timer_ = 0;
    return true;
  }
  return false;
}

// if all player names are sorted aphabetically, get this bots index in the list
std::size_t ContinuumGameProxy::GetIDIndex() {
  std::size_t i = 0;

  if (!player_) return i;

  for (i = 0; i < id_list_.size(); i++) {
    if (id_list_[i] == player_->id) {
      return i;
    }
  }
  return i;
}

}  // namespace marvin
