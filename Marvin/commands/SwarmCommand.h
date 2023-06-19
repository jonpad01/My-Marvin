#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class SwarmCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>("Swarm", false) == true) {
      if (bb.GetSwarm()) {
      game.SendPrivateMessage(sender, "Marv was already swarming.");
    } else {
      game.SendPrivateMessage(sender, "Switching swarm mode on.");
    }

    //bb.Set<bool>("Swarm", true);
    bb.SetSwarm(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"swarm"}; }
  std::string GetDescription() {
    return "Enable swarm behavior. Bots will respawn quickly with low health when basing";
  }
  int GetSecurityLevel() { return 1; }
};

class SwarmOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>("Swarm", false) == false) {
      if (!bb.GetSwarm()) {
      game.SendPrivateMessage(sender, "Swarm mode was already off.");
    } else {
      game.SendPrivateMessage(sender, "Switching swarm mode off.");
    }

    //bb.Set<bool>("Swarm", false);
    bb.SetSwarm(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"swarmoff"}; }
  std::string GetDescription() { return "Sets swarm off"; }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin
