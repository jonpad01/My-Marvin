#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class DelimiterCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    if (sender.empty()) return;

    bot.GetGame().SendPrivateMessage(sender, "The ';' character can be used to send more than one command in a single message." );
    bot.GetGame().SendPrivateMessage(sender, "");
    bot.GetGame().SendPrivateMessage(sender, "Example:  .setship 2;a;sf 1");
    bot.GetGame().SendPrivateMessage(sender, "");
    bot.GetGame().SendPrivateMessage(sender, "This command will set marv into ship 2, turn on anchoring, and send a freq change request to team 1.");
    bot.GetGame().SendPrivateMessage(sender, "The command prefix '.' or '!' is only used once at the beginning.");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  std::vector<std::string> GetAliases() { return {"delimiter"}; }
  std::string GetDescription() { return "Delimiter usage"; }
  //int GetSecurityLevel() { return 0; }
  SecurityLevel GetSecurityLevel() { return SecurityLevel::Unrestricted; }
  CommandType GetCommandType() { return CommandType::Info; };
};

}  // namespace marvin