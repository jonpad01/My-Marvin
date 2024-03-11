#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class UFOCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    game.SendPrivateMessage(sender, "Turning on UFO mode.");

    game.SetStatus(StatusFlag::Status_UFO, true);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"ufo"}; }
  std::string GetDescription() {
    return "Turn UFO status on.";
  }
  int GetSecurityLevel() { return 5; }
};

class UFOOffCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    game.SendPrivateMessage(sender, "Turning off UFO mode.");

    game.SetStatus(StatusFlag::Status_UFO, false);
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"ufooff"}; }
  std::string GetDescription() {
    return "Turn UFO status off.";
  }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin