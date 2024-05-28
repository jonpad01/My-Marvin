#pragma once

#include "../Bot.h"
#include "CommandSystem.h"

namespace marvin {

class HelpCommand : public CommandExecutor {
 public:
  void Execute(CommandSystem& cmd, Bot& bot, const std::string& sender,
               const std::string& alias, const std::string& arg) override {
    if (sender.empty()) return;

    bot.GetGame().SendPrivateMessage(sender, "------------------------------------------------------");
    bot.GetGame().SendPrivateMessage(sender, " ");
    bot.GetGame().SendPrivateMessage(sender, "   -- !commands {.c}  - pls be gentle uwu -_- (pm)");
    bot.GetGame().SendPrivateMessage(sender, " ");
    bot.GetGame().SendPrivateMessage(sender, "-------------   Commands By Type  --------------------");
    bot.GetGame().SendPrivateMessage(sender, " ");
    //bot.GetGame().SendPrivateMessage(sender, "0. - list all (or leave empty)");

    for (std::size_t i = 0; i < kCommandTypeStr.size(); i++) {
      bot.GetGame().SendPrivateMessage(sender, std::to_string(i) + ". - " + kCommandTypeStr[i]);
    }
    
    bot.GetGame().SendPrivateMessage(sender, " ");
    bot.GetGame().SendPrivateMessage(sender, "----------------  Example Usage  ---------------------");
    bot.GetGame().SendPrivateMessage(sender, " ");
    bot.GetGame().SendPrivateMessage(sender, "-- !commands 1 (view type 1 commands)");
    bot.GetGame().SendPrivateMessage(sender, "-- .commands   (view all commands)");
    bot.GetGame().SendPrivateMessage(sender, "-- .c 0        (view all commands)");
    bot.GetGame().SendPrivateMessage(sender, "-- .c          (view all commands)");
    bot.GetGame().SendPrivateMessage(sender, " ");
    bot.GetGame().SendPrivateMessage(sender, "------------------------------------------------------");

   // bot.GetGame().SendPrivateMessage(sender, "-- !actions {.ac}   -- action commands (pm)");
   // bot.GetGame().SendPrivateMessage(sender, "-- !behaviors {.bc} -- behavior commands (pm)");
  //  bot.GetGame().SendPrivateMessage(sender, "-- !toggles {.tc}   -- toggle commands (pm)");
   // bot.GetGame().SendPrivateMessage(sender, "-- !hosting {.hc}   -- hosting commands (pm)");
  //  bot.GetGame().SendPrivateMessage(sender, "-- !info {.ic}      -- info commands (pm)");
  }

  CommandAccessFlags GetAccess() { return CommandAccess_Private; }
  void SetAccess(CommandAccessFlags flags) { return; }
  std::vector<std::string> GetAliases() { return {"help", "h"}; }
  std::string GetDescription() { return "Helps"; }
  int GetSecurityLevel() { return 0; }
  CommandType GetCommandType() { return CommandType::Info; };
};

}  // namespace marvin
