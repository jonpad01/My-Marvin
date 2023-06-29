#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class HSFlagCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    // if (bb.ValueOr<bool>("CmdLock", false) == true) {
    if (bb.GetCanFlag()) {
      game.SendPrivateMessage(sender, "Marv was already flagging.");
    } else {
      game.SendPrivateMessage(sender, "Switching to flagging.");
    }

    // bb.Set<bool>("CmdLock", true);
    bb.SetCanFlag(true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"flag"}; }
  std::string GetDescription() { return "Allows bot to jump on to a flag team"; }
  int GetSecurityLevel() { return 0; }
};

class HSFlagOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    // if (bb.ValueOr<bool>("CmdLock", false) == true) {
    if (!bb.GetCanFlag()) {
      game.SendPrivateMessage(sender, "Marv was not flagging.");
    } else {
      game.SendPrivateMessage(sender, "Marv will stop flagging.");
    }

    // bb.Set<bool>("CmdLock", true);
    bb.SetCanFlag(false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"flagoff"}; }
  std::string GetDescription() { return "Disallows bot to jump on to a flag team"; }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin