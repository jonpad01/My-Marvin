#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class SayCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

     game.SendChatMessage(arg);

  }


  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"say"}; }
  std::string GetDescription() {
    return "I will say whatever you type.";
  }
  int GetSecurityLevel() { return 1; }
  CommandType GetCommandType() { return CommandType::Action; }
};

}  // namespace marvin