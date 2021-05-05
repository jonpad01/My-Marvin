#pragma once
#include "GameProxy.h"



namespace marvin {

	std::string PrefixCheck(std::string message);

	bool HasAccess(std::string player);

	void SendHelpMenu(GameProxy& game, std::string player);

	void SendCommandsMenu(GameProxy& game, std::string player);

	void SendModList(GameProxy& game, std::string player);

	
}