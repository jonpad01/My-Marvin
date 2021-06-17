#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class RepelCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseRepel", false) == true) {
      game.SendPrivateMessage(sender, "Already using repels.");
    } else {
      game.SendPrivateMessage(sender, "Turning repels on.");
    }

    bb.Set<bool>("UseRepels", true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"repel", "rep"}; }
  std::string GetDescription() { return "Sets repels on"; }
  int GetSecurityLevel() { return 0; }
};

class RepelOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("UseRepel", false) == false) {
      game.SendPrivateMessage(sender, "Repels were already off.");
    } else {
      game.SendPrivateMessage(sender, "Turning repels off.");
    }

    bb.Set<bool>("UseRepel", false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_All; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"repeloff", "ro"}; }
  std::string GetDescription() { return "Sets repels off"; }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin
