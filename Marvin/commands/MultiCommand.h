#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class MultiCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseMultiFire", false) == true) {
      game.SendPrivateMessage(sender, "Multifire is already on.");
    } else {
      game.SendPrivateMessage(sender, "Turning on multifire.");
    }

    bb.Set<bool>("UseMultiFire", true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"multi", "m"}; }
  std::string GetDescription() { return "Sets multifire on"; }
  int GetSecurityLevel() { return 0; }
};

class MultiOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseMultiFire", false) == false) {
      game.SendPrivateMessage(sender, "Multifire is already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning off multifire.");
    }

    bb.Set<bool>("UseMultiFire", false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"multioff", "mo"}; }
  std::string GetDescription() { return "Sets multifire off"; }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin
