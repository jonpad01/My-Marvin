#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class LockCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("CmdLock", false) == true) {
      game.SendPrivateMessage(sender, "Marv was already locked.");
    } else {
      game.SendPrivateMessage(sender, "Switching from unlocked to locked.");
    }

    bb.Set<bool>("CmdLock", true);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"lockmarv", "lm", "lock"}; }
  std::string GetDescription() { return "Locks marv so only mods can make changes"; }
  int GetSecurityLevel() { return 1; }
};

class UnlockCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("CmdLock", false) == false) {
      game.SendPrivateMessage(sender, "Marv was already unlocked.");
    } else {
      game.SendPrivateMessage(sender, "Switching from locked to unlocked.");
    }

    bb.Set<bool>("CmdLock", false);
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"unlockmarv", "um", "unlock"}; }
  std::string GetDescription() { return "Unlocks marv so players can make changes"; }
  int GetSecurityLevel() { return 1; }
};

}  // namespace marvin
