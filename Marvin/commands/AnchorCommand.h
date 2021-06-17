#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class AnchorCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("IsAnchor", false) == true) {
      game.SendPrivateMessage(sender, "Marv was already anchoring.");
    } else {
      game.SendPrivateMessage(sender, "Switching to anchor mode.");
    }

    bb.Set<bool>("IsAnchor", true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"anchor", "a"}; }
  std::string GetDescription() { return "Enable anchor behavior when basing"; }
  int GetSecurityLevel() { return 0; }
};

class RushCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    behavior::Blackboard& bb = bot.GetExecuteContext().blackboard;
    GameProxy& game = bot.GetGame();

    if (bb.ValueOr<bool>("IsAnchor", false) == false) {
      game.SendPrivateMessage(sender, "Marv was already rushing.");
    } else {
      game.SendPrivateMessage(sender, "Switching to rush mode.");
    }

    bb.Set<bool>("IsAnchor", false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"rush", "r"}; }
  std::string GetDescription() { return "Enable rush behavior when basing"; }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin
