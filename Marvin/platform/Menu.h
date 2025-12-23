#pragma once

#include <string>

#include "ExeProcess.h"
#include "..//GameProxy.h"

namespace marvin {

	struct ProfileData {
		std::string name;
		std::string password;
		std::string zone_name;
		std::string chats;
		int ship = 0;
		int window_mode = 0;
	};

	class Menu {
	public:
		Menu();

		bool IsOnMenu();
		std::string GetSelectedProfileName();
		bool SelectProfile(const std::string& player_name);
		bool WriteToSelectedProfile(const ProfileData& data);
		std::string GetSelectedZoneName();
		uint16_t GetSelectedZoneIndex();
		bool SetSelectedZone(std::string zone_name);
		bool SetSelectedZone(uint16_t index);

	private:
		std::size_t module_base_menu_ = 0;
		ExeProcess process_;
	};

}