#include "Menu.h"
#include "..\\Debug.h"

const char* MODULE_NAME = "menu040.dll";

namespace marvin {

	Menu::Menu() {
		module_base_menu_ = process_.GetModuleBase(MODULE_NAME);

    if (!module_base_menu_) {
      // no log file to record to yet
      MessageBoxA(nullptr, "Did not get module base menu!", "Yikes", MB_OK);
    }
	}

  bool Menu::IsInitialized() {
    if (!module_base_menu_) {
      module_base_menu_ = process_.GetModuleBase(MODULE_NAME);
    }

    return module_base_menu_ != 0;
  }

  bool Menu::IsOnMenu() {
    if (!module_base_menu_) return false;
    return *(uint8_t*)(module_base_menu_ + 0x47a84) == 0;
  }

// the selected profile index captured here is valid until the game gets closed back into the menu
// i had some bug where it was being changed
  std::string Menu::GetSelectedProfileName() {

    if (!module_base_menu_) return "";

    const std::size_t ProfileStructSize = 2860;

    uint16_t profile_index = process_.ReadU32(module_base_menu_ + 0x47FA0) & 0xFFFF;
    std::size_t addr = process_.ReadU32(module_base_menu_ + 0x47A38) + 0x15;

    if (!addr) return "";

    addr += profile_index * ProfileStructSize;
    std::string name = process_.ReadString(addr, 23);

    // remove "^" that gets placed on names when biller is down
    if (!name.empty() && name[0] == '^') {
      name.erase(name.begin());
    }

    name = name.substr(0, strlen(name.c_str()));

    return name;
  }

  bool Menu::SelectProfile(const std::string& player_name) {
    if (!module_base_menu_) return false;
    if (!IsOnMenu()) return false;

    const std::size_t ProfileStructSize = 2860;
    std::size_t addr = process_.ReadU32(module_base_menu_ + 0x47A38) + 0x15;
    uint32_t count = process_.ReadU32(module_base_menu_ + 0x47A30);  // size of profile list

    std::string name;

    if (addr == 0) {
      return false;
    }

    for (uint16_t i = 0; i < count; i++) {

      name = process_.ReadString(addr, 23);

      if (name == player_name) {
        *(uint32_t*)(module_base_menu_ + 0x47FA0) = i;
        return true;
      }

      addr += ProfileStructSize;
    }

    return false;
  }

  bool Menu::WriteToSelectedProfile(const ProfileData& data) {
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

#if 0 // couldnt get the struct to align, maybe just need to pack it?
#pragma pack(push, 1)
    struct MenuProfile {
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
    };
#pragma pack(pop)
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

  bool Menu::SetSelectedZone(std::string zone_name) {
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

  bool Menu::SetSelectedZone(uint16_t index) {
    if (!module_base_menu_) return false;
    if (!IsOnMenu()) return false;

    u16 zone_count = *(u16*)(module_base_menu_ + 0x46C54);
    u16* current_zone_index = (u16*)(module_base_menu_ + 0x47F9C);

    if (index >= zone_count) return false;

    *current_zone_index = index;

    return true;
  }

  std::string Menu::GetSelectedZoneName() {
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

  uint16_t Menu::GetSelectedZoneIndex() {
    if (!module_base_menu_) return 0xFFFF;
    if (!IsOnMenu()) return 0xFFFF;

    return *(u16*)(module_base_menu_ + 0x47F9C);
  }

}