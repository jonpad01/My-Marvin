#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class SwarmCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("Swarm", false) == true) {
      game.SendPrivateMessage(sender, "Marv was already swarming.");
    } else {
      game.SendPrivateMessage(sender, "Switching swarm mode on.");
    }

    bb.Set<bool>("Swarm", true);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"swarm", "s"}; }
  std::string GetDescription() {
    return "Enable swarm behavior. Bots will respawn quickly with low health when basing";
  }
  int GetSecurityLevel() { return 1; }
};

class SwarmOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("Swarm", false) == false) {
      game.SendPrivateMessage(sender, "Swarm mode was already off.");
    } else {
      game.SendPrivateMessage(sender, "Switching swarm mode off.");
    }

    bb.Set<bool>("Swarm", false);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"swarmoff", "so"}; }
  std::string GetDescription() { return "Sets swarm off"; }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin
