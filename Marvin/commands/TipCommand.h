#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class TipCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender,
               const std::string& alias, const std::string& arg) override {
    Blackboard& bb = bot.GetBlackboard();
    GameProxy& game = bot.GetGame();

    game.SendPrivateMessage(sender, "If you like my bot, you can send a tip.");
    game.SendPrivateMessage(sender, "Bitcoin: bc1qqkcwgku2eap9pkkhwx57666nef0472py0vn4y6");
    game.SendPrivateMessage(sender, "Paypal: https://www.paypal.com/donate/?hosted_button_id=WVY2SCL7YN5UA");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  CommandFlags GetFlags() { return CommandFlag_Lockable; }
  std::vector<std::string> GetAliases() { return {"tip"}; }
  std::string GetDescription() { return "Send the dev a tip!"; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Info; }
};

}  // namespace marvin
