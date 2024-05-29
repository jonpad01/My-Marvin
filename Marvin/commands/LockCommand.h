#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class LockCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    std::vector<std::string> args = Tokenize(arg, ' ');
    bool status = false;

    if (args.empty()) status = true;

    if (status) {
      if (bb.GetCommandLock()) {
        game.SendPrivateMessage(sender, "lock currently ON.");
      } else {
        game.SendPrivateMessage(sender, "lock currently OFF.");
      }
    } else {
      if (args[0] == "on") {
        game.SendPrivateMessage(sender, "Turning lock ON.");
        bb.SetCommandLock(true);
      } else if (args[0] == "off") {
        game.SendPrivateMessage(sender, "Turning lock OFF.");
        bb.SetCommandLock(false);
      } else {
        SendUsage(game, sender);
      }
    }
  }

  void SendUsage(GameProxy& game, const std::string& sender) {
    game.SendPrivateMessage(sender, "Use \"!lock on\" or \"!lock off\" or \"!lock\" (sends current status).");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"lock"}; }
  std::string GetDescription() { return "Locks/unlocks bot so only mods can make changes"; }
  int GetSecurityLevel() { return 5; }
  CommandType GetCommandType() { return CommandType::Action; }
};

}  // namespace marvin
