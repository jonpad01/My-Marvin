#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class SwarmCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender,
               const std::string& alias, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status && alias == "swarm") {
      if (bb.GetSwarm()) {
        game.SendPrivateMessage(sender, "swarm currently ON.");
      } else {
        game.SendPrivateMessage(sender, "swarm currently OFF.");
      }
    } else {
      if (alias == "swarmon" || !args.empty() && args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning swarm ON.");
        bb.SetSwarm(true);
      } else if (alias == "swarmoff" || !args.empty() && args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning swarm OFF.");
        bb.SetSwarm(false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!swarm on\" or \"!swarm off\" or \"!swarm\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"swarm", "swarmon", "swarmoff"}; }
  std::string GetDescription() {
    return "Enable swarm behavior. Bots will respawn quickly with low health when basing";
  }
  int GetSecurityLevel() { return 1; }
  CommandType GetCommandType() { return CommandType::Behavior; }
};

}  // namespace marvin
