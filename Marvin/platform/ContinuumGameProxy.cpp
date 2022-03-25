#include "ContinuumGameProxy.h"

#include <thread>
#include <vector>

#include "../Bot.h"
#include "../Debug.h"
#include "../Map.h"

/* Because this is injected code, there are limitations on what methods and win32 functions can be used.  The bot cannot wait or or use sleep functions or Continuum will crash.  
For example, PostMessage seems to work better because it does not wait for the call to return.  Instead, of waiting for an aknowledgement from a SendMessage call, the bot can send a
PostMessage call and read memory locations to know when a condition has been met.  This avoids possible hangs/crashes but probably results in spam/redundant PostMessage calls. */

namespace marvin {

ContinuumGameProxy::ContinuumGameProxy(HWND hwnd) : hwnd_(hwnd) {
  LoadGame();
}

void ContinuumGameProxy::LoadGame() {

  module_base_continuum_ = process_.GetModuleBase("Continuum.exe");
  module_base_menu_ = process_.GetModuleBase("menu040.dll");
  player_id_ = 0xFFFF;

  //these files probably get opened again if the LoadGame member ever gets called multiple times, need a better method
  marvin::debug_log.open(GetName() + ".log", std::ios::out | std::ios::trunc);
  marvin::memory_log.open(GetName() + " memory list.log", std::ios::out | std::ios::trunc);
  marvin::error_log.open(GetName() + " error list.log", std::ios::out | std::ios::app);

  game_addr_ = process_.ReadU32(module_base_continuum_ + 0xC1AFC);

  position_data_ = (uint32_t*)(game_addr_ + 0x126BC);

  mapfile_path_ = GetServerFolder() + "\\" + GetMapFile();

  map_ = Map::Load(mapfile_path_);

  FetchPlayers();
  SetZone();
  LogMemoryLocations();

  // Skip over existing messages on load
  u32 chat_base_addr = game_addr_ + 0x2DD08;
  chat_index_ = *(u32*)(chat_base_addr + 8);

  for (auto& player : players_) {
    if (player.name == GetName()) {
      player_id_ = player.id;
      player_ = &player;
    }
  }
}

bool ContinuumGameProxy::Update(float dt) {
  // Continuum stops processing input when it loses focus, so update the memory
  // to make it think it always has focus.
  SetWindowFocus();

  std::string map_file_path = GetServerFolder() + "\\" + GetMapFile();

  if (mapfile_path_ != map_file_path) {
    LoadGame();
    return false;
  }

    FetchPlayers();
    FetchBallData();
    FetchGreens();
    FetchChat();
    FetchWeapons();
  

  return true;
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
  const std::size_t kActiveOffset1 = 0x40;
  const std::size_t kActiveOffset2 = 0x4C;
  const std::size_t kShipOffset = 0x5C;
  const std::size_t kFreqOffset = 0x58;
  const std::size_t kStatusOffset = 0x60;
  const std::size_t kNameOffset = 0x6D;
  const std::size_t kDeadOffset = 0x178;
  const std::size_t kEnergyOffset1 = 0x208;
  const std::size_t kEnergyOffset2 = 0x20C;
  const std::size_t kMultiFireCapableOffset = 0x2EC;
  const std::size_t kMultiFireStatusOffset = 0x32C;

  std::size_t base_addr = game_addr_ + 0x127EC;
  std::size_t players_addr = base_addr + 0x884;
  std::size_t count_addr = base_addr + 0x1884;

  std::size_t count = process_.ReadU32(count_addr) & 0xFFFF;

  players_.clear();

  for (std::size_t i = 0; i < count; ++i) {
    std::size_t player_addr = process_.ReadU32(players_addr + (i * 4));

    if (!player_addr) continue;

    Player player;

    player.position.x = process_.ReadU32(player_addr + kPosOffset) / 1000.0f / 16.0f;

    player.position.y = process_.ReadU32(player_addr + kPosOffset + 4) / 1000.0f / 16.0f;

    player.velocity.x = process_.ReadI32(player_addr + kVelocityOffset) / 10.0f / 16.0f;

    player.velocity.y = process_.ReadI32(player_addr + kVelocityOffset + 4) / 10.0f / 16.0f;

    player.id = static_cast<uint16_t>(process_.ReadU32(player_addr + kIdOffset));

    player.discrete_rotation = static_cast<uint16_t>(process_.ReadU32(player_addr + kRotOffset) / 1000);

    player.ship = static_cast<uint16_t>(process_.ReadU32(player_addr + kShipOffset));

    player.frequency = static_cast<uint16_t>(process_.ReadU32(player_addr + kFreqOffset));

    player.status = static_cast<uint8_t>(process_.ReadU32(player_addr + kStatusOffset));

    player.multifire_status = static_cast<uint8_t>(process_.ReadU32(player_addr + kMultiFireStatusOffset));

    player.multifire_capable = (process_.ReadU32(player_addr + kMultiFireCapableOffset)) & 0x8000;

    player.name = process_.ReadString(player_addr + kNameOffset, 23);

    player.bounty = *(u32*)(player_addr + kBountyOffset1) + *(u32*)(player_addr + kBountyOffset2);

    player.flags = *(u32*)(player_addr + kFlagOffset1) + *(u32*)(player_addr + kFlagOffset2);

    // triggers if player is not moving and has no velocity, not used
    player.dead = process_.ReadU32(player_addr + kDeadOffset) == 0;

    // the id of the player the bot is attached to, a value of -1 means its not attached, or 65535
    player.attach_id = process_.ReadU32(player_addr + 0x1C);
    // might not work on bot but works well on other players
    player.active = *(u32*)(player_addr + kActiveOffset1) == 0 && *(u32*)(player_addr + kActiveOffset2) == 0;

    // Energy calculation @4485FA
    if (player.id == player_id_) {
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

    player.repels = *(u32*)(player_addr + 0x2B0) + *(u32*)(player_addr + 0x2B4);
    player.bursts = *(u32*)(player_addr + 0x2B8) + *(u32*)(player_addr + 0x2BC);
    player.decoys = *(u32*)(player_addr + 0x2D8) + *(u32*)(player_addr + 0x2DC);
    player.thors = *(u32*)(player_addr + 0x2D0) + *(u32*)(player_addr + 0x2D4);
    player.bricks = *(u32*)(player_addr + 0x2C0) + *(u32*)(player_addr + 0x2C4);
    player.rockets = *(u32*)(player_addr + 0x2C8) + *(u32*)(player_addr + 0x2CC);
    player.portals = *(u32*)(player_addr + 0x2E0) + *(u32*)(player_addr + 0x2E4);

    // remove "^" that gets placed on names when biller is down
    if (!player.name.empty() && player.name[0] == '^') {
      player.name.erase(0, 1);
    }

    players_.emplace_back(player);

    if (player.id == player_id_) {
      // RenderText("status  " + std::to_string((player.status)), GetWindowCenter() - Vector2f(0.0f, 60.0f),
      // RGB(100, 100, 100), RenderText_Centered); RenderText("test  " + std::to_string(test), GetWindowCenter() -
      // Vector2f(0.0f, 40.0f), RGB(100, 100, 100), RenderText_Centered);
      player_ = &players_.back();
      player_addr_ = player_addr;
    }
  }
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

  recent_chat_.clear();

  for (int i = 0; i < read_count; ++i) {
    if (chat_index_ >= 1024) {
      chat_index_ -= 64;
    }

    ChatEntry* entry = chat_ptr + chat_index_;
    ChatMessage chat;

    chat.message = entry->message;
    chat.player = entry->player;
    chat.type = entry->type;

    recent_chat_.push_back(chat);
    ++chat_index_;
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

  for (size_t i = 0; i < weapon_count; ++i) {
    u32 weapon_data = *(u32*)(weapon_ptrs + i * 4);

    WeaponMemory* data = (WeaponMemory*)(weapon_data);

    weapons_.emplace_back(data);
  }
}

void ContinuumGameProxy::SetZone() {
  zone_ = Zone::Devastation;

  if (GetServerFolder() == "zones\\SSCJ Devastation") {
    zone_ = Zone::Devastation;
  } else if (GetServerFolder() == "zones\\SSCU Extreme Games") {
    zone_ = Zone::ExtremeGames;
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

const Zone ContinuumGameProxy::GetZone() {
  return zone_;
}

std::vector<ChatMessage> ContinuumGameProxy::GetChat() const {
  return recent_chat_;
}

const std::vector<BallData>& ContinuumGameProxy::GetBalls() const {
  return balls_;
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

const ShipSettings& ContinuumGameProxy::GetShipSettings() const {
  return GetSettings().ShipSettings[player_->ship];
}

const ShipSettings& ContinuumGameProxy::GetShipSettings(int ship) const {
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

const float ContinuumGameProxy::GetEnergyPercent() {
  return ((float)GetEnergy() / GetMaxEnergy()) * 100.0f;
}

const float ContinuumGameProxy::GetMaxEnergy() {
  // zones like eg use initial energy but can be prized to increase max energy
  // hs probably hacks the initial energy for each player when they enter
  // deva needs to use max and it never changes
  float energy = (float)GetShipSettings().InitialEnergy;

  if (zone_ == Zone::Devastation) {
    energy = (float)GetShipSettings().MaximumEnergy;
  }
  if (zone_ == Zone::ExtremeGames) {
    while (player_->energy > energy) {
      energy += (float)GetShipSettings().UpgradeEnergy;
    }
  }
  return energy;
}

const float ContinuumGameProxy::GetMaxSpeed() {
  float speed = (float)GetShipSettings().InitialSpeed / 10.0f / 16.0f;

  if (zone_ == Zone::Devastation) {
    speed = (float)GetShipSettings().MaximumSpeed / 10.0f / 16.0f;
  }

  if (player_->velocity.Length() > speed) {
    speed = std::abs(speed + GetShipSettings().GravityTopSpeed);
  }
  return speed;
}

const float ContinuumGameProxy::GetRotation() {
  float rotation = (float)GetShipSettings().InitialRotation / 200.0f;

  if (zone_ == Zone::Devastation) {
    rotation = (float)GetShipSettings().MaximumRotation / 200.0f;
  }
  return rotation;
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

void ContinuumGameProxy::SetEnergy(float percent) {
  const uint64_t overflow = 4294967296;

  double max_energy = (double)GetMaxEnergy();

  u64 desired = u64(max_energy * (percent / 100));

  u64 mem_energy = (desired * 1000) + overflow;

  process_.WriteU32(player_addr_ + 0x208, (u32)(mem_energy / 2));
  process_.WriteU32(player_addr_ + 0x20C, (u32)(mem_energy / 2));
}

void ContinuumGameProxy::SetFreq(int freq) {
  SendChatMessage("=" + std::to_string(freq));
}

void ContinuumGameProxy::PageUp() {
  SendKey(VK_PRIOR);
}
void ContinuumGameProxy::PageDown() {
  SendKey(VK_NEXT);
}
void ContinuumGameProxy::XRadar() {
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

void ContinuumGameProxy::F7() {
  SendKey(VK_F7);
}

bool ContinuumGameProxy::SetShip(uint16_t ship) {
  int* menu_open_addr = (int*)(game_addr_ + 0x12F39);

  bool menu_open = *menu_open_addr;

  if (!menu_open) {
    SendKey(VK_ESCAPE);
  } else {
    if (ship == 8) {
      PostMessage(hwnd_, WM_CHAR, (WPARAM)('s'), 0);
    } else {
      PostMessage(hwnd_, WM_CHAR, (WPARAM)('1' + ship), 0);
    }
  }

  return menu_open;
}

void ContinuumGameProxy::Warp() {
  SendKey(VK_INSERT);
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

void ContinuumGameProxy::Flag() {
  SendChatMessage("?flag");
}

void ContinuumGameProxy::P() {
  SendChatMessage("!p");
}

void ContinuumGameProxy::L() {
  SendChatMessage("!l");
}

void ContinuumGameProxy::R() {
  SendChatMessage("!r");
}

void ContinuumGameProxy::Attach(std::string name) {
  SendChatMessage(":" + name + ":?attach");
}

void ContinuumGameProxy::SetWindowFocus() {
  std::size_t focus_addr = game_addr_ + 0x3039c;

  process_.WriteU32(focus_addr, 1);
}

ExeProcess& ContinuumGameProxy::GetProcess() {
  return process_;
}

void ContinuumGameProxy::SendKey(int vKey) {
  PostMessage(hwnd_, WM_KEYDOWN, (WPARAM)vKey, 0);
  PostMessage(hwnd_, WM_KEYUP, (WPARAM)vKey, 0);
}

void ContinuumGameProxy::SendChatMessage(const std::string& mesg) const {
  typedef void(__fastcall * ChatSendFunction)(void* This, void* thiscall_garbage, char* msg, u32* unknown1,
                                              u32* unknown2);

  if (mesg.empty()) return;

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
}

void ContinuumGameProxy::SendPrivateMessage(const std::string& target, const std::string& mesg) const {
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
  for (std::size_t i = 0; i < players_.size(); ++i) {
    const Player& player = players_[i];

    if (player.id == id) {
      *(u32*)(game_addr_ + 0x2DED0) = i;
      return;
    }
  }
}

void ContinuumGameProxy::LogMemoryLocations() {
  
    marvin::memory_log << "A list of memory addresses read by Marvin." << std::endl;


  marvin::memory_log << "Module Base: - " << module_base_continuum_ << std::endl;
  marvin::memory_log << "Module Base Menu: - " << module_base_menu_ << std::endl;
  marvin::memory_log << "Game Address: - " << game_addr_ << std::endl;
  marvin::memory_log << "Position Data: - " << game_addr_ + 0x126BC << std::endl;

  std::size_t base_addr = game_addr_ + 0x127EC;
  marvin::memory_log << "Base Address: - " << game_addr_ << std::endl;
  std::size_t players_addr = base_addr + 0x884;
  marvin::memory_log << "Players Address: - " << players_addr << std::endl;
  std::size_t count_addr = base_addr + 0x1884;
  marvin::memory_log << "Count Address: - " << count_addr << std::endl;

  std::size_t count = process_.ReadU32(count_addr) & 0xFFFF;

  for (std::size_t i = 0; i < count; ++i) {
    std::size_t player_addr = process_.ReadU32(players_addr + (i * 4));
    if (!player_addr) continue;
    marvin::memory_log << "Player Address: - " << player_addr << std::endl;
    for (std::size_t i = 0; i < offsets_.size(); i++) {
      marvin::memory_log << offset_titles_[i] << " - " << player_addr + offsets_[i] << std::endl;
    }
  }

}

}  // namespace marvin
