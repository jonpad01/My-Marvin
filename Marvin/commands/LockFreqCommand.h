#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class LockFreqCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>("FreqLock", false) == true) {
      if (bb.GetFreqLock()) {
      game.SendPrivateMessage(sender, "Marv was already locked.");
    } else {
      game.SendPrivateMessage(sender, "Switching from unlocked to locked.");
    }

    //bb.Set<bool>("FreqLock", true);
    bb.SetFreqLock(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"lockfreq", "lf"}; }
  std::string GetDescription() { return "Locks frequency"; }
  int GetSecurityLevel() { return 1; }
};

class UnlockFreqCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    //if (bb.ValueOr<bool>("FreqLock", false) == false) {
      if (!bb.GetFreqLock()) {
      game.SendPrivateMessage(sender, "Marv was already unlocked.");
    } else {
      game.SendPrivateMessage(sender, "Switching from locked to unlocked.");
    }

    //bb.Set<bool>("FreqLock", false);
    bb.SetFreqLock(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"unlockfreq", "uf"}; }
  std::string GetDescription() { return "Unlocks frequency"; }
  int GetSecurityLevel() { return 1; }
};

}  // namespace marvin
