#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class HelpCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender, const std::string& arg) override {
    if (sender.empty()) return;

    bot.GetGame().SendPrivateMessage(sender, "!commands {.c} -- see command list (pm)");
    bot.GetGame().SendPrivateMessage(sender, "!delimiter {.d} -- how to send multiple commands with one message (pm)");
    bot.GetGame().SendPrivateMessage(sender, "!modlist {.ml} -- see who can lock marv (pm)");
  }

  CommandAccessFlags GetAccess(Bot& bot) { return CommandAccess_Private; }
  std::vector<std::string> GetAliases() { return {"help", "h"}; }
  std::string GetDescription() { return "Helps"; }
  int GetSecurityLevel() { return 0; }
};

}  // namespace marvin
